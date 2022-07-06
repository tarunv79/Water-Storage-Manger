
/*
 * Author - Tarun Vishwakarma
 * Date Created - April 1, 2022
 * 
 * Host Device for Arduino - Responsible for controlling water pumps/starters 
 * and also serve as wifi client for Master Arduino
 * 
 * 
 * 
 * 
 * 
 * Command Prefix List:
 * MSG:
 * STS:WIFI:1
 * TM:
 * 
 */





#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Ticker.h>
 
#include "TV.h" 

#define WLAN_SSID       ""
#define WLAN_PASS       ""

#define AIO_SERVER      "io.adafruit.com"
#define AIO_USERNAME    ""
#define AIO_KEY         ""
#define AIO_SERVERPORT  1883  //1883 for non-secure



#define ON LOW
#define OFF HIGH
#define DEBOUNCE 200
#define SHORTDELAY 100


#define SERIAL(x) Serial.println(x)

Ticker timeTicker;
Ticker statusTicker;

WiFiClient client;
WiFiUDP ntpUDP;


const long utcOffsetInSeconds = 19800; //+5.5x60x60 
char daysOfTheWeek[7][12] = {"Sun ", "Mon ", "Tue ", "Wed ", "Thu ", "Fri ", "Sat "};
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

int hour = 0;
int minute = 0;
int hours = 0;
int minutes = 0;

boolean wifiConnected = true;
boolean mqttConnected = false;
//Bug Work Around
//boolean MQTT_connect();


void setup() {
  delay(100);
  Serial.begin(9600);
  SERIAL(CMD_MSG_PRE+"Starting node mcu...");

  connectWifi();
  delay(100);

  SERIAL(CMD_MSG_PRE+"Time client begin...");
  timeClient.begin();   
  timeTicker.attach_ms(5000,updateTime);
  statusTicker.attach_ms(30000,updateStatus);
  delay(10);
  SERIAL(CMD_MSG_PRE+"Host Initialized!");
}

void loop() {  

}


void connectWifi(){
  if(WiFi.status() == WL_CONNECTED && wifiConnected){
    return;
  }else if(WiFi.status() == WL_CONNECTED && !wifiConnected){
    SERIAL(CMD_STATUS_PRE+CMD_STATUS_WIFI_PRE+"1");
    wifiConnected = true;
    return;
  }
    
  int retry = 10;

   String msg = CMD_MSG_PRE+"SSID: ";
   msg+=WLAN_SSID;
   SERIAL(msg);
     
   WiFi.begin(WLAN_SSID, WLAN_PASS);
   while (retry>=0) {
      if(WiFi.status() != WL_CONNECTED){
        delay(1000);
      } else{

        SERIAL(CMD_MSG_PRE+"WiFi connected!");
        SERIAL(CMD_STATUS_PRE+CMD_STATUS_WIFI_PRE+"1");
        wifiConnected = true;
        return;
      }
      retry--;
      if(retry==0){
        wifiConnected = false;
        SERIAL(CMD_MSG_PRE+"Wifi Disconnected!");
        SERIAL(CMD_STATUS_PRE+CMD_STATUS_WIFI_PRE+"0");
        break;
      } 
   }
  wifiConnected = false;  
}

void updateStatus(){
  connectWifi();
}

void updateTime(){
  timeClient.update();
  String t = CMD_TIME_PRE;
  t+= daysOfTheWeek[timeClient.getDay()];
  hours = timeClient.getHours();
  minutes = timeClient.getMinutes();
  int seconds = timeClient.getSeconds();

  t+=hours;
  t+=":";
  t+=minutes;
  
  SERIAL(t);

}
