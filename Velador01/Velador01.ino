#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h >
#include <WiFiManager.h>
#include <ESP8266HTTPClient.h>

#define RELAY1_PIN 0
#define BUTTON1_PIN 2
String token = "YOUR_TOKEN_HERE";

int prevStatus;


// HTTP Client
HTTPClient http;
WiFiClient client;

// HTTP Server
ESP8266WebServer server(80);

void handleRoot(){
   server.send(200, "text/plain", "Usage: http://1.1.1.1/0/YOUR_TOKEN_HERE (Turn off) or http://1.1.1.1/1/YOUR_TOKEN_HERE (Turn on)or http://1.1.1.1/2/YOUR_TOKEN_HERE (Toggle mode)");
}

void handleNotFound(){
   server.send(404, "text/plain", "404 error: Not found");
}


void setup(){
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(BUTTON1_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, LOW);
  Serial.begin(115200);


  // WiFi Manager
  WiFiManager wifiManager;
  //wifiManager.resetSettings();
  wifiManager.autoConnect("Velador01");
  Serial.println("Conectado con exito.");



  // HTTP WebServer
  server.on("/", handleRoot);
  server.on("/0/"+token, []() {
     server.send(200, "text/plain", "OK");
     turnOff();
  });
  server.on("/1/"+token, []() {
     server.send(200, "text/plain", "OK");
     turnOn();
  });
  server.on("/2/"+token, []() {
     server.send(200, "text/plain", "OK");
     toggle();
  });
  
  server.onNotFound(handleNotFound);
 
  server.begin();
  Serial.println("HTTP Server Ready!");
  prevStatus = digitalRead(BUTTON1_PIN);
}



void loop() {
  server.handleClient();
  
  int currentButtonStatus = digitalRead(BUTTON1_PIN);
  if(prevStatus != currentButtonStatus){
    prevStatus = currentButtonStatus;
    toggle();
  }
  
}

void turnOn(){
  digitalWrite(RELAY1_PIN, HIGH);
}

void turnOff(){
  digitalWrite(RELAY1_PIN, LOW);  
}

void toggle(){
  digitalWrite(RELAY1_PIN, !digitalRead(RELAY1_PIN));
}
