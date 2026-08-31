#include <ZS042MH.h>

ZS042MH module;

void setup() {
  Serial.begin(9600);
  module.begin();

  if (!module.eepromConnected()) {
    Serial.println("AT24C32 not found at 0x57");
    return;
  }

  const uint8_t message[] = "Hello, EEPROM!";
  uint8_t result[sizeof(message)];
  if (!module.eepromWrite(0, message, sizeof(message))) {
    Serial.println("EEPROM write failed");
    return;
  }
  if (!module.eepromRead(0, result, sizeof(result))) {
    Serial.println("EEPROM read failed");
    return;
  }

  Serial.print("Read back: ");
  Serial.println((char *)result);
}

void loop() {
}