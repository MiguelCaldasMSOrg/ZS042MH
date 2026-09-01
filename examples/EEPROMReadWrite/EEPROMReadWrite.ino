#include <ZS042MH.h>

ZS042MH zs042mh;

void setup() {
  Serial.begin(9600);
  zs042mh.begin();

  if (!zs042mh.eepromConnected()) {
    Serial.print("AT24C32 connection failed, error ");
    Serial.println((int)zs042mh.lastError());
    return;
  }

  const uint8_t message[] = "Hello, EEPROM!";
  uint8_t result[sizeof(message)];
  if (!zs042mh.eepromWrite(0, message, sizeof(message))) {
    Serial.print("EEPROM write failed, error ");
    Serial.println((int)zs042mh.lastError());
    return;
  }
  if (!zs042mh.eepromRead(0, result, sizeof(result))) {
    Serial.print("EEPROM read failed, error ");
    Serial.println((int)zs042mh.lastError());
    return;
  }

  Serial.print("Read back: ");
  Serial.println((char *)result);
}

void loop() {
}