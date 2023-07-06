
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
#define DHTPIN 9

#define AVG_COUNT 10
#define INTERVAL 100
#define PACKET_INTERVAL 2500

#define MIN_VAL 12
#define STEP 1.5

RH_ASK driver;
DHT dht(DHTPIN, DHTTYPE);

const int TRIG = 7; //PD7 - D7
const int ECHO = 8; //PB0 - D8

void setup() {
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);


  #ifdef RH_HAVE_SERIAL
    Serial.begin(9600);   // Debugging only
  #endif
      if (!driver.init())
  #ifdef RH_HAVE_SERIAL
         Serial.println("INIT-FAILED");
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
  
  Serial.println("INIT-DONE");
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

    digitalWrite(TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG, LOW);

    duration = pulseIn(ECHO, HIGH);
    distance = duration*0.017;

    level = level + distance;
  
    delay(INTERVAL);    
  }

  h = h/AVG_COUNT;
  t = t/AVG_COUNT;
  level = level/AVG_COUNT;
  level = 100-((level-MIN_VAL)*STEP);

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
  packet.concat(" ~");
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
