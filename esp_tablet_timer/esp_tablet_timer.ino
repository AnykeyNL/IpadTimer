/*
 * --------------------------------------------------------------------------------------------------------------------
 * Tablet Timer Project
 * Written by Richard Garsthagen - the.anykey@gmail.com
 * 
 * Version 1.0 - June 27th 2018
 * https://github.com/AnykeyNL/IpadTimer
 * --------------------------------------------------------------------------------------------------------------------

 */
 
#include <Wire.h> // Enable this line if using Arduino Uno, Mega, etc.
//#include <TinyWireM.h> // Enable this line if using Adafruit Trinket, Gemma, etc.
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"
#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>

char ssid[] = "JanRichX";  //  your network SSID (name)
char pass[] = "ikwilliefde";   

#define RST_PIN   0     // Configurable, see typical pin layout above
#define SS_PIN    2    // Configurable, see typical pin layout above
#define SwitchPin 16   // pin that has the switch connected to it
#define piezoPin  15


Adafruit_7segment matrix = Adafruit_7segment();
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance

int sumids = 0;

unsigned int localPort = 2390;      // local port to listen for UDP packets
const char* ntpServerName = "europe.pool.ntp.org";
IPAddress timeServerIP;
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
WiFiUDP udp;

long timeNow = 0;
long timeLast = 0;
long DefaultTime = 10810; // Default time is 3 hours and 10 seconds.
long timeLeft = 0;
long lastMinuteStored = 0;
byte LastDay = 0;

bool CountDown = false;

// Card ID1, ID2, ID3, ID3, Func

int Cards[][5] = {  {0xA6, 0xF8, 0x83, 0x63, 254}, // reset used cards
                    {0x39, 0x97, 0x75, 0x63, 253}, // reset day
                    {0xD0, 0xB2, 0x70, 0xA3, 30},  // extra 30 minutes card
                    {0xC0, 0xF8, 0x2E, 0xA4, 30},  // extra 30 minutes card
                    {0x80, 0xEB, 0x0D, 0xA3, 30},  // extra 30 minutes card
                    {0xE0, 0x67, 0x30, 0xA4, 30},  // extra 30 minutes card
                    {0x30, 0xD1, 0x14, 0xA3, 30},  // extra 30 minutes card
                    {0x70, 0x81, 0x2D, 0xA4, 30},  // extra 30 minutes card
                    {0xEC, 0xC0, 0x2F, 0x07, 30}   // extra 30 minutes card
                };
int TotalCards = sizeof(Cards) / (sizeof(int)*5);

int Days[8] = {0,0,0,660,3600,10810,10810,10810}; // noday, monday, tuesday, wednessday, thursday, friday, saturday, sunday

void setup() {
  Serial.begin(115200);  // Initialize serial communications with the PC
  SPI.begin();         // Init SPI bus
  mfrc522.PCD_Init();  // Init MFRC522 card
  matrix.begin(0x70);

  pinMode(SwitchPin, INPUT);
  timeNow = millis(); 
  timeLast = millis();

  pinMode(piezoPin, OUTPUT);

   EEPROM.begin(256);

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  int c = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    ShowCon(c);
    c++;
    if (c>5) { c= 0;}
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());  

  Serial.println("Starting UDP fo time requests (NTP)");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());
  
  ShowDay();
  CheckDay();
       
  timeLeft = GetLastTime();
  lastMinuteStored = timeLeft;
      
  Serial.print("Staring with time: ");
  Serial.println(timeLeft);

  Serial.println("EEprom debug:");
  int d = 0;
  for (int x=0; x<5; x++){
         d = int(EEPROM.read(x));
         long T = EEPROM.read(1);
         Serial.println(T);  
         Serial.println(d);   
       }
  Serial.println("");

   for (int x=0; x<TotalCards; x++){
          Serial.println(EEPROM.read(100+x));
    
    }

  Serial.println("");
  Serial.println("Starting...");
  
 }

void CheckDay(){
    byte day = GetDay();

    LastDay = EEPROM.read(0);
    if (LastDay > 0 && LastDay < 8){
       if (LastDay != day) {
        ResetDay(day);
       }
       Serial.println("Resuming on the same day");
     }
     else
     {
      Serial.println("First time starting");
      ResetDay(day);
     }
}

long GetLastTime(){
   byte buf[4];  
   long LastTime = 0;
     // Check is new use
     Serial.print("Reading excisting time from EEprom: ");
     EEPROM.get(1,LastTime);
     Serial.println(LastTime);
  return LastTime;
}

void SaveLastTime(long LeftSeconds){
    
    if (lastMinuteStored != LeftSeconds){
      Serial.print("Saving time in EEPROM: ");
      Serial.println(LeftSeconds);
      EEPROM.put(1,LeftSeconds);
      EEPROM.commit();
      lastMinuteStored = LeftSeconds;
    }
    else {
      Serial.println("skipping Saving time in EEPROM, already done this minute ");
    }
}

void ResetDay(byte day){
    Serial.println("Resetting day..");
    EEPROM.put(0,day);
    EEPROM.commit();
    LastDay = day;
    timeLeft = Days[day];
    SaveLastTime(timeLeft);
}

unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

int GetDay(){
  WiFi.hostByName(ntpServerName, timeServerIP); 
  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  Serial.println("Getting Time");
  int cb = udp.parsePacket();
  while (!cb){
    if (!cb) {
      Serial.println("no time packet yet");
      delay(100);
      cb = udp.parsePacket();
    }
  }
  
    Serial.print("packet received, length=");
    Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);

    // Calculate Day of week
    byte day =  int(((epoch / 86400) + 4) % 7);
    Serial.print("Day of week: ");
    Serial.println(day);

    // print the hour, minute and second:
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    Serial.print(':');
    if ( ((epoch % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    Serial.print(':');
    if ( (epoch % 60) < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(epoch % 60); // print the second
  
    return day;  
  
}



void ShowCountDown(int seconds){

      int totalminutes = int(seconds / 60);
      int leftseconds = (seconds - (totalminutes*60));
      int leftminutes = (totalminutes % 60);
      int lefthours = (totalminutes / 60);

      if ((seconds % 2) == 0){
        matrix.drawColon(true);  
      }
      else {
        matrix.drawColon(false);
      }

     if (!CountDown){
        matrix.drawColon(true);  
     }
      
     if (seconds >= 3600) {  // If time left is more then 1 hour
      matrix.writeDigitNum(0, int(lefthours /10));
      matrix.writeDigitNum(1, (lefthours % 10));
      matrix.writeDigitNum(3, int(leftminutes /10));
      matrix.writeDigitNum(4, (leftminutes % 10));
      
     }
     else {
      matrix.writeDigitNum(0, int(leftminutes /10));
      matrix.writeDigitNum(1, (leftminutes % 10));
      matrix.writeDigitNum(3, int(leftseconds /10));
      matrix.writeDigitNum(4, (leftseconds % 10));
     }

     matrix.writeDisplay();
}


void ShowEnd() {
    matrix.writeDigitRaw(0,0);
    matrix.writeDigitRaw(1,B01111001);
    matrix.writeDigitRaw(3,B01010100);
    matrix.writeDigitRaw(4,B01011110);
    matrix.drawColon(false);
    matrix.writeDisplay();
}

void ShowUSED() {
    matrix.writeDigitRaw(0,B00111110); //U 00111110
    matrix.writeDigitRaw(1,B01101101); //S 01101101
    matrix.writeDigitRaw(3,B01111001); //E
    matrix.writeDigitRaw(4,B00111111); //D 00111111
    matrix.drawColon(false);
    matrix.writeDisplay();
}

void ShowCon(int w) {
    matrix.writeDigitRaw(0,B00111001); // C
    matrix.writeDigitRaw(1,B01011100); // O
    matrix.writeDigitRaw(3,B01010100); // N
    int s = 0;
    bitSet(s,w);    
    matrix.writeDigitRaw(4,s);
    matrix.drawColon(false);
    matrix.writeDisplay();
}

void ShowDay() {
    matrix.writeDigitRaw(0,B00111111); // D
    matrix.writeDigitRaw(1,B01110111); // A
    matrix.writeDigitRaw(3,B01100110); // Y
    matrix.writeDigitRaw(4,0);
    matrix.drawColon(false);
    matrix.writeDisplay();
}

void soundFX(float amplitude,float period){ 
 int uDelay=2+amplitude+amplitude*sin(millis()/period);
 for(int i=0;i<5;i++){
   digitalWrite(piezoPin,HIGH);
   delayMicroseconds(uDelay);
   digitalWrite(piezoPin,LOW);
   delayMicroseconds(uDelay);
 }
}


void NotifyIFTTT(String Message){
  if (CountDown) {  // only send messenges when iPad is not present

  String data = "{  \"value1\" : \"" + Message + "\"}";   
  String url = "http://maker.ifttt.com/trigger/ipad/with/key/c8rdhV9bsabMNcSVfyYCmNbGOGMTbgooabtGw6Ga_ur";

  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.POST(data);
  http.writeToStream(&Serial);
  http.end();
  }
  
  
}


int CheckCard(byte b1, byte b2, byte b3, byte b4){

int r = 0;

for (int x=0; x< TotalCards; x++) {
  if (Cards[x][0] == b1 && Cards[x][1] == b2 && Cards[x][2] == b3 && Cards[x][3]){
     Serial.println("Card is known");

     if (Cards[x][4] < 250){
       byte cardused = EEPROM.read(100+x);
       if (cardused == 0 || cardused == 255){
        Serial.println("Card is not used, adding time");
        
           r = Cards[x][4];
           EEPROM.write(100+x,1);
           EEPROM.commit();
           
           
       }
       else
       {
         Serial.println("Card is already used");
         ShowUSED();
       }
     }
     else
     {
      // Card is 254 = Resetting used status to unused
        if (Cards[x][4] == 254) {
        for (int x=0; x<TotalCards; x++){
           EEPROM.write(100+x,0);
        }
         EEPROM.commit();
       }
       // Card is 253 = Resetting day
       if (Cards[x][4] == 253) { 
         byte day = GetDay();
         ResetDay(day);
       }
      

     }
  }
  else
  {
      Serial.println("Card is unknown!!!");    
  }

  
 }

  return r;
}


void loop() {

if (timeLeft > 0) {
  ShowCountDown(timeLeft);
  }
  else
  {
    ShowEnd();
   
  }

 if (digitalRead(SwitchPin) == 0) {
    CountDown = true;
 }
 else {
     CountDown = false;
 }
 
 if ( (int(millis() / 1000) % 3600) == 0){ // Check for new day every hour
    CheckDay();
  }

  if (CountDown && timeLeft > 0) {
   timeNow = millis(); 
   if ((timeLast + 1000) < timeNow) {
      timeLeft = timeLeft -1;
      timeLast = timeLast + 1000;
      Serial.print("timeleft: ");
      Serial.println(timeLeft);
   }
  if (timeLeft == 1) {
    ShowCountDown(timeLeft);
    NotifyIFTTT("Time is up! Please return your iPad");
    for (int s=0; s<16; s++){
    soundFX(3000.0,30);
    delay(30); 
    }
    timeLeft = 0;
    ShowCountDown(timeLeft);
  }

  if ((timeLeft % 60) == 0) {  // Store every minute, the last minute in EEPROM
     SaveLastTime(timeLeft);
  }

   if ((timeLeft % 3600) == 0 && timeLeft > 1) {  // Exectute every full hour, expect 0 hour
    String hoursleft = String(int(timeLeft / 3600));
    if (int(timeLeft / 3600) == 1 ) {
      NotifyIFTTT("You have 1 hour left today on your iPad");
    }
    else
    {
      String msg = "You have " + hoursleft + " hours left today on your iPad";
      NotifyIFTTT(msg);
    }
    ShowCountDown(timeLeft);
    delay(1000); // wait one second, else multiple messages will be send in one second
    timeLeft = timeLeft -1;
  }


  if (timeLeft == 900) {  // Last 15 minutes notification
   NotifyIFTTT("You have 15 Minutes left today on your iPad");
   ShowCountDown(timeLeft);
   delay(1000); // wait one second, else multiple messages will be send in one second
   timeLeft = timeLeft -1;
  }

  }
  else {
    timeLast = millis();
   }
   
  
  
  // Look for new cards, and select one if present
  if ( mfrc522.PICC_IsNewCardPresent() & mfrc522.PICC_ReadCardSerial() ) {
  
    Serial.print(F("Card UID:"));
    sumids = 0;
    if (mfrc522.uid.size == 4) {
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(mfrc522.uid.uidByte[i], HEX);
        sumids = sumids + mfrc522.uid.uidByte[i];
      } 
      int r = CheckCard(mfrc522.uid.uidByte[0], mfrc522.uid.uidByte[1], mfrc522.uid.uidByte[2], mfrc522.uid.uidByte[3]);
      Serial.println(r);
      if (r > 0) {  // If card is accepted, beep once
      timeLeft = timeLeft + (r * 60);
      NotifyIFTTT("You have been rewarded " + String(r) + " minutes extra time!");
      soundFX(3000.0,30);
      delay(1000);
      }
      else   // if card is already used, beep 3x
      {
        for (int s=0; s < 3; s++){
          soundFX(3000.0,30);
          delay(30);
        }
        delay(1000);
      }
    
      matrix.println(sumids, DEC);
      matrix.writeDisplay();
    }
   
    }

}
