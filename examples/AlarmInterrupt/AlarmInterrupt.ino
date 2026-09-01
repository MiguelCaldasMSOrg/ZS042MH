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
    Serial.print("DS3231 connection failed, error ");
    Serial.println((int)zs042mh.lastError());
    return;
  }
  if (!zs042mh.setAlarm1(ZS042MHAlarm1Mode::EverySecond, 1, 0, 0, 0)) {
    Serial.print("Could not configure Alarm 1, error ");
    Serial.println((int)zs042mh.lastError());
    return;
  }
  if (!zs042mh.setAlarmInterruptMode()) {
    Serial.print("Could not enable alarm interrupt output, error ");
    Serial.println((int)zs042mh.lastError());
    return;
  }

  // Other schedules include minute/second, daily time, monthly date, and weekly weekday matches.
  // zs042mh.setAlarm1(ZS042MHAlarm1Mode::MatchMinuteSecond, 0, 0, 15, 30);
  // zs042mh.setAlarm2(ZS042MHAlarm2Mode::MatchWeekdayHourMinute, 1, 9, 0);

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
    Serial.print("Could not read alarm status, error ");
    Serial.println((int)zs042mh.lastError());
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