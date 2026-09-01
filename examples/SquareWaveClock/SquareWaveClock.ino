#include <ZS042MH.h>

const uint16_t CLOCK_RATE_HZ = 1;

ZS042MH zs042mh;

void setup() {
  Serial.begin(9600);
  zs042mh.begin();

  if (!zs042mh.rtcConnected()) {
    Serial.println("DS3231 not found at 0x68");
    return;
  }
  if (!zs042mh.setSquareWave(CLOCK_RATE_HZ)) {
    Serial.println("Could not configure square-wave output");
    return;
  }

  Serial.println("Clock output enabled on INT/SQW");
}

void loop() {
}
