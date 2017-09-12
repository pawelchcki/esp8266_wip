#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <Wire.h>
//for LED status
#include <Ticker.h>
Ticker ticker;

#define BUILTIN_LED 1

void tick()
{
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

//************************** End OF EC Function ***************************//

String gauge(const String &name, const float &value){
    String res = "# TYPE ";
    res += name;
    res += " gauge\n";
    res += name;
    res += " ";
    res += value;
    res += "\n";
    return res;
}
  
  String mcounter(const String &name, const float &value){
    
    String res = "# TYPE ";
    res += name;
    res += " counter\n";
    res += name;
    res += " ";
    res += value;
    res += "\n";
    return res;
  };


//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}


void setupOta(){
    // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

void setupWifiManager(){
    //set led pin as output
  pinMode(BUILTIN_LED, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
//  wifiManager.resetSettings();
// littlePonny

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }
}

void setupResetServer(){
  
}

volatile int interruptCounter =0 ;

void handleInterrupt() {
    interruptCounter++;
    if (interruptCounter % 2 == 0) {
        digitalWrite(BUILTIN_LED, LOW);
    } else {
        digitalWrite(BUILTIN_LED, HIGH);
    }
}
  
ESP8266WebServer server(80);

union {
  uint16_t integer;
  uint8_t bytes[2];
} tint;

void setupWire(){
  Wire.begin(2, 0);
  Wire.setClockStretchLimit(15000);  
  // delay(1000); 
  uint8_t cmd = 0b00010000;

  Wire.beginTransmission(121);
  Wire.write(cmd);
  Wire.endTransmission();  
}

void handleMetrics()
{
  digitalWrite(BUILTIN_LED, 1);
  String response = "";

  response += mcounter("agro_counter_raw", interruptCounter);
  uint8_t cmd = 0b00010000;
  // Serial.write(String(cmd, HEX));

  Wire.beginTransmission(121);
  Wire.write(cmd);
  Wire.endTransmission();  

  Wire.beginTransmission(121);
  Wire.write((0b0001 << 8) | 0);
  Wire.endTransmission();  

  int num = Wire.requestFrom(121, 2);

  if (num == 2){
    tint.bytes[0] = Wire.read();
    tint.bytes[1] = Wire.read();
    response += gauge("agro_gauge_raw0", tint.bytes[0]);
    response += gauge("agro_gauge_raw1", tint.bytes[1]);
    response += gauge("agro_gauge_raw", tint.integer);
  }


  response += "\n";
  
  server.send(200, "text/plain", response);
  
  
  digitalWrite(BUILTIN_LED, 0);
}

void setup() {
  Serial.begin(115200);
    //set led pin as output
  pinMode(BUILTIN_LED, OUTPUT);
  // pinMode(4, INPUT_PULLUP);
  // attachInterrupt(digitalPinToInterrupt(4), handleInterrupt, FALLING);

  Serial.println("Booting");
  setupWifiManager();
  setupOta();
  
 //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  ticker.detach();
  //keep LED on
  digitalWrite(BUILTIN_LED, LOW);

  Serial.println("Ready");
  Serial.print("IP address: ");
  
  Serial.println(WiFi.localIP());
  server.on("/metrics", handleMetrics);
  
  server.begin();
  setupWire();
}




void loop() {
  server.handleClient();    
  ArduinoOTA.handle();
}