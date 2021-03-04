#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h >
#include <WiFiManager.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266HTTPClient.h>

#define RST_PIN 2
#define SS_PIN 4
#define RELAY1_PIN 5
#define RELAY2_PIN 16

MFRC522 reader(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
String url = "http://10.10.0.9/AccessControl01/index.php?key=";
String token = "YOUR_TOKEN_HERE";



// HTTP Client
HTTPClient http;
WiFiClient client;

// HTTP Server
ESP8266WebServer server(80);

void handleRoot(){
   server.send(200, "text/plain", "Usage: http://1.1.1.1/1/YOUR_TOKEN_HERE or http://1.1.1.1/2/YOUR_TOKEN_HERE");
}

void handleNotFound(){
   server.send(404, "text/plain", "404 error: Not found");
}


void setup(){
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, HIGH);
  digitalWrite(RELAY2_PIN, HIGH);
  Serial.begin(115200);


  // WiFi Manager
  WiFiManager wifiManager;
  //wifiManager.resetSettings();
  wifiManager.autoConnect("AccessControl01");
  Serial.println("Conectado con exito.");


  // SPI / RFID Reader
  SPI.begin();
  reader.PCD_Init();
  delay(4);
  for (byte i = 0; i < 6; i++){
    key.keyByte[i] = 0xFF; //keyByte is defined in the "MIFARE_Key" 'struct' definition in the .h file of the library
  }
  Serial.println("RFID Ready!");


  // HTTP WebServer
  server.on("/", handleRoot);
  server.on("/1/"+token, []() {
     server.send(200, "text/plain", "OK");
     openFirstDoor();
  });
  server.on("/2/"+token, []() {
     server.send(200, "text/plain", "OK");
     openSecondDoor();
  });
  server.onNotFound(handleNotFound);
 
  server.begin();
  Serial.println("HTTP Server Ready!");
  
}

void loop() {

  server.handleClient();
  
    // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if (!reader.PICC_IsNewCardPresent()){
    return;
  }

  // Select one of the cards. This returns false if read is not successful; and if that happens, we stop the code
  if (!reader.PICC_ReadCardSerial()){
    return;
  }

  // At this point, the serial can be read. We transform from byte to hex

  String serial = "";
  for (int x = 0; x < reader.uid.size; x++){
    // If it is less than 10, we add zero
    if (reader.uid.uidByte[x] < 0x10){
      serial += "0";
    }
    // Transform the byte to hex
    serial += String(reader.uid.uidByte[x], HEX);
    // Add a hypen
    if (x + 1 != reader.uid.size)
    {
      serial += "-";
    }
  }
  // Transform to uppercase
  serial.toUpperCase();

  Serial.println("Read serial is: " + serial);

  // Halt PICC
  reader.PICC_HaltA();
  // Stop encryption on PCD
  reader.PCD_StopCrypto1();
  

   if (http.begin(client, url+serial)) //Iniciar conexión
   {
      Serial.print("[HTTP] GET...\n");
      int httpCode = http.GET();  // Realizar petición
 
      if (httpCode > 0) {
         Serial.printf("[HTTP] GET... code: %d\n", httpCode);
 
         if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            String payload = http.getString();   // Obtener respuesta
            Serial.println(payload);   // Mostrar respuesta por serial
            if(payload == "true"){
              openFirstDoor();
            }
         }
      }
      else {
         Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
 
      http.end();
   }
   else {
      Serial.printf("[HTTP} Unable to connect\n");
   }
  

  
  }


void openFirstDoor(){
  Serial.println("Opening door 01");
  digitalWrite(RELAY1_PIN, LOW);
  delay(5000);
  digitalWrite(RELAY1_PIN, HIGH);
}

void openSecondDoor(){
  Serial.println("Opening door 02");
  digitalWrite(RELAY2_PIN, LOW);
  delay(300);
  digitalWrite(RELAY2_PIN, HIGH);
  delay(7000);
  digitalWrite(RELAY2_PIN, LOW);
  delay(300);
  digitalWrite(RELAY2_PIN, HIGH);
}
