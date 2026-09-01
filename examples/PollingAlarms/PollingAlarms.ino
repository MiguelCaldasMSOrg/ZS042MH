#include <ZS042MH.h>

ZS042MH zs042mh;

void setup() {
  Serial.begin(9600);
  zs042mh.begin();

  if (!zs042mh.rtcConnected()) {
    Serial.print("DS3231 connection failed, error ");
    Serial.println((int)zs042mh.lastError());
    return;
  }
  if (!zs042mh.setAlarm1(ZS042MH_A1_EVERY_SECOND, 0, 0, 0, 0)) {
    Serial.print("Could not configure Alarm 1, error ");
    Serial.println((int)zs042mh.lastError());
    return;
  }
  if (!zs042mh.setAlarm2(ZS042MH_A2_EVERY_MINUTE, 0, 0, 0)) {
    Serial.print("Could not configure Alarm 2, error ");
    Serial.println((int)zs042mh.lastError());
    return;
  }

  // No interrupt wire is required. Alarm flags can also be polled while INT/SQW provides a clock.
  // Call zs042mh.setSquareWave(1) here to output 1 Hz while continuing to poll the flags.
}

void loop() {
  uint8_t fired;
  if (!zs042mh.checkAndClearAlarms(fired)) {
    Serial.print("Could not read alarm status, error ");
    Serial.println((int)zs042mh.lastError());
    delay(1000);
    return;
  }
  if (fired & ZS042MH::ALARM_1) {
    Serial.println("Alarm 1 fired");
  }
  if (fired & ZS042MH::ALARM_2) {
    Serial.println("Alarm 2 fired");
  }
  delay(100);
}
