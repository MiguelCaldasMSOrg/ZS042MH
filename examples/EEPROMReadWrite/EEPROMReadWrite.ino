#include <ZS042MH.h>

ZS042MH zs042mh;

void setup() {
  Serial.begin(9600);
  zs042mh.begin();

  if (!zs042mh.eepromConnected()) {
    Serial.println("AT24C32 not found at 0x57");
    return;
  }

  const uint8_t message[] = "Hello, EEPROM!";
  uint8_t result[sizeof(message)];
  if (!zs042mh.eepromWrite(0, message, sizeof(message))) {
    Serial.println("EEPROM write failed");
    return;
  }
  if (!zs042mh.eepromRead(0, result, sizeof(result))) {
    Serial.println("EEPROM read failed");
    return;
  }

  Serial.print("Read back: ");
  Serial.println((char *)result);
}

void loop() {
}