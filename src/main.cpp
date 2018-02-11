#ifndef UNIT_TEST
#include <vector>
#include <string>

#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>

// needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>  //https://github.com/tzapu/WiFiManager

#include <ThinPrometheus.h>

// #include <Wire.h>
// for LED status

#define BUILTIN_LED 10

void tick() {
  // toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

ADC_MODE(ADC_VCC);

#include <ec.h>

//************************** End OF EC Function ***************************//

String gaugeType(const String &name) { return "# TYPE " + name + " gauge\n"; }

String gauge(const String &name, const double &value) {
  String res = gaugeType(name);
  res += name;
  res += " ";
  res += value;
  res += "\n";
  return res;
}

String gaugeLabels(const String &name, const double &value, const String &label0Name, const String &label0Value,
                   const String &label1Name, const String &label1Value) {
  return name + "{" + label0Name + "=\"" + label0Value + "\"," + label1Name + "=\"" + label1Value + "\"} " + value +
         "\n";
}

#include <esp8266_peri.h>
inline int fastDigitalRead(uint8_t pin){
  if(pin < 16){
    return GPIP(pin);
  } else if(pin == 16){
    return GP16I & 0x01;
  }
  return 0;
}

// #define fastDigitalRead(p) ((GPI & (1 << ((p) & 0xF))) != 0)

void fastDigitalWrite(uint8_t pin, uint8_t val){
  if(pin < 16){
    if(val) GPOS = (1 << pin);
    else GPOC = (1 << pin);
  } else if(pin == 16){
    if(val) GP16O |= 1;
    else GP16O &= ~1;
  }
}

void fastOutputMode(uint8_t pin){
  if(pin < 16){    
    GPF(pin) = GPFFS(GPFFS_GPIO(pin));//Set mode to GPIO
    GPC(pin) = (GPC(pin) & (0xF << GPCI)); //SOURCE(GPIO) | DRIVER(NORMAL) | INT_TYPE(UNCHANGED) | WAKEUP_ENABLE(DISABLED)
    GPES = (1 << pin); //Enable
  } else if(pin == 16){
    GP16E |= 1;    
  }
}

void fastInputMode(uint8_t pin){

}

void outputWrite(uint8_t pin, uint8_t val) {
  fastOutputMode(pin);
  fastDigitalWrite(pin, val);
}

#include <DallasTemperature.h>
#include <OneWire.h>

#define ONE_WIRE_BUS 1

// Setup a oneWire instance to communicate with any OneWire devices (not just
// Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);
DeviceAddress insideThermometer;

void setupDallas() {
  sensors.begin();
  sensors.getAddress(insideThermometer, 0);
}

float getTemperature() {
  sensors.requestTemperatures();
  return sensors.getTempC(insideThermometer);
}

#define EC 13
#define C_P 12
#define C_M 14

// #define EC 3
// #define C_P 0
// #define C_M 2

float TemperatureCoef = 0.019;  // this changes depending on what chemical we are measuring
float K = 10.0;

Registry Prometheus;

uint16_t waitForState(uint8_t pin, int state){
  uint16_t c = 0;
  while(++c < 5000 && fastDigitalRead(pin) == state) {}
  return c;
}

String fancy_ec() {
  String res = "";
  std::vector<unsigned long> time0v, time1v;

  float tempStart = getTemperature();

  unsigned long start = micros();

  static int delayTime= 30;
  int maxTries = 1000;

  for (int i = 0; i < 17; i++) {
    pinMode(EC, INPUT);
    outputWrite(C_P, HIGH);
    outputWrite(C_M, LOW);

    int c=0;
    while (++c < maxTries && fastDigitalRead(EC) != HIGH) {
    }

    delayMicroseconds(delayTime);

    pinMode(C_P, INPUT);

    outputWrite(EC, LOW);
    unsigned long time0 = 0;
    while (++time0 < maxTries && fastDigitalRead(C_P) == HIGH) {
    }

    pinMode(EC, INPUT);
    outputWrite(C_P, HIGH);
    outputWrite(C_M, HIGH);

    c=0;


    // second measurement
    delayMicroseconds(delayTime);
    
    digitalWrite(C_P, LOW);
    delayMicroseconds(delayTime);
    
    c=0;
    // while (++c < 5000 && fastDigitalRead(EC) != HIGH) {
    // }

    pinMode(C_P, INPUT);
    outputWrite(EC, HIGH);

    unsigned long time1 = 0;

    while (++time1 < maxTries && fastDigitalRead(C_P) != HIGH) {
    }

    pinMode(EC, LOW);
    outputWrite(C_P, LOW);
    outputWrite(C_M, LOW);
    
    delayMicroseconds(delayTime);
    if (i>0){
      time0v.push_back(time0);
      time1v.push_back(time1);
    }
  }
  double timeElapsed = (micros() - start) / 1000000.0;
  res += gauge("agro_ec_elapsed_seconds", timeElapsed);
  double freq =  1.0 / (timeElapsed / 16.0);
  // delayTime = freq
  res += gauge("agro_ec_frequency", freq);

  float tempC = (getTemperature() + tempStart) / 2.0;
  // tempC = 25;
  int cnt = 0;

  res += gaugeType("agro_resistance_raw");
  res += gaugeType("agro_ec");
  res += gaugeType("agro_ec25");

  for (auto time : time0v) {
    double ec = 1000 / (time * K);
    double ec25 = ec / (1 + TemperatureCoef * (tempC - 25.0));
    res += gaugeLabels("agro_resistance_raw", time, "time", "0", "measurement", String(cnt)) +
           gaugeLabels("agro_ec", ec, "time", "0", "measurement", String(cnt)) +
           gaugeLabels("agro_ec25", ec25, "time", "0", "measurement", String(cnt));
    cnt++;
  }

  cnt = 0;
  for (auto time : time1v) {
    double ec = 1000 / (time * K);
    double ec25 = ec / (1 + TemperatureCoef * (tempC - 25.0));
    res += gaugeLabels("agro_resistance_raw", time, "time", "1", "measurement", String(cnt)) +
           gaugeLabels("agro_ec", ec, "time", "1", "measurement", String(cnt)) +
           gaugeLabels("agro_ec25", ec25, "time", "1", "measurement", String(cnt));
    cnt++;
  }

  res += gauge("agro_ec_temp", tempC);

  return res;
}

// gets called when WiFiManager enters configuration mode
void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  // if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void setupOta() {
  ArduinoOTA.onStart([]() { Serial.println("Start"); });
  ArduinoOTA.onEnd([]() { Serial.println("\nEnd"); });
  ArduinoOTA.onProgress(
      [](unsigned int progress, unsigned int total) { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)
      Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

void setupWifiManager() {
  // set led pin as output
  pinMode(BUILTIN_LED, OUTPUT);

  // WiFiManager
  // Local intialization. Once its business is done, there is no need to
  // keep it
  // around
  WiFiManager wifiManager;
  // reset settings - for testing
  //  wifiManager.resetSettings();
  // littlePonny

  // set callback that gets called when connecting to previous WiFi fails,
  // and
  // enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  // fetches ssid and pass and tries to connect
  // if it does not connect it starts an access point with the specified
  // name
  // here  "AutoConnectAP"
  // and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    // reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }
}

void setupResetServer() {}

volatile int interruptCounter = 0;

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

void setupWire() {
  // // Wire.begin(2, 0);
  // Wire.begin();
  // Wire.setClockStretchLimit(15000);
  // // delay(1000);
  // uint8_t cmd = 0b00010000;

  // Wire.beginTransmission(121);
  // Wire.write(cmd);
  // Wire.endTransmission();
}

String searchOneWire() {
  byte i;
  byte present = 0;
  byte data[12];
  byte addr[8];

  String ret = "";
  ret += ("Looking for 1-Wire devices...\n\r");
  while (oneWire.search(addr)) {
    ret += ("\n\rFound \'1-Wire\' device with address:\n\r");
    for (i = 0; i < 8; i++) {
      ret += ("0x");
      if (addr[i] < 16) {
        ret += ('0');
      }
      Serial.print(addr[i], HEX);
      if (i < 7) {
        ret += (", ");
      }
    }
    if (OneWire::crc8(addr, 7) != addr[7]) {
      return "CRC is not valid!\n";
    }
  }
  ret += ("\n\r\n\rThat's it.\r\n");
  oneWire.reset_search();
  return ret;
}

void handleMetrics() {
  // String response = Prometheus.collectAndRepresent();

  // response += Prometheus.collectAndRepresent();
  // response += "\n"; // TODO: is this newline still needed ?

  server.send(200, "text/plain",  Prometheus.collectAndRepresent());
}

void oneWireSearchEndpoint() { server.send(200, "text/plain", searchOneWire()); }

#include <BME280I2C.h>
#include <SPI.h>

#include <Wire.h>

  BME280I2C::Settings settings(
    BME280::OSR_X1,
    BME280::OSR_X1,
    BME280::OSR_X1,
    BME280::Mode_Forced,
    BME280::StandbyTime_1000ms,
    BME280::Filter_Off,
    BME280::SpiEnable_False,
    0x76 // I2C address. I2C specific.
  );

  BME280I2C bme(settings);


void collectGbM(Registry& registry) {
  static auto &chipModel = registry.gauge("bme_chip_model", "Chip model");
  chipModel.set(bme.chipModel());
  float temp(NAN), hum(NAN), pres(NAN);

  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
  BME280::PresUnit presUnit(BME280::PresUnit_Pa);
  bme.read(pres, temp, hum, tempUnit, presUnit);

  static auto &temperature = registry.gauge("bme_temperature_celsius", "Temperature");
  temperature.set(temp);

  static auto &pressure = registry.gauge("bme_pressure_pascals", "Pressure");
  pressure.set(pres);

  static auto &humidity = registry.gauge("bme_humidity_relative", "Humidity");
  humidity.set(hum);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");

  Wire.begin(D3, D4);
  bme.begin();

  Prometheus.addCollector(CommonCollectors::collectEspInfo);
  Prometheus.addCollector(collectGbM);

  
  // set led pin as output
  pinMode(BUILTIN_LED, OUTPUT);
  // pinMode(4, INPUT_PULLUP);
  // attachInterrupt(digitalPinToInterrupt(4), handleInterrupt, FALLING);

  Serial.println("Booting");
  setupWifiManager();
  setupOta();

  // if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  // keep LED on
  digitalWrite(BUILTIN_LED, LOW);

  Serial.println("Ready");
  Serial.print("IP address: ");

  Serial.println(WiFi.localIP());
  server.on("/metrics", handleMetrics);
  server.on("/onewire", oneWireSearchEndpoint);

  server.begin();
  setupWire();
  setupDallas();
}

void loop() {
  server.handleClient();
  ArduinoOTA.handle();
}
#endif