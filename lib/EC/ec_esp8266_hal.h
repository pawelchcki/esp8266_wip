#include <esp8266_peri.h>

struct ECEsp8266Hal {
  static int digitalRead(uint8_t pin){
    if(pin < 16){
      return GPIP(pin);
    } else if(pin == 16){
      return GP16I & 0x01;
    }
    return 0;
  }
  
  static void digitalWrite(uint8_t pin, uint8_t val){
    if(pin < 16){
      if(val) GPOS = (1 << pin);
      else GPOC = (1 << pin);
    } else if(pin == 16){
      if(val) GP16O |= 1;
      else GP16O &= ~1;
    }
  };
  
  static void outputMode(uint8_t pin){
    if(pin < 16){    
      GPF(pin) = GPFFS(GPFFS_GPIO(pin));//Set mode to GPIO
      GPC(pin) = (GPC(pin) & (0xF << GPCI)); //SOURCE(GPIO) | DRIVER(NORMAL) | INT_TYPE(UNCHANGED) | WAKEUP_ENABLE(DISABLED)
      GPES = (1 << pin); //Enable
    } else if(pin == 16){
      GP16E |= 1;    
    }
  };

  static void inputMode(uint8_t pin){
    pinMode(pin, INPUT);
  }

  static void outputWrite(uint8_t pin, uint8_t val) {
    ECEsp8266Hal::outputMode(pin);
    ECEsp8266Hal::digitalWrite(pin, val);
  };

  static void setupPin(uint8_t pin) {
    pinMode(pin, INPUT);
  }
};