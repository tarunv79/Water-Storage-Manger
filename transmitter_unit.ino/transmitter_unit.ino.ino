
/*
 * Author - Tarun Vishwakarma
 * Date - March 8, 2022
 * 
 * Transmitter Unit - this will be installed on water storage container.
 * It will analyse water levels by using ultrasonic sensor and transmit data to receiver/main unit.
 * 
 * Device - Arduino Nano
 * Transmitter - RF(433 MHz) through UART
 * Ultrasonic - HCSR04
 * 
 * Connection:
 * TRIGGER PIN - D7
 * ECHO PIN - D8
 * 
 * Conversion formula
 * minVal = 12 cm = 0 (empty)-> 99% (filled)
 * maxVal = 78 cm  = 99 -> 0%
 * step = 99/(maxVal-minVal)
 * f = 100 - [(Reading-minVal)*step]
 * 
 * RF-DATA / TX-PIN D12 (Note - TX pin is not used in RH_ASK library, hence use default pin 12 for transmitter)
 * 
 */


#include "DHT.h"
#include <stdio.h>

#include <RH_ASK.h>
#ifdef RH_HAVE_HARDWARE_SPI
#include <SPI.h> // Not actually used but needed to compile
#endif

#define DHTTYPE DHT11
#define DHTPIN 6

#define AVG_COUNT 10
#define INTERVAL 100
#define PACKET_INTERVAL 2500

#define MIN_VAL 12
#define STEP 1.5

#define P10 7
#define P25 A5
#define P35 A4
#define P50 A3
#define P65 A2
#define P80 A1
#define P100 A0
 

RH_ASK driver;
DHT dht(DHTPIN, DHTTYPE);

const int TRIG = 7; //PD7 - D7
const int ECHO = 8; //PB0 - D8
boolean sensorEnabled = false;

void setup() {
  if(sensorEnabled){
    pinMode(TRIG, OUTPUT);
    pinMode(ECHO, INPUT);
    pinMode(LED_BUILTIN, OUTPUT);
  }

  #ifdef RH_HAVE_SERIAL
    Serial.begin(9600);   // Debugging only
  #endif
      if (!driver.init())
  #ifdef RH_HAVE_SERIAL
         Serial.println("TX-UNIT:INIT-FAILED!");
  #else
    ;
  #endif

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
  pinMode(P80, INPUT_PULLUP);
  pinMode(P100, INPUT_PULLUP);

  delay(25);
  
  Serial.println("TX-UNIT: INIT-DONE..");
}

//Humidity: 36.00%  Temperature: 34.60°C   Heat index: 35.43°C 60

int i;
char buff[50];
        
float h;
float t;
float hic;

long duration;
int distance;
int level;



void loop() {  

  h = 0.0;
  t = 0.0;
  level = 0;
  
  for (i=0; i<AVG_COUNT; i++){
    h = h + dht.readHumidity();
    t = t + dht.readTemperature();

    if(sensorEnabled){
      digitalWrite(TRIG, LOW);
      delayMicroseconds(2);
      digitalWrite(TRIG, HIGH);
      delayMicroseconds(10);
      digitalWrite(TRIG, LOW);
  
      duration = pulseIn(ECHO, HIGH);
      distance = duration*0.017;
  
      level = level + distance;
    }else{
      //level = level + levelPolling();
    }
  
    delay(INTERVAL);    
  }

  h = h/AVG_COUNT;
  t = t/AVG_COUNT;
  //level = level/AVG_COUNT;
  
  //if(sensorEnabled){
    //level = 100-((level-MIN_VAL)*STEP);
  //}
  level = levelPolling();
  
  hic = dht.computeHeatIndex(t, h, false);
  
  String packet = "";
  String temp = String(t,1);
  packet.concat(temp);
  packet.concat("'C ");
  String humid = String(h,0);
  packet.concat(humid);
  packet.concat("% ");
  String index = String(hic,1);
  packet.concat(index);
  //packet.concat("'C ");
  packet.concat(" ");
  packet.concat(level);
  packet.concat("%");
  
  int len  = packet.length();
  //Serial.println(len);

  //number 20 is the number of 0 chars in single row of alphanumeric LCD, so we are limiting the string to be displayed in single line only.
  if(len<20){
    int diff = 20-len;
    for(i = 0; i< diff; i++){
      packet.concat(" ");
    }
  }

  char arr[20];

  for(i=0; i<20; i++){
    arr[i] = packet[i];
  }

  //Serial.println(packet);
  Serial.println(packet);                                                                                                                                                  
  driver.send((uint8_t *)arr, 20);
  
  //sprintf(buff,"%s %dcm",temp,level);
  //driver.send((uint8_t *)buff, strlen(buff));
  //Serial.println((char*)(uint8_t *)buff);
  driver.waitPacketSent();
  
  delay(PACKET_INTERVAL);
}

int levelPolling(){
  if(digitalRead(P100)==LOW){
    return 100;
  }else if(digitalRead(P80)==LOW){
    return 80;
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
