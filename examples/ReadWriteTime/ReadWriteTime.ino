#include <ZS042MH.h>

ZS042MH zs042mh;

void printTwoDigits(uint8_t value) {
  if (value < 10) {
    Serial.print('0');
  }
  Serial.print(value);
}

void setup() {
  Serial.begin(9600);
  zs042mh.begin();

  if (!zs042mh.rtcConnected()) {
    Serial.print("DS3231 connection failed, error ");
    Serial.println((int)zs042mh.lastError());
    return;
  }

  bool stopped;
  if (!zs042mh.oscillatorStopped(stopped)) {
    Serial.print("Could not read oscillator status, error ");
    Serial.println((int)zs042mh.lastError());
    return;
  }
  if (stopped) {
    Serial.println("RTC time may be invalid; set the clock before relying on it");
  }

  // Uncomment once to set the RTC, then comment it again and upload.
  // zs042mh.setTime(2026, 1, 31, 12, 0, 0);
}

void loop() {
  ZS042MHDateTime now;
  if (!zs042mh.getTime(now)) {
    Serial.print("RTC read failed, error ");
    Serial.println((int)zs042mh.lastError());
    delay(1000);
    return;
  }

  Serial.print(now.year);
  Serial.print('-');
  printTwoDigits(now.month);
  Serial.print('-');
  printTwoDigits(now.day);
  Serial.print(' ');
  printTwoDigits(now.hour);
  Serial.print(':');
  printTwoDigits(now.minute);
  Serial.print(':');
  printTwoDigits(now.second);
  Serial.print("  ");
  Serial.print(zs042mh.getTemperature());
  Serial.println(" C");
  delay(1000);
}