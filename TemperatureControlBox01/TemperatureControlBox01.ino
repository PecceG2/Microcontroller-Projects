// Variables
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Ethernet.h>
#include <SPI.h>
#include <DHT.h>
#include <EEPROM.h>

/*
 * WARNING: ADMIN_HARDCODDED_PASSWORD MAY NOT CONTAIN THESE PROTECTED WORDS:
 * config, passwd, sampleTime, humPercentAlert, tempAlert, httpListenPort, or admin
 * If you use any of these words, unexpected behavior is expected. Must...Resist...Stupidity Impulse!
 */
#define ADMIN_HARDCODDED_PASSWORD "12345678"        // Administrator password

// Config variables ¡Default values!
struct {
  int sample_time = 2000;         // Time (in ms) between temperature measurements
  float humPercentAlert = 50;     // Percent of humidity for alarm  -- Datacenter recommended: 45-55%
  float tempAlert = 30;           // Temperature for alarm (in °C)
  int httpListenPort = 80;        // Listen port for HTTP web server
  String softversion = "1.2";     // Software version
  int firstStart = 0;             // Used for check eeprom, DO NOT CHANGE THIS VALUE
} Config;


// This is used for load/save eeprom, DO NOT CHANGE THIS VALUES
struct {
  int sample_time = 2000;         // Time (in ms) between temperature measurements
  float humPercentAlert = 50;     // Percent of humidity for alarm  -- Datacenter recommended: 45-55%
  float tempAlert = 30;           // Temperature for alarm (in °C)
  int httpListenPort = 80;        // Listen port for HTTP web server
  String softversion = "1.2";     // Software version
  int firstStart = 1;             // Used for check eeprom, DO NOT CHANGE THIS VALUE
} EEPROMConfig;


// Global variables
const int pinBuzzer = 9, resetPin = 8, dhtpin = 2, resetAlarmPin = 7;
bool alarmaActiva = false, dhcpMode = true;
byte mac[] = { 0x90, 0xA2, 0xDA, 0x58, 0xD4, 0xFF };
float hum = 0, temp = 0;
unsigned long millisLoopReadDHT = 0;
int addrEEPROM = 0;
String httpRequestBuffer;

// Global objects
EthernetServer server(Config.httpListenPort);
LiquidCrystal_I2C lcd(0x27,16,2);
DHT dht(dhtpin, DHT11);



/*
 *    MAIN SETUP
 */
void setup(){
  digitalWrite(resetPin, HIGH);
  initSoftware();
}
void initSoftware(){
  // Serial begin
  Serial.begin(9600);
  Serial.println("Soft version "+Config.softversion);

  // Pin definition
  pinMode(resetPin, OUTPUT);
  pinMode(resetAlarmPin, INPUT);

  // Load config
  loadEEPROM();

  // DHT begin
  dht.begin();

  // LCD begin
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Soft version 1.2");

  // Mode selector
  if(digitalRead(resetAlarmPin) == HIGH){
    lcd.setCursor(0, 1);
    lcd.print("Standalone mode");
    Serial.println("Standalone mode");
    dhcpMode = false;
    delay(3000);
  }else{
    lcd.setCursor(0, 1);
    lcd.print("Ethernet mode");
    Serial.println("Ethernet mode");
    delay(3000);
    beginDHCP();
    beginHTTPServer();
  }
  
  
}



/*
 *    MAIN LOOP
 */
void loop(){
  // Hook for http server
  if(dhcpMode){
    HTTPClientHook();
  }
  // Main check loop
  if(millis() - millisLoopReadDHT >= Config.sample_time){
    millisLoopReadDHT = millis();
    readDHT();
    if(temp >= Config.tempAlert){
      alarmaActiva = true;
    }
    if(hum >= Config.humPercentAlert){
      alarmaActiva = true;
    }
  }

  // Alarm sound and reset check
  alarma();
  if(digitalRead(resetAlarmPin) == HIGH){
    alarmaActiva = false;
  }
  
}


// Alarm beep function
void alarma(){
  if(alarmaActiva){
    tone(pinBuzzer, 50);
    delay(1000);
    noTone(pinBuzzer);
    delay(500);
    tone(pinBuzzer, 523, 300);
    delay(500);
  }

}




// Ethernet DHCP Function
void beginDHCP(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Waiting for DHCP");
  Serial.println("Starting network connection: DHCP");
  if (Ethernet.begin(mac) == 0){
    lcd.setCursor(0, 1);
    lcd.print("DHCP failed");
    Serial.println("Failed to configure Ethernet using DHCP");
    delay(10000);
    hardReset();
  }else{
    lcd.setCursor(0, 1);
    lcd.print("IP: "+Ethernet.localIP());
    Serial.print("DHCP client ip: ");
    Serial.println(Ethernet.localIP());
  }
}



// Hard reset function
void hardReset(){
  Serial.println("Hard reset called, restarting in 4 seconds");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Restarting...");
  delay(4000);
  digitalWrite(resetPin, LOW);
}


void readDHT(){
  hum = dht.readHumidity();
  temp = dht.readTemperature();

  lcd.clear();
  lcd.setCursor(0,0);
  
  // Comprobamos si ha habido algún error en la lectura
  if (isnan(hum) || isnan(temp)) {
    Serial.println("Error obteniendo los datos del sensor DHT11");
    lcd.print("Sensor error 12");
    lcd.setCursor(0,1);
    lcd.print("Check hardware");
  }else{
    Serial.print("Sensor read: Temperature: ");
    Serial.print(temp);
    Serial.print("°C - Humidity: ");
    Serial.print(hum);
    Serial.println(" %");

    lcd.print("Temp: ");
    lcd.print(temp);
    lcd.print(" ");
    lcd.print((char)223);
    lcd.print("C");
    lcd.setCursor(0,1);
    lcd.print("Hum: ");
    lcd.print(hum);
    lcd.print("%");
  }
  
}



/*
 *    EEPROM functions
 */
void clearEEPROM(){
  for (int i = 0 ; i < EEPROM.length() ; i++) {
    if(EEPROM.read(i) != 0){
      EEPROM.write(i, 0);
    }
  }
  Serial.println("EEPROM erased");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("EEPROM Cleared");
  lcd.setCursor(0,1);
  lcd.print("Restarting...");
  hardReset();
}

void loadEEPROM(){
  EEPROM.get(addrEEPROM, EEPROMConfig);
  if(EEPROMConfig.firstStart != 0){
    // is first start (eeprom empty)
    EEPROM.put(addrEEPROM,Config);
    EEPROM.end();
  }else{
    EEPROM.get(addrEEPROM, Config);
  }
}

void saveEEPROM(){
  EEPROM.put(addrEEPROM,Config);
  EEPROM.end();
}



/*
 *    HTTP Server
 */

// HTTP Server function
void beginHTTPServer(){
  server.begin();
}

void HTTPClientHook(){
  EthernetClient client = server.available();
  if(client){
    Serial.println("------------------------------------------");
    Serial.println("[HTTP Server] New request received");
    unsigned long startRequestTime = millis();
    boolean currentLineIsBlank = true;
    while(client.connected()){
      if(client.available()){
        
        char c = client.read();

        if (httpRequestBuffer.length() < 100) {
          //store characters to buffer
          httpRequestBuffer += c;
          Serial.print(c); //print what server receives to serial monitor
        } 
        
        // End of http request
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: application/json");
          client.println("Connection: close");
          client.println();
          
          ///////////////////// PATH FUNCTIONS ////////////////////
          if(httpRequestBuffer.indexOf("config") > 0){
            if(httpRequestBuffer.indexOf("config") > 0 && httpRequestBuffer.indexOf("passwd") > 0 && httpRequestBuffer.indexOf("sampleTime") > 0 && httpRequestBuffer.indexOf("humPercentAlert") > 0 && httpRequestBuffer.indexOf("tempAlert") > 0 && httpRequestBuffer.indexOf("httpListenPort") > 0){
              // Valid config params (¡¡VALIDATE ADMIN_HARDCODDED_PASSWORD!!)
              client.print("{error: 0, status: 'Config saved successfully to EEPROM.'}");
              Serial.println("[HTTP Server] Config received, saving on EEPROM...");
              
            }
          }else if(httpRequestBuffer.indexOf("admin") > 0){
            
            
          }else{
            // Index or any invalid path
            unsigned long requestTime = millis()- startRequestTime;
            client.print("{ 'temperature': '");
            client.print(temp);
            client.print("', 'humidity': '");
            client.print(hum);
            client.print("', 'requestTime': '");
            client.print(requestTime);
            client.println("' }");
          }

          Serial.println("[HTTP Server] Response sended to client");

          // clearing buffer
          httpRequestBuffer="";

          
          break;
        }
        if (c == '\n') {
          currentLineIsBlank = true;
        } else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }
    }

    delay(2);
    client.stop();
    Serial.println("[HTTP Server] Client disconnected");
    Serial.println("------------------------------------------");
  }
}
