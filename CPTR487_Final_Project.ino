/*
 *  HTTP over TLS (HTTPS) example sketch
 *
 *  This example demonstrates how to use
 *  WiFiClientSecure class to access HTTPS API.
 *  We fetch and display the status of
 *  esp8266/Arduino project continuous integration
 *  build.
 *
 *  Created by Ivan Grokhotkov, 2015.
 *  This example is in public domain.
 *
 *  Edited by Jordan Davis and Eric Brasuell, May 3 2020
 *  Updated it to inclued support for ESP32 and the U8g2 screen library
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <U8g2lib.h>
#define BUTTON                    18  //Pin for button
#define LED                       25  //Pin for board LED
#define FONT_ONE_HEIGHT           8
#define FONT_TWO_HEIGHT           20
char      chBuffer[128];

U8G2_SSD1306_128X64_NONAME_F_HW_I2C       u8g2(U8G2_R0, 16, 15, 4);

const char* chSSID = "WIFI SSID"; //Insert WIFI SSID here
const char* password = "WIFI password"; //Insert WIFI password here

const char* host = "api.lifx.com";
const int httpsPort = 443;

int usleep(useconds_t usec); 

// Use web browser to view and copy
// SHA1 fingerprint of the certificate
bool buttonPressed;
void setup() {
  //Serial baud rate
  Serial.begin(115200);

  //setup for Ledc Code
  ledcSetup(0, 1E5, 12);
  ledcAttachPin(26, 0); // Change 25 to 26 as required
  ledcWrite(26, 25);

  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, 1);

  //Setup for u8g2 screen
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
  
  buttonPressed = false;
  //write to screen that it is attempting to connect to SSID
  u8g2.clearBuffer();
  sprintf(chBuffer, "%s", "Connecting to:");
  u8g2.drawStr(64 - (u8g2.getStrWidth(chBuffer) / 2), 0, chBuffer);
  sprintf(chBuffer, "%s", chSSID);
  u8g2.drawStr(64 - (u8g2.getStrWidth(chBuffer) / 2), 31 - (FONT_ONE_HEIGHT / 2), chBuffer);
  u8g2.sendBuffer();
  
  Serial.begin(115200);
  Serial.println();
  Serial.print("connecting to ");
  Serial.println(chSSID);
  WiFi.begin(chSSID, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  digitalWrite(LED, 0);
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  char  chIp[81];

  //write ip address to screen (proves if connected)
  u8g2.clearBuffer();
  WiFi.localIP().toString().toCharArray(chIp, sizeof(chIp) - 1);
  sprintf(chBuffer, "IP  : %s", chIp);
  u8g2.drawStr(0, FONT_ONE_HEIGHT * 2, chBuffer);
  u8g2.sendBuffer();

  //write to screen the SSID you are connected to 
  u8g2.clearBuffer();
  delay(2000);
  sprintf(chBuffer, "%s", "Connected to:");
  u8g2.drawStr(64 - (u8g2.getStrWidth(chBuffer) / 2), 0, chBuffer);
  sprintf(chBuffer, "%s", chSSID);
  u8g2.drawStr(64 - (u8g2.getStrWidth(chBuffer) / 2), 31 - (FONT_ONE_HEIGHT / 2), chBuffer);
  u8g2.sendBuffer();
  
  attachInterrupt(digitalPinToInterrupt(BUTTON), button, FALLING);
}


// Requests Light status from Lifx API & displays light status on screen (on/off/Error *Error only prints in Serial Monitor*)
void getLightStatus(){
  WiFiClientSecure client;
  Serial.print("connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
      Serial.println("connection failed");
      return;
  }
  //https://api.developer.lifx.com/docs/list-lights
  String url = "/v1/lights/all";
  Serial.print("requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + "?duration=2 HTTP/1.1\r\n" +
              "Host: " + host + "\r\n" +
              "User-Agent: BuildFailureDetectorESP8266\r\n" +
              //Authorization: Bearer [device token]\r\n
              "Authorization: Bearer [device token]\r\n" + //Add your device token here
              "Connection: close\r\n\r\n");

  Serial.println("request sent");
  while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
      Serial.println("headers received");
      break;
  }
  }

  String line;
  String chunk;
  chunk = client.readStringUntil('\n');
  while(chunk.length() > 0){
    line += chunk;
    chunk = client.readStringUntil('\n');
  }
  Serial.println(chunk);
  int statusIndex = line.indexOf("\"power\": \""); //look for "power" header in reply

  //print light status to screen
  if(line.charAt(statusIndex + 11) == 'f') {    // Look for the twelfth character of the "power" header in reply. If letter is "f", write to screen
    u8g2.clearBuffer();
    sprintf(chBuffer, "%s", "Light is off");
    u8g2.drawStr(64 - (u8g2.getStrWidth(chBuffer) / 2), 31 - (FONT_ONE_HEIGHT / 2), chBuffer);
    u8g2.sendBuffer();

  } else if(line.charAt(statusIndex + 11) == 'n') {   // if letter is "n", write to screen
    u8g2.clearBuffer();
    sprintf(chBuffer, "%s", "Light is on");
    u8g2.drawStr(64 - (u8g2.getStrWidth(chBuffer) / 2), 31 - (FONT_ONE_HEIGHT / 2), chBuffer);
    u8g2.sendBuffer();
  
  } else {    //write errors in serial
    Serial.print("Error: Character is: ");
    Serial.println(line.charAt(statusIndex  + 11));

  }
}

//When button is pressed, print to serial
//"u8g2" code is screen 

void button(){
  buttonPressed=true;
  Serial.println("Button has been pressed"); 
}




/*This function has everything involved with the lifx api.
  It is only designed to toggle the light at the moment*/
void toggleLight(){
  WiFiClientSecure client;
  Serial.print("connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
      Serial.println("connection failed");
      return;
  }

  //https://api.developer.lifx.com/docs/toggle-power
  String url = "/v1/lights/all/toggle";
  Serial.print("requesting URL: ");
  Serial.println(url);

  client.print(String("POST ") + url + "?duration=2 HTTP/1.1\r\n" +
              "Host: " + host + "\r\n" +
              "User-Agent: BuildFailureDetectorESP8266\r\n" +
              //Authorization: Bearer [device token]\r\n
              "Authorization: Bearer [device token]\r\n" + //Add your device token here
              "Connection: close\r\n\r\n");

  Serial.println("request sent");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
    Serial.println("headers received");
    break;
    }
  }
  

  String line;
  String chunk;
  chunk = client.readStringUntil('\n');
  while(chunk.length() > 0){
    line += chunk;
    chunk = client.readStringUntil('\n');
  }
  int statusIndex = line.indexOf("\"power\": \"");

  //print light status to serial
  if(line.charAt(statusIndex + 11) == 'f') {    // light is off
    Serial.println("Light is off");
  } else if(line.charAt(statusIndex + 11) == 'n'){
    Serial.println("Light is on");
  } else{
    Serial.print("Error: Character is: ");
    Serial.println(line.charAt(statusIndex  + 11));
  }
  


  //write reply headers to serial
  Serial.println("reply was:");
  Serial.println("==========");
  Serial.println(line);
  Serial.println("==========");
  Serial.println("closing connection");
}


void megalovania(){

  //write "Playing Music" to screen while tones are being played
  u8g2.clearBuffer();
  sprintf(chBuffer, "%s", "Playing Music"); 
  u8g2.drawStr(64 - (u8g2.getStrWidth(chBuffer) / 2), 31 - (FONT_ONE_HEIGHT / 2), chBuffer);
  u8g2.sendBuffer();

  //ledcWriteTone(buzzChannel, frequency);

  ledcWriteTone(0, 294);//D4
  delay(125);
  usleep(125);
  ledcWriteTone(0, 294);//D4
  delay(125);
  usleep(125);
  ledcWriteTone(0, 587);//D5
  delay(250);
  usleep(250);
  ledcWriteTone(0, 440);//A4
  delay(250);
  usleep(375);
  ledcWriteTone(0, 415);//Ab4
  delay(125);
  usleep(250);
  ledcWriteTone(0, 392);//G4
  delay(250);
  usleep(250);
  ledcWriteTone(0, 349);//F4
  delay(250);
  usleep(250);
  ledcWriteTone(0, 294);//D4
  delay(125);
  usleep(125);
  ledcWriteTone(0, 349);//F4
  delay(125);
  usleep(125);
  ledcWriteTone(0, 392);//G4
  delay(125);
  usleep(125);
  ledcWriteTone(0, 261);//C4(middle)
  delay(125);
  usleep(62);
  ledcWriteTone(0, 261);//C4(middle)     
  delay(125);
  usleep(62);
  ledcWriteTone(0, 261);//C4(middle)     
  delay(150);
  usleep(62);
  ledcWriteTone(0, 261);//C4(middle)     
  delay(150);
  usleep(62);
  ledcWriteTone(0, 587);//D5
  delay(250);
  usleep(250);
  ledcWriteTone(0, 440);//A4
  delay(375);
  usleep(375);
  ledcWriteTone(0, 415);//Ab4
  delay(150);
  usleep(250);
  ledcWriteTone(0, 392);//G4
  delay(250);
  usleep(250);
  ledcWriteTone(0, 349);//F4
  delay(250);
  usleep(250);
  ledcWriteTone(0, 294);//D4
  delay(125);
  usleep(125);
  ledcWriteTone(0, 349);//F4
  delay(125);
  usleep(125);
  ledcWriteTone(0, 392);//G4
  delay(125);
  usleep(125);
  ledcWriteTone(0, 247);//B3
  delay(125);
  usleep(125);
  ledcWriteTone(0, 247);//B3
  delay(125);
  usleep(125);
  ledcWriteTone(0, 587);//D5
  delay(250);
  usleep(250);
  ledcWriteTone(0, 440);//A4
  delay(375);
  usleep(375);
  ledcWriteTone(0, 415);//Ab4
  delay(125);
  usleep(250);
  ledcWriteTone(0, 392);//G4
  delay(250);
  usleep(250);
  ledcWriteTone(0, 349);//F4
  delay(250);
  usleep(250);
  ledcWriteTone(0, 294);//D4
  delay(125);
  usleep(125);
  ledcWriteTone(0, 349);//F4
  delay(125);
  usleep(125);
  ledcWriteTone(0, 392);//G4
  delay(125);
  usleep(125);
  ledcWriteTone(0, 233);//Bb3
  delay(62);
  usleep(62);
  ledcWriteTone(0, 233);//Bb3
  delay(62);
  usleep(62);
  ledcWriteTone(0, 233);//Bb3
  delay(62);
  usleep(62);
  ledcWriteTone(0, 233);//Bb3
  delay(62);
  usleep(62);
  ledcWriteTone(0, 587);//D5
  delay(250);
  usleep(250);
  ledcWriteTone(0, 440);//A4
  delay(375);
  usleep(375);
  ledcWriteTone(0, 415);//Ab4
  delay(125);
  usleep(250);
  ledcWriteTone(0, 0);//last played tone hangs and keeps playing, so this tells it to stop playing tone
}



void loop() {
  getLightStatus();
  if(buttonPressed){
    digitalWrite(LED, 1); //turn on board LED if button is pressed

    //write button pressed to screen
    u8g2.clearBuffer();
    sprintf(chBuffer, "%s", "Button Pressed");
    u8g2.drawStr(64 - (u8g2.getStrWidth(chBuffer) / 2), 0, chBuffer);
    sprintf(chBuffer, "%s", "Toggling Light");
    u8g2.drawStr(64 - (u8g2.getStrWidth(chBuffer) / 2), 31 - (FONT_ONE_HEIGHT / 2), chBuffer);
    u8g2.sendBuffer();

    //toggle light
    toggleLight();
    delay(3000);
    //play music
    megalovania();

    //turn off board LED when button is not pressed
    buttonPressed=false;
    digitalWrite(LED, 0);
  }
}
