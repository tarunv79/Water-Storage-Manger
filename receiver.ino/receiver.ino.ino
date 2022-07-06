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

#include "TV.h" 
#include "Ticker.h"


#define IN0 6 //IC pin 12
#define IN1 7 //IC pin 13
#define IN2 8 //IC pin 14

#define OUT0 A0
#define OUT1 A1
#define OUT2 A2
#define OUT3 A3
#define OUT4 A4
#define OUT5 A5
#define OUT6 10

#include <RH_ASK.h>
#ifdef RH_HAVE_HARDWARE_SPI
#include <SPI.h> // Not actually used but needed to compile
#endif

RH_ASK driver;
//RH_ASK driver(2000, 0, 1, 0);

const int rs = 12, en = 13, d4 = 5, d5 = 4, d6 = 3, d7 = 9;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

byte Wifi[8] = {
0b00000,
0b00000,
0b00111,
0b01000,
0b10011,
0b10100,
0b10101,
0b10101
};

byte Wifi0[8] = {
0b11111,
0b00000,
0b01111,
0b00000,
0b00111,
0b00000,
0b00011,
0b00001
};

byte Wifi1[8] = {
0b11111,
0b00000,
0b11110,
0b00000,
0b11100,
0b00000,
0b11000,
0b10000
};

byte Lock[8] = {
0b01110,
0b10001,
0b10001,
0b11111,
0b11011,
0b11011,
0b11111,
0b11111
};

byte MqSvc[8] = {
0b00100,
0b01110,
0b10101,
0b00100,
0b00100,
0b00100,
0b00100,
0b11111
};

byte Clk[8] = {
0b01110,
0b00000,
0b01110,
0b10101,
0b10111,
0b10001,
0b01110,
0b10001
};

byte Routine[8] = {
0b00100,
0b01110,
0b00101,
0b10001,
0b10001,
0b10100,
0b01110,
0b00100
};

byte Port[8] = {
0b01110,
0b01110,
0b01110,
0b00000,
0b00000,
0b11011,
0b11011,
0b11011
};




//Symbols flags
boolean isNetwork = false;
boolean isAdafruit = false;
boolean isRF = false;
boolean isTimer = false;
boolean isScheduler = false;


String serialData = "";
bool isSerialData = false; 


void updateStatus();

Ticker statusTicker(updateStatus, 2000, 0, MILLIS);

void setup() {  
  lcd.begin(20, 4);
  lcd.clear();
  lcd.print("Tarun Labs!");
  lcd.setCursor(0,3);
  lcd.print("Initializing...");
  delay(500);

  lcd.createChar(0, Wifi0);
  lcd.createChar(1, Wifi1);
  lcd.createChar(2, Lock);
  lcd.createChar(3, MqSvc);
  lcd.createChar(4, Clk);
  lcd.createChar(5, Routine);
  lcd.createChar(6, Wifi);
  lcd.createChar(7, Port);
  
  pinMode(IN0, INPUT_PULLUP);
  pinMode(IN1, INPUT_PULLUP);
  pinMode(IN2, INPUT_PULLUP);

  pinMode(OUT0, OUTPUT);
  pinMode(OUT1, OUTPUT);
  pinMode(OUT2, OUTPUT);
  pinMode(OUT3, OUTPUT);
  pinMode(OUT4, OUTPUT);
  pinMode(OUT5, OUTPUT);
  pinMode(OUT6, OUTPUT);
 

  Serial.begin(9600); //to communicate with host NodeMCU device
  serialData.reserve(200);

  if (!driver.init()){
    lcd.setCursor(0,2);
    lcd.print("RF Initialization failed!");  
    delay(500);
  }
  
  delay(250);
  lcd.clear();  

  lcd.print("Ready!");

}

  // set the cursor to column 0, line 1 initially
  // (note: line 1 is the second row, since counting begins with 0):
void loop() {

   buttonPolling();
   receiverPolling();
   
   statusTicker.update();
                                                                                                                                                                                                                                                                                                                                          
}



void buttonPolling(){
  if(digitalRead(IN0)==LOW){
    digitalWrite(OUT0, HIGH);
    delay(1000);
    digitalWrite(OUT0, LOW);
  }
  if(digitalRead(IN1)==LOW){
    digitalWrite(OUT1, HIGH);
    delay(1000);
    digitalWrite(OUT1, LOW);
  }
 
}

void receiverPolling(){
  uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
  uint8_t buflen = sizeof(buf);
  if (driver.recv(buf, &buflen)) // Non-blocking
  {
    int i;
    lcd.setCursor(0,3);
      
    for (i=0;i<buflen;i++){
      lcd.print((char)buf[i]);
    }
    
  }
}

void updateStatus(){
  if(isNetwork){
    lcd.setCursor(0,0);
    lcd.write(byte(6));
  }else{
    lcd.setCursor(0,0);
    lcd.print("?");
  }

  lcd.setCursor(3,0);
  lcd.write(byte(3));

  lcd.setCursor(5,0);
  lcd.write(byte(4));

  lcd.setCursor(7,0);
  lcd.write(byte(5));

  lcd.setCursor(9,0);
  lcd.write(byte(2));

  lcd.setCursor(11,0);
  lcd.write(byte(7));

  lcd.setCursor(0,2);
  lcd.print("                    ");
  lcd.setCursor(0,2);
  lcd.print("how are you?");
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
      isSerialData = true;
    }
  }

    if(isSerialData){
      debug(serialData);
    
    char ch0 = serialData.charAt(0);
    char ch1 = serialData.charAt(1);
    char ch2 = serialData.charAt(2);

    if(ch0=='M' && ch1=='S' && ch2=='G'){
      debug("Message received");
      lcd.setCursor(0,2);
      lcd.print("                    ");
      lcd.setCursor(0,2);
      lcd.print(serialData.substring(4));
      serialData = "";
      isSerialData = false;
    }
    
    else if(ch0=='T' && ch1=='I' && ch2=='M'){
      debug("time update!");
      lcd.setCursor(0,1);
      lcd.print("          ");
      lcd.setCursor(0,1);
      lcd.print(serialData.substring(4));
      serialData = "";
      isSerialData = false;
    }

    else if(ch0=='S' && ch1=='T' && ch2=='S'){
      debug("Status update");
      if(serialData.charAt(4)=='W' && serialData.charAt(5)=='I'){
        if(serialData.charAt(9)=='1'){
            lcd.setCursor(0,0);
            //lcd.write(byte(0));
            //lcd.write(byte(1));
            lcd.write(byte(6));
            isNetwork = true;
        }else if(serialData.charAt(9)=='0'){
            lcd.setCursor(0,0);
            lcd.print("?");
            isNetwork = false;
        }
      }
      lcd.setCursor(0,1);
      lcd.print("             ");
      lcd.setCursor(0,1);
      lcd.print(serialData.substring(4));
      serialData = "";
      isSerialData = false;
    } 
    isSerialData = false;
    serialData = "";
   }
    


}

void debug(String s){
  Serial.println(s);
}
