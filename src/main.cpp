#include <vector>

#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>

// needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>  //https://github.com/tzapu/WiFiManager

// #include <Wire.h>
// for LED status
#include <Ticker.h>
Ticker ticker;

#define BUILTIN_LED 10

void tick() {
  // toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

ADC_MODE(ADC_VCC);

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

String gaugeLabels(const String &name, const double &value,
                   const String &label0Name, const String &label0Value,
                   const String &label1Name, const String &label1Value) {
  return name + "{" + label0Name + "=\"" + label0Value + "\"," + label1Name +
         "=\"" + label1Value + "\"} " + value + "\n";
}

void outputWrite(uint8_t pin, uint8_t val) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, val);
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

String temperature() { return ""; }

float getTemperature() {
  sensors.requestTemperatures();
  return sensors.getTempC(insideThermometer);
}

// #define EC 3
// #define C_P 12
// #define C_M 14

#define EC 3
#define C_P 0
#define C_M 2

float TemperatureCoef =
    0.019;  // this changes depending on what chemical we are measuring
float K = 10.0;

String fancy_ec() {
  String res = "";
  std::vector<unsigned long> time0v, time1v;

  float tempStart = getTemperature();

  unsigned long start = micros();

  for (int i = 0; i < 16; i++) {
    pinMode(EC, INPUT);
    outputWrite(C_P, HIGH);
    outputWrite(C_M, LOW);

    delayMicroseconds(100);

    pinMode(C_P, INPUT);

    outputWrite(EC, LOW);
    unsigned long time0 = 0;
    while (++time0 < 5000 && digitalRead(C_P) == HIGH) {
    }

    pinMode(EC, INPUT);
    outputWrite(C_P, HIGH);
    outputWrite(C_M, HIGH);

    // second measurement
    delayMicroseconds(100);

    digitalWrite(C_P, LOW);
    delayMicroseconds(100);

    pinMode(C_P, INPUT);
    outputWrite(EC, HIGH);

    unsigned long time1 = 0;

    while (++time1 < 5000 && digitalRead(C_P) != HIGH) {
    }

    pinMode(EC, LOW);
    outputWrite(C_P, LOW);
    outputWrite(C_M, LOW);
    delayMicroseconds(100);

    time0v.push_back(time0);
    time1v.push_back(time1);
  }
  double timeElapsed = (micros() - start) / 1000000.0;
  res += gauge("agro_ec_elapsed_seconds", timeElapsed);
  res += gauge("agro_ec_frequency", 1.0 / (timeElapsed / 16.0));

  float tempC = (getTemperature() + tempStart) / 2.0;
  // tempC = 25;
  int cnt = 0;

  res += gaugeType("agro_resistance_raw");
  res += gaugeType("agro_ec");
  res += gaugeType("agro_ec25");

  for (auto time : time0v) {
    double ec = 1000 / (time * K);
    double ec25 = ec / (1 + TemperatureCoef * (tempC - 25.0));
    res +=
        gaugeLabels("agro_resistance_raw", time, "time", "0", "measurement",
                    String(cnt)) +
        gaugeLabels("agro_ec", ec, "time", "0", "measurement", String(cnt)) +
        gaugeLabels("agro_ec25", ec25, "time", "0", "measurement", String(cnt));
    cnt++;
  }

  cnt = 0;
  for (auto time : time1v) {
    double ec = 1000 / (time * K);
    double ec25 = ec / (1 + TemperatureCoef * (tempC - 25.0));
    res +=
        gaugeLabels("agro_resistance_raw", time, "time", "1", "measurement",
                    String(cnt)) +
        gaugeLabels("agro_ec", ec, "time", "1", "measurement", String(cnt)) +
        gaugeLabels("agro_ec25", ec25, "time", "1", "measurement", String(cnt));
    cnt++;
  }

  res += gauge("agro_ec_temp", tempC);

  return res;
}

String mcounter(const String &name, const float &value) {
  String res = "# TYPE ";
  res += name;
  res += " counter\n";
  res += name;
  res += " ";
  res += value;
  res += "\n";
  return res;
};

// gets called when WiFiManager enters configuration mode
void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  // if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  // entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

String infoEsp() {
  String res = "";
  res += gauge("esp_vcc", ESP.getVcc() / 1024.0);
  res += gauge("esp_free_heap", ESP.getFreeHeap());
  res += gauge("esp_chip_id", ESP.getChipId());
  res += gauge("esp_boot_version", ESP.getBootVersion());
  res += gauge("esp_boot_mode", ESP.getBootMode());
  res += gauge("esp_cpu_freq_mhz", ESP.getCpuFreqMHz());
  res += gauge("esp_flash_chip_id", ESP.getFlashChipId());
  res += gauge("esp_flash_chip_real_size", ESP.getFlashChipRealSize());
  res += gauge("esp_flash_chip_size", ESP.getFlashChipSize());
  res += gauge("esp_flash_chip_speed", ESP.getFlashChipSpeed());
  res +=
      gauge("esp_flash_chip_size_by_chip_id", ESP.getFlashChipSizeByChipId());
  res += gauge("esp_reset_reason", ESP.getResetInfoPtr()->reason);
  res += gauge("esp_sketch_size", ESP.getSketchSize());
  res += gauge("esp_sketch_free_space", ESP.getFreeSketchSpace());
  res += mcounter("esp_cycle_count", ESP.getCycleCount());

  return res;
}

void setupOta() {
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() { Serial.println("Start"); });
  ArduinoOTA.onEnd([]() { Serial.println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
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
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);

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
  String response = "";

  response += fancy_ec();
  response += temperature();
  response += infoEsp();

  response += "\n";

  server.send(200, "text/plain", response);
}

void oneWireSearchEndpoint() {
  server.send(200, "text/plain", searchOneWire());
}

void setup() {
  // Serial.begin(115200);
  // set led pin as output
  pinMode(BUILTIN_LED, OUTPUT);
  // pinMode(4, INPUT_PULLUP);
  // attachInterrupt(digitalPinToInterrupt(4), handleInterrupt, FALLING);

  Serial.println("Booting");
  setupWifiManager();
  setupOta();

  // if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  ticker.detach();
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