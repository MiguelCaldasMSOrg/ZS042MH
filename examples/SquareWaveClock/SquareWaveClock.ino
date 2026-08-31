#include <ZS042MH.h>

const uint16_t CLOCK_RATE_HZ = 1;

ZS042MH module;

void setup() {
  Serial.begin(9600);
  module.begin();

  if (!module.rtcConnected()) {
    Serial.println("DS3231 not found at 0x68");
    return;
  }
  if (!module.setSquareWave(CLOCK_RATE_HZ)) {
    Serial.println("Could not configure square-wave output");
    return;
  }

  Serial.println("Clock output enabled on INT/SQW");
}

void loop() {
}
