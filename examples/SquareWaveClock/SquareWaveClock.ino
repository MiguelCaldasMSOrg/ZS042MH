#include <ZS042MH.h>

const uint16_t CLOCK_RATE_HZ = 1;

ZS042MH zs042mh;

void setup() {
  Serial.begin(9600);
  zs042mh.begin();

  if (!zs042mh.rtcConnected()) {
    Serial.print("DS3231 connection failed, error ");
    Serial.println((int)zs042mh.lastError());
    return;
  }
  if (!zs042mh.setSquareWave(CLOCK_RATE_HZ)) {
    Serial.print("Could not configure square-wave output, error ");
    Serial.println((int)zs042mh.lastError());
    return;
  }

  Serial.println("Clock output enabled on INT/SQW");
}

void loop() {
}
