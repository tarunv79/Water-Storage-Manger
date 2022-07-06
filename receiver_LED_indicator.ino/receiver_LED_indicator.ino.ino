#include <FastLED.h>

/*
 * 
Author - Tarun Vishwakarma
Date - March 16, 2022

  The circuit:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 13
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 9
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin
 * 
 * RF-DATA / RX-PIN 11 (Note - RX pin is not used in RH_ASK library, hence use default pin 11 for receiver)
 * 
 * 3 input buttons in encoder mode - pin 6, 7 and 8
 * 7 output pins - A0 to A5 and pin 10

*/

#include <LiquidCrystal.h>
#include <FastLED.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>


#define LED_PIN 2
#define NUM_LEDS 10
#define RELAY 8
#define BUTTON 7

#include <RH_ASK.h>
#ifdef RH_HAVE_HARDWARE_SPI
#include <SPI.h> // Not actually used but needed to compile
#endif

RH_ASK driver;
//RH_ASK driver(2000, 0, 1, 0);

const int rs = 12, en = 13, d4 = 5, d5 = 4, d6 = 3, d7 = 9;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);


String serialData = "";
bool isSerialData = false; 


CRGB leds[NUM_LEDS];

void setup() {  
  lcd.begin(20, 4);
  lcd.clear();
  lcd.print("Tarun Labs!");
  lcd.setCursor(0,3);
  lcd.print("Initializing...");
  delay(500);

  Serial.begin(9600); //to communicate with host NodeMCU device
  serialData.reserve(200);

  if (!driver.init()){
    lcd.setCursor(0,2);
    lcd.print("RF Initialization failed!");  
    delay(500);
  }
  
  delay(250);
  lcd.clear();  
  //delay(100);
  lcd.print("Ready!");

  pinMode(RELAY, OUTPUT);
  pinMode(BUTTON,INPUT_PULLUP);

  digitalWrite(RELAY,HIGH);
  

  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);
  FastLED.clear();
  FastLED.show();
  leds[1] = CRGB(100,100,100);
  FastLED.setBrightness(10);
  FastLED.show();
  delay(1000);
  flashLEDs();
  delay(100);
  FastLED.clear();
}

int val = 0;
boolean relayFlag = false;
boolean buzzerFlag = true;
  // set the cursor to column 0, line 1 initially
  // (note: line 1 is the second row, since counting begins with 0):
void loop() {

   //buttonPolling();
   receiverPolling();
   if(relayFlag){
      digitalWrite(RELAY,LOW);
      if(digitalRead(BUTTON)==LOW){
        relayFlag = false;
        buzzerFlag = false;
        digitalWrite(RELAY,HIGH);
      }
      delay(100);
   }
   //statusTicker.update();
   
                                                                                                                                                                                                                                                                                                                                         
}

void updateLedBar(int val){
  FastLED.clear();
  for(int i=10; i<=val; i+=10){
      int k = i/10;
      if(val<30){
        leds[k-1] = CRGB(255, 0, 0);
        FastLED.show();
      }else if(val<70){
        leds[k-1] = CRGB(0, 0, 255);
        FastLED.show();    
      }else if(val>=70){
        leds[k-1] = CRGB(0, 255, 0);
        FastLED.show();
      }
      if(val>=100){
        leds[k-1] = CRGB(255, 255, 255);
        FastLED.show();
      }
  }

   if(val<90){
      buzzerFlag = true;
   }

   if(val>100 && buzzerFlag){
      relayFlag = true;
   }
  
 
}

void flashLEDs(){
  Serial.println("flasing LEDs...");
  for(int i=0; i<3; i++){
    leds[i] = CRGB(100, 0, 0);
    FastLED.show();
    delay(150);
  }
  for(int i=3; i<7; i++){
    leds[i] = CRGB(0, 0, 100);
    FastLED.show();
    delay(150);
  }
  for(int i=7; i<10; i++){
    leds[i] = CRGB(0, 100, 0);
    FastLED.show();
    delay(150);
  }
  delay(250);
  for(int i=0; i<10; i++){
    leds[i] = CRGB(100, 100, 100);
    FastLED.show();
    delay(5);
  }
  delay(250);
}


void receiverPolling(){
  uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
  uint8_t buflen = sizeof(buf);

  char level[3];
  if (driver.recv(buf, &buflen)) // Non-blocking
  {
    int i;
    lcd.setCursor(0,3);
      
    for (i=0;i<buflen;i++){
      lcd.print((char)buf[i]);
      if(i==17){
        level[0]=(char)buf[i];
      }else if(i==18){
        level[1]=(char)buf[i];
      }else if(i==19){
        level[2]=(char)buf[i];
      }
    }
    Serial.print("LEVEL= ");
    Serial.println(level);
    if(level[2]=='%'){
      level[2]='0';
      val = atoi(level);
      val = val/10;
    }else {
      val = atoi(level);
    }
    //Serial.print("VAL=");
    //Serial.println(val);
    updateLedBar(val);
    
  }
  
 
}
