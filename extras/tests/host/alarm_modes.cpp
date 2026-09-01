#include <ZS042MH.h>

#include <assert.h>
#include <initializer_list>
#include <iostream>
#include <vector>

TwoWire Wire;

uint32_t millis() {
  static uint32_t value = 0;
  return value++;
}

void delay(unsigned long milliseconds) {
  (void)milliseconds;
}

void resetWire() {
  Wire.transmissions.clear();
  for (size_t index = 0; index < sizeof(Wire.registers); index++) {
    Wire.registers[index] = 0;
  }
  Wire.registers[0x0E] = 0xA0;
  Wire.registers[0x0F] = 0x8B;
}

void expectFirstTransmission(std::initializer_list<uint8_t> expected) {
  assert(!Wire.transmissions.empty());
  assert(Wire.transmissions[0].address == ZS042MH::DEFAULT_RTC_ADDRESS);
  assert(Wire.transmissions[0].data == std::vector<uint8_t>(expected));
}

void expectAlarm1(ZS042MH &zs042mh, ZS042MHAlarm1Mode mode, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second, std::initializer_list<uint8_t> expected) {
  resetWire();
  assert(zs042mh.setAlarm1(mode, day, hour, minute, second));
  expectFirstTransmission(expected);
  assert(Wire.registers[0x0E] == 0xA1);
  assert(Wire.registers[0x0F] == 0x8A);
}

void expectAlarm2(ZS042MH &zs042mh, ZS042MHAlarm2Mode mode, uint8_t day, uint8_t hour, uint8_t minute, std::initializer_list<uint8_t> expected) {
  resetWire();
  assert(zs042mh.setAlarm2(mode, day, hour, minute));
  expectFirstTransmission(expected);
  assert(Wire.registers[0x0E] == 0xA2);
  assert(Wire.registers[0x0F] == 0x89);
}

int main() {
  ZS042MH zs042mh(Wire);

  resetWire();
  ZS042MH addressedRtcEeprom(0x69, 0x50);
  assert(addressedRtcEeprom.rtcConnected());
  assert(Wire.transmissions.back().address == 0x69);
  assert(addressedRtcEeprom.eepromConnected());
  assert(Wire.transmissions.back().address == 0x50);
  assert(addressedRtcEeprom.eepromSize() == ZS042MH::DEFAULT_EEPROM_SIZE);

  expectAlarm1(zs042mh, ZS042MH_A1_EVERY_SECOND, 255, 255, 255, 255, {0x07, 0x80, 0x80, 0x80, 0x80});
  expectAlarm1(zs042mh, ZS042MH_A1_MATCH_SECOND, 255, 255, 255, 45, {0x07, 0x45, 0x80, 0x80, 0x80});
  expectAlarm1(zs042mh, ZS042MH_A1_MATCH_MINUTE_SECOND, 255, 255, 23, 45, {0x07, 0x45, 0x23, 0x80, 0x80});
  expectAlarm1(zs042mh, ZS042MH_A1_MATCH_HOUR_MINUTE_SECOND, 255, 14, 23, 45, {0x07, 0x45, 0x23, 0x14, 0x80});
  expectAlarm1(zs042mh, ZS042MH_A1_MATCH_DATE_HOUR_MINUTE_SECOND, 31, 14, 23, 45, {0x07, 0x45, 0x23, 0x14, 0x31});
  expectAlarm1(zs042mh, ZS042MH_A1_MATCH_WEEKDAY_HOUR_MINUTE_SECOND, 1, 14, 23, 45, {0x07, 0x45, 0x23, 0x14, 0x41});

  expectAlarm2(zs042mh, ZS042MH_A2_EVERY_MINUTE, 255, 255, 255, {0x0B, 0x80, 0x80, 0x80});
  expectAlarm2(zs042mh, ZS042MH_A2_MATCH_MINUTE, 255, 255, 23, {0x0B, 0x23, 0x80, 0x80});
  expectAlarm2(zs042mh, ZS042MH_A2_MATCH_HOUR_MINUTE, 255, 14, 23, {0x0B, 0x23, 0x14, 0x80});
  expectAlarm2(zs042mh, ZS042MH_A2_MATCH_DATE_HOUR_MINUTE, 31, 14, 23, {0x0B, 0x23, 0x14, 0x31});
  expectAlarm2(zs042mh, ZS042MH_A2_MATCH_WEEKDAY_HOUR_MINUTE, 1, 14, 23, {0x0B, 0x23, 0x14, 0x41});

  resetWire();
  Wire.registers[0x0E] = 0xA4;
  assert(zs042mh.setAlarm1(ZS042MH_A1_EVERY_SECOND, 0, 0, 0, 0));
  assert(Wire.registers[0x0E] == 0xA5);
  assert(zs042mh.setAlarm2(ZS042MH_A2_EVERY_MINUTE, 0, 0, 0));
  assert(Wire.registers[0x0E] == 0xA7);

  resetWire();
  assert(!zs042mh.setAlarm1(ZS042MH_A1_MATCH_SECOND, 0, 0, 0, 60));
  assert(!zs042mh.setAlarm1(ZS042MH_A1_MATCH_MINUTE_SECOND, 0, 0, 60, 0));
  assert(!zs042mh.setAlarm1(ZS042MH_A1_MATCH_HOUR_MINUTE_SECOND, 0, 24, 0, 0));
  assert(!zs042mh.setAlarm1(ZS042MH_A1_MATCH_DATE_HOUR_MINUTE_SECOND, 32, 0, 0, 0));
  assert(!zs042mh.setAlarm1(ZS042MH_A1_MATCH_WEEKDAY_HOUR_MINUTE_SECOND, 8, 0, 0, 0));
  assert(!zs042mh.setAlarm1((ZS042MHAlarm1Mode)-1, 0, 0, 0, 0));
  assert(Wire.transmissions.empty());

  resetWire();
  assert(!zs042mh.setAlarm2(ZS042MH_A2_MATCH_MINUTE, 0, 0, 60));
  assert(!zs042mh.setAlarm2(ZS042MH_A2_MATCH_HOUR_MINUTE, 0, 24, 0));
  assert(!zs042mh.setAlarm2(ZS042MH_A2_MATCH_DATE_HOUR_MINUTE, 32, 0, 0));
  assert(!zs042mh.setAlarm2(ZS042MH_A2_MATCH_WEEKDAY_HOUR_MINUTE, 8, 0, 0));
  assert(!zs042mh.setAlarm2((ZS042MHAlarm2Mode)-1, 0, 0, 0));
  assert(Wire.transmissions.empty());

  resetWire();
  assert(!zs042mh.setSquareWave(0));
  assert(!zs042mh.setSquareWave(2));
  assert(Wire.transmissions.empty());

  const uint16_t squareWaveRates[] = {1, 1024, 4096, 8192};
  const uint8_t expectedRateBits[] = {0x00, 0x08, 0x10, 0x18};
  for (size_t index = 0; index < sizeof(squareWaveRates) / sizeof(squareWaveRates[0]); index++) {
    resetWire();
    Wire.registers[0x0E] |= 0x04;
    assert(zs042mh.setSquareWave(squareWaveRates[index]));
    assert(Wire.registers[0x0E] == (uint8_t)(0xA0 | expectedRateBits[index]));
  }

  resetWire();
  Wire.registers[0x0E] = 0xBB;
  assert(zs042mh.setAlarmInterruptMode());
  assert(Wire.registers[0x0E] == 0xBF);

  std::cout << "All alarm patterns, output modes, and validation paths passed" << std::endl;
  return 0;
}
