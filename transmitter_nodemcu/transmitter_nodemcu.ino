/*
 * Author - Tarun Vishwakarma
 * Date - July 1, 2023
 * 
 * Transmitter Unit - V2.0
 * 
 * Transmitter Unit - this will be installed on water storage container.
 * It will analyse water levels by using sensor and transmit data to receiver/main unit.
 * 
 * Device - Node MCU
 * 
 * Conversion formula
 * minVal = 12 cm = 0 (empty)-> 99% (filled)
 * maxVal = 78 cm  = 99 -> 0%
 * step = 99/(maxVal-minVal)
 * f = 100 - [(Reading-minVal)*step]
 *  
 */

#include "DHT.h"
#include <stdio.h>
#include "Credentials.h"
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <Ticker.h>


#define AVG_COUNT 10
#define INTERVAL 100
#define PACKET_INTERVAL 2500

#define MIN_VAL 12
#define STEP 1.5

/*
#define D0 16 GPIOs..
#define D1 5 
#define D2 4 
#define D3 0  
#define D4 2 
#define D5 14
#define D6 12
#define D7 13
#define D8 15

#define S1 8 GPIO8
#define S2 9 GPIO9
#define S3 10 GPIO10

#define SC 11 GPIO11
#define S0 6 GPIO6
#define SK 7 GPIO7

#define A0 A0
#define RX 3 GPIO3
#define TX 1 GPIO1
*/


#define P10 16
#define P25 5 
#define P35 4  
#define P50 0
#define P65 2
#define P75 14
#define P85 12
#define P100 13

#define DHTTYPE DHT22
#define DHTPIN 15 
#define PACKET_LEN 22

DHT dht(DHTPIN, DHTTYPE);

WiFiClient client;

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

Adafruit_MQTT_Publish PUB_TEMP = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/monitoring");
Adafruit_MQTT_Publish PUB_LEVEL = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/water-level");
Adafruit_MQTT_Publish PUB_MESSAGE = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/message");

Ticker timeTicker;

void setup() {
  Serial.begin(9600);
  delay(100);
  Serial.println("TX-UNIT: Starting...");
  dht.begin();

  digitalWrite(LED_BUILTIN, HIGH);
  delay(250);
  digitalWrite(LED_BUILTIN, LOW);
  delay(250);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(250);
  digitalWrite(LED_BUILTIN, LOW);
  delay(250);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(250);
  digitalWrite(LED_BUILTIN, LOW);
  delay(250);

  pinMode(P10, INPUT_PULLUP);
  pinMode(P25, INPUT_PULLUP);
  pinMode(P35, INPUT_PULLUP);
  pinMode(P50, INPUT_PULLUP);
  pinMode(P65, INPUT_PULLUP);
  pinMode(P75, INPUT_PULLUP);
  pinMode(P85, INPUT_PULLUP);
  pinMode(P100, INPUT_PULLUP);
  
  delay(25);

  connectWifi();
  delay(1200);
  MQTT_connect();
  delay(50);
  timeTicker.attach_ms(1000,updateTimeTickerFlag);
  Serial.println("TX-UNIT: INIT-DONE..");
}




//Humidity: 36.00%  Temperature: 34.60°C   Heat index: 35.43°C 60

boolean tick = false;
int tickCount = 0;

int i;
char buff[50];
        
float h;
float t;
float hic;

long duration;
int distance;
int level;

boolean wifiConnected = false;
boolean mqttConnected = false;
void updateTimeTickerFlag(){
  tick = true;
  tickCount++;
  //if(tickCount>=60){
    //tickCount=0;
  //}
}


boolean connectWifi(){
  int retry = 10;

   //Serial.print("Connecting to ");
   //Serial.println(WLAN_SSID);
      
   WiFi.begin(WLAN_SSID, WLAN_PASS);
   while (retry>=0) {
      if(WiFi.status() != WL_CONNECTED){
        delay(500);
      } else{
        Serial.println("TX-UNIT: Online!");
        wifiConnected = true;
        break;
      }
      retry--;
      if(retry==0){
        wifiConnected = false;
        Serial.println("TX-UNIT: Offline!");
        break;
      } 
   }
  return wifiConnected;
}

boolean MQTT_connect() {
  // Stop if already connected.
  if (mqtt.connected()) {
    return true;
  }
  int8_t ret;
  //Serial.print("Connecting to MQTT... ");
  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       //Serial.println(mqtt.connectErrorString(ret));
       //Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(1000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         return false;
       }
  }

  mqttConnected = mqtt.connected() && wifiConnected;
  if(mqttConnected){
    Serial.println("TX-UNIT: MQTT OK!");
  }else{
    Serial.println("TX-UNIT: NO MQTT!");
  }

  return mqttConnected;
}
 
void publishTemp(float temp){

      if (!PUB_TEMP.publish(String(temp).c_str())) {
        //Serial.println("Pub Temp Failed!");
      } else {
        //Serial.println("Pub temp in OK");
      }
  
}
void publishLevel(int level){
      if (!PUB_LEVEL.publish(String(level).c_str())) {
        //Serial.println("Pub Level Failed!");
      } else {
        //Serial.println("OK");
      }
  
}

void publishMessage(String message){
      if (!PUB_MESSAGE.publish(message.c_str())) {
        //Serial.println("Pub Level Failed!");
      } else {
        //Serial.println("OK");
      }
  
}

void loop() {
  h = 0.0;
  t = 0.0;
  level = 0;

  for (i=0; i<AVG_COUNT; i++){
    h = h + dht.readHumidity();
    //Serial.print(h);
    t = t + dht.readTemperature();
    //Serial.print(t);
    delay(INTERVAL);    
  }

  h = h/AVG_COUNT;
  t = t/AVG_COUNT;

  level = levelPolling();

  hic = dht.computeHeatIndex(t, h, false);

  String packet = "";
  String temp = String(t,1);
  packet.concat(temp); //4
  packet.concat("'C "); //7
  String humid = String(h,0);
  packet.concat(humid); //9
  packet.concat("% "); //11
  String index = String(hic,1);
  packet.concat(index); //16
  packet.concat("'C "); //19
  //packet.concat(" ");
  packet.concat(level); //21
  packet.concat("%"); //22
  
  int len  = packet.length();
  //Serial.println("packet if under construct..");
 
  //Packet length = 22
  if(len<PACKET_LEN){
    int diff = PACKET_LEN-len;
    for(i = 0; i< diff; i++){
      packet.concat(" ");
    }
  }

  char arr[PACKET_LEN];

  for(i=0; i<PACKET_LEN; i++){
    arr[i] = packet[i];
  }
  //Serial.println("sending packet now..");
  Serial.println(packet);  

  if(tick){
     if(tickCount>=59){
      tickCount=0;
      //Serial.println("1 min loop");
      if(WiFi.status() != WL_CONNECTED){
        wifiConnected = connectWifi();     
      }else{
        wifiConnected = true;
      }
      if(wifiConnected){
        if(!mqttConnected)
          MQTT_connect();         
        //Serial.println("publishing..");  
        publishTemp(t); 
        publishLevel(level);
        publishMessage(packet);
      }
     }
  }
                                                                                                                                                  
 
  delay(PACKET_INTERVAL);

}

int levelPolling(){
  if(digitalRead(P100)==LOW){
    return 100;
  }else if(digitalRead(P85)==LOW){
    return 85;
  }else if(digitalRead(P75)==LOW){
    return 75;
  }else if(digitalRead(P65)==LOW){
    return 65;
  }else if(digitalRead(P50)==LOW){
    return 50;
  }else if(digitalRead(P35)==LOW){
    return 35;
  }else if(digitalRead(P25)==LOW){
    return 25;
  }else if(digitalRead(P10)==LOW){
    return 10;
  }else{
    return -1;
  }
}
