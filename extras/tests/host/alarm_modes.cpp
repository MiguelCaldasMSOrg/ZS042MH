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
  Wire.endTransmissionResults.clear();
  Wire.requestFromResult = -1;
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
  assert(zs042mh.lastError() == ZS042MHError::None);

  resetWire();
  assert(!zs042mh.setSquareWave(0));
  assert(zs042mh.lastError() == ZS042MHError::InvalidArgument);
  assert(!ZS042MH::isValidDate(2026, 2, 29));
  assert(zs042mh.lastError() == ZS042MHError::InvalidArgument);
  zs042mh.begin();
  assert(zs042mh.eepromSize() == ZS042MH::DEFAULT_EEPROM_SIZE);
  assert(zs042mh.lastError() == ZS042MHError::InvalidArgument);

  ZS042MH secondInstance(Wire);
  assert(secondInstance.lastError() == ZS042MHError::None);

  resetWire();
  Wire.endTransmissionResults.push_back(2);
  assert(!zs042mh.rtcConnected());
  assert(zs042mh.lastError() == ZS042MHError::AddressNack);

  resetWire();
  Wire.endTransmissionResults.push_back(3);
  assert(!zs042mh.rtcConnected());
  assert(zs042mh.lastError() == ZS042MHError::DataNack);

  resetWire();
  Wire.endTransmissionResults.push_back(4);
  assert(!zs042mh.rtcConnected());
  assert(zs042mh.lastError() == ZS042MHError::I2c);

  resetWire();
  Wire.requestFromResult = 1;
  float temperature = zs042mh.getTemperature();
  assert(temperature != temperature);
  assert(zs042mh.lastError() == ZS042MHError::ShortRead);

  resetWire();
  ZS042MHDateTime dateTime;
  assert(!zs042mh.getTime(dateTime));
  assert(zs042mh.lastError() == ZS042MHError::InvalidRtcData);

  resetWire();
  Wire.endTransmissionResults.push_back(0);
  for (uint8_t attempt = 0; attempt < 25; attempt++) {
    Wire.endTransmissionResults.push_back(2);
  }
  assert(!zs042mh.eepromWriteByte(0, 0x55));
  assert(zs042mh.lastError() == ZS042MHError::EepromTimeout);

  resetWire();
  assert(zs042mh.rtcConnected());
  assert(zs042mh.lastError() == ZS042MHError::None);

  resetWire();
  ZS042MH addressedRtcEeprom(0x69, 0x50);
  assert(addressedRtcEeprom.rtcConnected());
  assert(Wire.transmissions.back().address == 0x69);
  assert(addressedRtcEeprom.eepromConnected());
  assert(Wire.transmissions.back().address == 0x50);
  assert(addressedRtcEeprom.eepromSize() == ZS042MH::DEFAULT_EEPROM_SIZE);

  expectAlarm1(zs042mh, ZS042MHAlarm1Mode::EverySecond, 255, 255, 255, 255, {0x07, 0x80, 0x80, 0x80, 0x80});
  expectAlarm1(zs042mh, ZS042MHAlarm1Mode::MatchSecond, 255, 255, 255, 45, {0x07, 0x45, 0x80, 0x80, 0x80});
  expectAlarm1(zs042mh, ZS042MHAlarm1Mode::MatchMinuteSecond, 255, 255, 23, 45, {0x07, 0x45, 0x23, 0x80, 0x80});
  expectAlarm1(zs042mh, ZS042MHAlarm1Mode::MatchHourMinuteSecond, 255, 14, 23, 45, {0x07, 0x45, 0x23, 0x14, 0x80});
  expectAlarm1(zs042mh, ZS042MHAlarm1Mode::MatchDateHourMinuteSecond, 31, 14, 23, 45, {0x07, 0x45, 0x23, 0x14, 0x31});
  expectAlarm1(zs042mh, ZS042MHAlarm1Mode::MatchWeekdayHourMinuteSecond, 1, 14, 23, 45, {0x07, 0x45, 0x23, 0x14, 0x41});

  expectAlarm2(zs042mh, ZS042MHAlarm2Mode::EveryMinute, 255, 255, 255, {0x0B, 0x80, 0x80, 0x80});
  expectAlarm2(zs042mh, ZS042MHAlarm2Mode::MatchMinute, 255, 255, 23, {0x0B, 0x23, 0x80, 0x80});
  expectAlarm2(zs042mh, ZS042MHAlarm2Mode::MatchHourMinute, 255, 14, 23, {0x0B, 0x23, 0x14, 0x80});
  expectAlarm2(zs042mh, ZS042MHAlarm2Mode::MatchDateHourMinute, 31, 14, 23, {0x0B, 0x23, 0x14, 0x31});
  expectAlarm2(zs042mh, ZS042MHAlarm2Mode::MatchWeekdayHourMinute, 1, 14, 23, {0x0B, 0x23, 0x14, 0x41});

  resetWire();
  Wire.registers[0x0E] = 0xA4;
  assert(zs042mh.setAlarm1(ZS042MHAlarm1Mode::EverySecond, 0, 0, 0, 0));
  assert(Wire.registers[0x0E] == 0xA5);
  assert(zs042mh.setAlarm2(ZS042MHAlarm2Mode::EveryMinute, 0, 0, 0));
  assert(Wire.registers[0x0E] == 0xA7);

  resetWire();
  assert(!zs042mh.setAlarm1(ZS042MHAlarm1Mode::MatchSecond, 0, 0, 0, 60));
  assert(!zs042mh.setAlarm1(ZS042MHAlarm1Mode::MatchMinuteSecond, 0, 0, 60, 0));
  assert(!zs042mh.setAlarm1(ZS042MHAlarm1Mode::MatchHourMinuteSecond, 0, 24, 0, 0));
  assert(!zs042mh.setAlarm1(ZS042MHAlarm1Mode::MatchDateHourMinuteSecond, 32, 0, 0, 0));
  assert(!zs042mh.setAlarm1(ZS042MHAlarm1Mode::MatchWeekdayHourMinuteSecond, 8, 0, 0, 0));
  assert(!zs042mh.setAlarm1(static_cast<ZS042MHAlarm1Mode>(255), 0, 0, 0, 0));
  assert(Wire.transmissions.empty());

  resetWire();
  assert(!zs042mh.setAlarm2(ZS042MHAlarm2Mode::MatchMinute, 0, 0, 60));
  assert(!zs042mh.setAlarm2(ZS042MHAlarm2Mode::MatchHourMinute, 0, 24, 0));
  assert(!zs042mh.setAlarm2(ZS042MHAlarm2Mode::MatchDateHourMinute, 32, 0, 0));
  assert(!zs042mh.setAlarm2(ZS042MHAlarm2Mode::MatchWeekdayHourMinute, 8, 0, 0));
  assert(!zs042mh.setAlarm2(static_cast<ZS042MHAlarm2Mode>(255), 0, 0, 0));
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
