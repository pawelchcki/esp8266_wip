#include <ThinPrometheus.h>
#include "ec_esp8266_hal.h"
#include <DallasTemperature.h>
#include <OneWire.h>

typedef ECEsp8266Hal ECHal;

class TemperatureSource {
    // Maxim/Dallas temperature ICs)
    OneWire oneWire;
    
    // Pass our oneWire reference to Dallas Temperature.
    DallasTemperature sensors;
    DeviceAddress insideThermometer;

    void setupDallas() {
        this->sensors.begin();
        this->sensors.getAddress(insideThermometer, 0);
    };
public:
    TemperatureSource(uint8_t oneWirePin) : oneWire(oneWirePin), sensors(&oneWire)  {

    };

    float getTemperature() {
        this->sensors.requestTemperatures();
        return this->sensors.getTempC(insideThermometer);
    };
};

class EC {    
    const uint8_t ecPin;
    const uint8_t capacitorNegativePin;
    const uint8_t capacitorPositivePin;

    float K  =1.1;
    float TemperatureCoef = 1.1;
public:
    EC(uint8_t ecPin, uint8_t capacitorNegativePin, uint8_t capacitorPositivePin) : ecPin(ecPin), capacitorNegativePin(capacitorNegativePin), capacitorPositivePin(capacitorPositivePin) {}

    void setup(){
        ECHal::setupPin(ecPin);
        ECHal::setupPin(capacitorNegativePin);
        ECHal::setupPin(capacitorPositivePin);
    };

    float getTemperature(){
        return 1.1;
    }

    String gaugeType(String) {return "";};
    String gauge(String, double){return "";};
    String gaugeLabels(String,double,String,String,String, String) {return "";};

    void process(){
        String res = "";
        std::vector<unsigned long> time0v, time1v;
      
        float tempStart = getTemperature();
      
        unsigned long start = micros();
      
        static int delayTime= 30;
        int maxTries = 1000;
      
        for (int i = 0; i < 17; i++) {
          ECHal::inputMode(ecPin);
          ECHal::outputWrite(capacitorPositivePin, HIGH);
          ECHal::outputWrite(capacitorNegativePin, LOW);
      
          int c=0;
          while (++c < maxTries && ECHal::digitalRead(ecPin) != HIGH) {
          }
      
          delayMicroseconds(delayTime);
      
          ECHal::inputMode(capacitorPositivePin);
      
          ECHal::outputWrite(ecPin, LOW);
          unsigned long time0 = 0;
          while (++time0 < maxTries && ECHal::digitalRead(capacitorPositivePin) == HIGH) {
          }
      
          ECHal::inputMode(ecPin);
          ECHal::outputWrite(capacitorPositivePin, HIGH);
          ECHal::outputWrite(capacitorNegativePin, HIGH);
      
          c=0;
      
      
          // second measurement
          delayMicroseconds(delayTime);
          
          ECHal::digitalWrite(capacitorPositivePin, LOW);
          delayMicroseconds(delayTime);
          
          c=0;
          // while (++c < 5000 && ECHal::digitalRead(ecPin) != HIGH) {
          // }
      
          ECHal::inputMode(capacitorPositivePin);
          ECHal::outputWrite(ecPin, HIGH);
      
          unsigned long time1 = 0;
      
          while (++time1 < maxTries && ECHal::digitalRead(capacitorPositivePin) != HIGH) {
          }
      
          ECHal::inputMode(ecPin);          
          ECHal::outputWrite(capacitorPositivePin, LOW);
          ECHal::outputWrite(capacitorNegativePin, LOW);
          
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
    }
};