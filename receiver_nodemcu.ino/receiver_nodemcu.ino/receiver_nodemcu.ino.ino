 
/*
 * this code is for Receiver Unit of Smart Water Storage Control System
 * Date Started: 28/11/2022
 * Author: Tarun Vishwakarma
 */

 /*
  * 
  * 
  * 
Planned Features:
- Storage level monitor on LCD and LED levels
- Temperature and Humidity on LCD
- Auto motor switch on/off based on cut-off settings
- wifi enabled and remote monitoring support


/*
 * Peripherials
 * -OLED
 * -Transmiter Data in via UART Rx
 * -Neo Pixel LED Strip
 * -Buttons
 */

/*
 * Connections in this version:
 *  OLED : 
 *    SCL of OLED -D1
 *    SDA of OLED -D2 
 *  Buttons:
 *    S1 = Buzzer off button / Prev
 *    S2 = Refresh(reset) button / Next
 *    S3 = Options/Settings/Menu / Exit
 *    S4 = Ok / Save 
 *  RELAYS:
 *    RELAY1 = D5
 *    RELAY2 = D6
 *    RELAY3 = D7
 *    RELAY4 = D8    
 */

/*
#define WLAN_SSID       ""
#define WLAN_PASS       ""


#define AIO_SERVER      ""
#define AIO_USERNAME    ""
#define AIO_KEY         ""
#define AIO_SERVERPORT    //1883 for non-secure
*/

/* Pins Mapping
D0-16 - for wake, do not use this
D1-5
D2-4
D3-0
D4-2
D5-14
D6-12
D7-13
D8-15
  //SD3=GPIO10
  //SD2=GPIO9
A0-17
RX=3=GPIO3
TX=1=GPIO1
*/

#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Ticker.h>
#include <SPI.h>
#include "Credentials.h"

#include <Adafruit_NeoPixel.h>


#define D0 16 //button pin
#define D1 5 //SCL of OLED
#define D2 4 //SDA of OLED
#define D3 0  //button pin
#define D4 2 //Fast LED pin
#define D5 14
#define D6 12
#define D7 13
#define D8 15
#define D9 9 //SD2 - not found
#define D10 10 //SD3 - not found
#define RX 3
#define TX 1 
#define ADCPIN A0

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32 
#define OLED_RESET LED_BUILTIN

#define ON HIGH  
#define OFF LOW
#define DEBOUNCE 200
#define LONGPRESSTIME 1700
#define SHORTDELAY 100

#define NUM_LEDS 10
#define LED_PIN D4

WiFiClient client;
WiFiUDP ntpUDP;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

//follow the form: <username>/feeds/<feedname> for AIO
Adafruit_MQTT_Publish PUB_TEMP = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/monitoring");
Adafruit_MQTT_Publish PUB_LEVEL = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/water-level");
Adafruit_MQTT_Publish PUB_ACK = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/smart-control-command");
Adafruit_MQTT_Subscribe SUB_COMMAND = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/smart-control-command");


// You can specify the time server pool and the offset, (in seconds)
// additionaly you can specify the update interval (in milliseconds).
const long utcOffsetInSeconds = 19800; //+5.5x60x60 
//char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
char daysOfTheWeek[7][12] = {"Sun ", "Mon ", "Tue ", "Wed ", "Thu ", "Fri ", "Sat "};
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
//in.pool.ntp.org
//asia.pool.ntp.org

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

Ticker timeTicker;




const uint8_t SBUZZ= D0; //           
const uint8_t SCUTOFF = D3;
//const uint8_t S3 = D3;
//const uint8_t S4 = D4;

const uint8_t RELBUZZ = D5;
const uint8_t RELAY2 = D6;
const uint8_t RELAY3 = D7;
const uint8_t RELAY4 = D8;

boolean interrupt = false;
boolean sleep = false;


int hour = 0;
int minute = 0;
int hours = 0;
int minutes = 0;
boolean tick = false;
int tickCount = 0;
boolean isHold = false;
boolean changed = false;
boolean setTime = false;
String t = "";
String strTx = "";

int currentLevel = 0;
int outTemp = 0;
float inMinTemp = 100.0;
float inMaxTemp = -100.0;

float outMinTemp = 100.0;
float outMaxTemp = -100.0;

float inTemp = 0; 

String serialData = "";
boolean serialEventFlag = false;

int cutOffLevel=100;
boolean cutOffUpdate = false;
boolean buzzState = false;
boolean buzzerFlag = true;

boolean refreshDispFlag = false;
boolean logInsideTemp = false;

boolean wifiConnected = true;
boolean mqttConnected = false;

boolean remoteCommand = false;
boolean txInput = false;
//Bug Work Around
boolean MQTT_connect();

/*
ICACHE_RAM_ATTR void handleInterruptSBuzz() {
  buzzState = digitalRead(RELBUZZ)^1;
  digitalWrite(RELBUZZ, buzzState);
}
ICACHE_RAM_ATTR void handleInterruptSCutOff() {
  cutOffLevel-=25;
  if(cutOffLevel<=0)
    cutOffLevel=100;    
  cutOffUpdate = true;  
}*/

void setup() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Display init failed!!");
    //for(;;);
  }
   display.display();
   displayStartMsg();
   displayMsg(1,"Starting...");
   Serial.begin(9600);
   delay(10);
   
   pinMode(SBUZZ, INPUT_PULLUP);
   pinMode(SCUTOFF, INPUT_PULLUP);
   //pinMode(S3, INPUT_PULLUP);
   //pinMode(S4, INPUT_PULLUP);   
   //attachInterrupt(digitalPinToInterrupt(SBUZZ), handleInterruptSBuzz, FALLING);
   //attachInterrupt(digitalPinToInterrupt(SCUTOFF), handleInterruptSCutOff, FALLING);
   //attachInterrupt(digitalPinToInterrupt(S3), handleInterruptS3, FALLING);
   //attachInterrupt(digitalPinToInterrupt(S4), handleInterruptS4, FALLING);

   pinMode(RELBUZZ,OUTPUT);

   digitalWrite(RELBUZZ, OFF);
   Serial.println();
   
   pixels.begin();
   for(int i=0;i<=NUM_LEDS;i++){
    pixels.setPixelColor(i, pixels.Color(50-(i*5),i<5 ? (i*5) : (50-(i*5)),i*5));
    delay(25);
    pixels.show();
   }

   displayMsg(1,"Initializing...");
   connectWifi();

   delay(1200);
  
   mqtt.subscribe(&SUB_COMMAND);
   delay(500);
   mqttConnected = MQTT_connect();
   delay(50);
   timeClient.begin();   
   delay(100);
   timeTicker.attach_ms(1000,updateTimeTickerFlag);
   readTemp();
   updateTime();
}


void updateTimeTickerFlag(){
  tick = true;
  tickCount++;
  if(tickCount>=60){
    tickCount=0;
  }
}

boolean connectWifi(){
  int retry = 10;

   Serial.print("Connecting to ");
   Serial.println(WLAN_SSID);
   
   display.clearDisplay();
   display.setTextSize(1);
   display.setTextColor(WHITE);
   display.setCursor(0,0);
   display.println(F("connecting to "));
   display.print(F(WLAN_SSID));
   display.display();
   delay(100);
   
   WiFi.begin(WLAN_SSID, WLAN_PASS);
   while (retry>=0) {
      if(WiFi.status() != WL_CONNECTED){
        delay(500);
        Serial.print(".");      
        display.print(".");
        display.display();
      } else{
        Serial.println();
        Serial.println("WiFi connected");
        Serial.println("IP address: "); 
        Serial.println(WiFi.localIP());
        display.println("");
        display.print(F("Wifi connected!"));
        display.display();
        wifiConnected = true;
        break;
      }
      retry--;
      if(retry==0){
        wifiConnected = false;
        displayMsg(1,WLAN_SSID);
        displayNextLineMsg(1,"");
        displayNextLineMsg(1,"WiFi Not Connected!");
        break;
      } 
   }
   updateTime();
   refreshDisplay("",false);
  return wifiConnected;
}

void updateTime(){
  timeClient.update();
  t = daysOfTheWeek[timeClient.getDay()];
  hours = timeClient.getHours();
  minutes = timeClient.getMinutes();
  int seconds = timeClient.getSeconds();
      Serial.println(timeClient.getFormattedTime());
      Serial.print(daysOfTheWeek[timeClient.getDay()]);
      Serial.print(", ");      
      Serial.print(hours);
      Serial.print(":");       
      Serial.print(minutes);
      Serial.print(":");
      Serial.println(timeClient.getSeconds());

  t+=hours;
  t+=":";
  t+=minutes;
  refreshDisplay("",false);
}

void refreshDisplay(String message, boolean isMsg){
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println(t);
  //display.display();
  //display.setTextSize(1);
  //display.println("");
  if(wifiConnected){ 
    display.setCursor(64,0);
    display.print("((*))");
  }
  if(buzzState){
    display.setCursor(110,0);
    display.print(" |B");
  }
  if(isMsg){
    display.setCursor(0,8);
    display.print(message);
  }
  if(true){
    display.setCursor(0,16);
    display.print((strTx));
    //display.print(currentLevel);
  }
  if(true){
    display.setCursor(0,24);
    display.print(inTemp);
    display.print(" 'C @Room");
  }
  
  display.display();
}

void handleMqttSubscription(){
  if(mqttConnected){
    Adafruit_MQTT_Subscribe *subscription;
    while ((subscription = mqtt.readSubscription(700))) {
      String command; 
      boolean isRemoteCommand = false;
      if (subscription == &SUB_COMMAND) {   
        command = (char *)SUB_COMMAND.lastread;
      }
      Serial.println(("Got: remote command  : "+command));
      refreshDisplay("CMD:"+command,true); 
      processCommand(command, true);
    }    
  }
}


void processCommand(String command, boolean remoteUpdate){
    if(command.startsWith("SER=")){
      serialEventFlag=true;
      serialData = command.substring(4);
      publishAck("ok!");
    }else if(command.startsWith("LED:")){
      String val = getValue(command);
      updateLedBar(val.toInt());
      publishAck("ok!");
    }else if(command.startsWith("CTF:SET")){
      String val = getValue(command);
      updateCutOff(val.toInt());
      publishAck("ok!");
    }else if(command.startsWith("CTF:GET")){
      publishAck(String(cutOffLevel));
    }else if(command.startsWith("BUZ:OFF")){
      turnBuzzOff();
      publishAck("ok!");
    }else if(command.startsWith("DISP:CLR")){ 
      refreshDisplay("Remote DISP CLR",true);
      publishAck("ok!");
    }else if(command.startsWith("TEMP:GET:MIN:IN")){
      publishAck(String(inMinTemp));
    }else if(command.startsWith("TEMP:GET:MAX:IN")){
      publishAck(String(inMaxTemp));
    }else if(command.startsWith("TEMP:GET:MIN:OUT")){
      publishAck(String(outMinTemp));
    }else if(command.startsWith("TEMP:GET:MAX:OUT")){
      publishAck(String(outMaxTemp));
    }else if(command.startsWith("TEMP:MINMAX:CLR")){
      clearMinMaxTemp();
      publishAck("ok!");
    }else if(command.startsWith("TEMP:LOG:IN")){
      logInsideTemp = true;
      publishAck("ok!");
    }else if(command.startsWith("TEMP:LOG:OUT")){
      logInsideTemp = false;
      publishAck("ok!");
    }else if(command.startsWith("TEMP:UPDATE:F")){
      readTemp();
      updateTemp();
      publishTemp();
      refreshDisplay("remote update!",true);
      publishAck("ok!");
    }
    
}

void clearMinMaxTemp(){
   inMinTemp=100.0;
   inMaxTemp=-100.0;
   outMinTemp=100.0;
   outMaxTemp=-100.0;
}

void publishTemp(){
  if(mqttConnected){
    if(logInsideTemp){
      if (!PUB_TEMP.publish(String(inTemp).c_str())) {
        Serial.println("Pub Temp Failed!");
      } else {
        Serial.println("Pub temp in OK");
      }
    }else{
      if (!(String(outTemp).equals("NAN"))){
        if(!PUB_TEMP.publish(String(outTemp).c_str())){
          Serial.println("Pub Temp Failed!");
        } else {
          Serial.println("Pub temp out OK");
        }
       }
    }
  }
}
void publishLevel(String message){
  if(mqttConnected){
      if (!PUB_LEVEL.publish(message.c_str())) {
        Serial.println("Pub Level Failed!");
      } else {
        //Serial.println("OK");
      }
  }
}
void publishAck(String message){
  if(mqttConnected){
      if (!PUB_ACK.publish(message.c_str())) {
        Serial.println("Pub Level Failed!");
      } else {
        //Serial.println("OK");
      }
  }
}

String getValue(String input){
  int idx = input.indexOf("=");
  return input.substring(idx+1);
}

boolean MQTT_connect() {
  // Stop if already connected.
  if (mqtt.connected()) {
    return true;
  }
  int8_t ret;
  Serial.print("Connecting to MQTT... ");
  displayMsg(1,"Connecting MQTT...");
  delay(500);
  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       displayMsg(1,"Reconnecting AIO...");
       displayNextLineMsg(1,mqtt.connectErrorString(ret));
       mqtt.disconnect();
       delay(1000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         return false;
       }
  }

  mqttConnected = mqtt.connected() && wifiConnected;
  String sts = mqttConnected == 1 ? "1" : "0";
  Serial.println("MQTT Status: ");
  displayNextLineMsg(1,"MQTT Status: "+ sts);
  delay(1000);
  updateTime();
  return mqttConnected;
}


void displayMsg(int textSize, String msg){
  display.clearDisplay();
  display.setTextSize(textSize);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println(msg);
  display.display();
}

void displayLargeMsg(String msg){
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,16);
  display.println(msg);
  display.display();
}

void displayNextLineMsg(int textSize, String msg){
  display.setTextSize(textSize);
  display.println(msg);
  display.display();
}


void displayStartMsg(void){
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(10,0);
  display.println(F("Tarun"));
  display.println(F("  Labs!"));
  display.display();
  delay(500);
  display.startscrollright(0x00,0x0F);
  delay(1700);
  display.stopscroll();
}


void updateLedBar(int val){
  for(int c=0;c<=10;c++){
     pixels.setPixelColor(c, pixels.Color(0,0,0));
     pixels.show();
  }
  for(int i=10; i<=val; i+=10){
      int k = i/10;
      if(val<30){
        pixels.setPixelColor(k-1, pixels.Color(255,0,0));
        pixels.show();
      }else if(val<70){
        pixels.setPixelColor(k-1, pixels.Color(0,2,200));
        pixels.show();    
      }else if(val>=70){
        pixels.setPixelColor(k-1, pixels.Color(1,150,2));
        pixels.show();
      }
      if(val>=100){
        pixels.setPixelColor(k-1, pixels.Color(100,100,100));
        pixels.show();
      }
  }
  if(buzzState && val>=cutOffLevel){
    digitalWrite(RELBUZZ,ON);
  }
  if(!buzzState && val<90){
     buzzerFlag = true;
  }
  if(!buzzState && val>=cutOffLevel && buzzerFlag){
    digitalWrite(RELBUZZ,ON);
  }
  publishLevel(String(val));   
}


void readTemp(){
  int analogVal = 0;
  for(int i=0; i<5; i++){
    analogVal = analogVal + analogRead(ADCPIN);
    delay(5);
  }
  analogVal = analogVal/5;
  
  float milliVolts = (analogVal/1024.0)*3300;
  inTemp = milliVolts/10;
  Serial.print("Inside Temp = ");
  Serial.println(inTemp);
}

void handleSerialEvent(){
  //Format= "tt'C hh% ii ll%" -> len = 16 "25'C 15% 22 72%"
  String temp = "";
  String hum = "";
  String idx = "";
  String lev = "";

  boolean isTemp = true;
  boolean isHum = false;
  boolean isIdx = false;
  boolean isLev = false;
  
  for(int i = 0; i<serialData.length(); i++){
    char ch = serialData.charAt(i);
    if(ch=='\''){
      isTemp = false;
      isHum = true;
      i+=2;
      continue;
    }else if(ch == '%'){
      isHum =false;
      isIdx = true;
      i+=1;
      continue;
    }else if(ch == ' '){
      isIdx = false;
      isLev = true;
      continue;
    }else if(ch == '%'){
      isLev = false;
      break;
    }
    
    if(isTemp)
      temp+=ch;
    else if(isHum)  
      hum+=ch;
    else if(isIdx)
      idx+=ch;
    else if(isLev)
      lev+=ch; 
  }
  Serial.println("Temp="+temp);
  Serial.println("Humidity="+hum);
  Serial.println("Temp Index="+idx);
  Serial.println("Level="+lev);
  strTx = serialData;
  currentLevel=lev.toInt();
  outTemp=temp.toInt();
  updateLedBar(currentLevel);
  refreshDisplay("",false);
}

void updateTemp(){
    if(inTemp<inMinTemp){
      inMinTemp = inTemp;  
    }
    if(inTemp>inMaxTemp){
      inMaxTemp = inTemp;
    }  

    if(outTemp<outMinTemp){
      outMinTemp = outTemp;  
    }
    if(outTemp>outMaxTemp){
      outMaxTemp = outTemp;
    }    
}

void routine(){
  if(tick){
    //1 second interval
    if(tickCount>=59){
      //around 1 min interval    
      if(WiFi.status() != WL_CONNECTED){
        wifiConnected = connectWifi();     
      }else{
        wifiConnected = true;
      }
      if(wifiConnected){
        mqttConnected = MQTT_connect();   
        updateTime(); 
      }

      readTemp();
      updateTemp();
      publishTemp();
    }
    if(tickCount%10==0){
      //each 10 seconds
      if(mqttConnected){
        refreshDisplay("Busy",true);
        handleMqttSubscription();
        refreshDisplay("",false);
      }
    }
    
    tick = false;
  }  
}

void updateCutOff(){
  cutOffLevel-=25;
  updateCutOffFunc();    
}

void updateCutOff(int val){
  cutOffLevel=val;
  updateCutOffFunc();   
}
void updateCutOffFunc(){
  if(cutOffLevel<=0){
    cutOffLevel=100; 
  }else if(cutOffLevel>100){
    cutOffLevel=100;
  }
  String msg = "Buzz@ "+ String(cutOffLevel) +" %";
  displayLargeMsg(msg);
  delay(750);
  if(cutOffLevel!=100){
    buzzState=true;
  }else{
    buzzState=false;
  }
  buzzerFlag=true;
  
  refreshDisplay(msg,true);   
}

void turnBuzzOff(){
    digitalWrite(RELBUZZ, OFF); 
    buzzerFlag=false;
    cutOffLevel=100;
    buzzState=false;      
    refreshDisplay("Buzzer-off",true);  
    delay(250);
    refreshDisplay("",false);
}

void buttonPolling(){
  if(digitalRead(SBUZZ)==LOW){
    //turnBuzzOff();
    delay(DEBOUNCE);
  }
  if(digitalRead(SCUTOFF)==LOW){
    //updateCutOff(); 
    delay(DEBOUNCE);
  }  

  //menu options
  //min maxx temp clear and getVal
}

void loop() {    
  routine();
  if(serialEventFlag){
    handleSerialEvent();
    serialData = "";
    serialEventFlag = false;
  }
  buttonPolling();
}

void serialEvent() {
    while (Serial.available()) {
      // get the new byte:
      char inChar = (char)Serial.read();
      // add it to the inputString:
      serialData += inChar;
      // if the incoming character is a newline, set a flag so the main loop can
      // do something about it:
      if (inChar == '\n') {
        serialEventFlag = true;
      }
    }

}
