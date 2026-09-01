#include <ZS042MH.h>

const uint8_t ALARM_PIN = 2;

ZS042MH zs042mh;
volatile bool alarmPending = false;

void onAlarm() {
  alarmPending = true;
}

void setup() {
  Serial.begin(9600);
  zs042mh.begin();
  pinMode(ALARM_PIN, INPUT_PULLUP);

  if (!zs042mh.rtcConnected()) {
    Serial.println("DS3231 not found at 0x68");
    return;
  }
  if (!zs042mh.setAlarm1(ZS042MH_A1_EVERY_SECOND, 1, 0, 0, 0)) {
    Serial.println("Could not configure Alarm 1");
    return;
  }
  if (!zs042mh.setAlarmInterruptMode()) {
    Serial.println("Could not enable alarm interrupt output");
    return;
  }

  // Other schedules include minute/second, daily time, monthly date, and weekly weekday matches.
  // zs042mh.setAlarm1(ZS042MH_A1_MATCH_MINUTE_SECOND, 0, 0, 15, 30);
  // zs042mh.setAlarm2(ZS042MH_A2_MATCH_WEEKDAY_HOUR_MINUTE, 1, 9, 0);

  attachInterrupt(digitalPinToInterrupt(ALARM_PIN), onAlarm, FALLING);
  alarmPending = digitalRead(ALARM_PIN) == LOW;
}

void loop() {
  if (!alarmPending) {
    return;
  }

  noInterrupts();
  alarmPending = false;
  interrupts();

  uint8_t fired;
  if (!zs042mh.checkAndClearAlarms(fired)) {
    Serial.println("Could not read alarm status");
    alarmPending = true;
    return;
  }
  if (fired & ZS042MH::ALARM_1) {
    Serial.println("Alarm 1 fired");
  }
  if (fired & ZS042MH::ALARM_2) {
    Serial.println("Alarm 2 fired");
  }
}