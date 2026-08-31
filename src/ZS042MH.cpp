#include "ZS042MH.h"

#include <math.h>
#include <string.h>

namespace {
const uint8_t REG_TIME = 0x00;
const uint8_t REG_ALARM_1 = 0x07;
const uint8_t REG_ALARM_2 = 0x0B;
const uint8_t REG_CONTROL = 0x0E;
const uint8_t REG_STATUS = 0x0F;
const uint8_t REG_TEMPERATURE = 0x11;

const uint8_t CONTROL_RS2 = 0x10;
const uint8_t CONTROL_RS1 = 0x08;
const uint8_t CONTROL_INTCN = 0x04;
const uint8_t CONTROL_A2IE = 0x02;
const uint8_t CONTROL_A1IE = 0x01;

const uint8_t STATUS_OSF = 0x80;
const uint8_t STATUS_A2F = 0x02;
const uint8_t STATUS_A1F = 0x01;

bool isValidBcd(uint8_t value) {
  return (value & 0x0F) <= 9 && (value >> 4) <= 9;
}
}

ZS042MH::ZS042MH(TwoWire &wire, uint8_t rtcAddress, uint8_t eepromAddress, uint16_t eepromSize) : _wire(wire), _rtcAddress(rtcAddress), _eepromAddress(eepromAddress), _eepromSize(eepromSize) {
}

void ZS042MH::begin() {
  _wire.begin();
}

bool ZS042MH::rtcConnected() {
  return deviceConnected(_rtcAddress);
}

bool ZS042MH::eepromConnected() {
  return deviceConnected(_eepromAddress);
}

bool ZS042MH::isValidDate(uint16_t year, uint8_t month, uint8_t day) {
  static const uint8_t daysPerMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (year < 2000 || year > 2099 || month < 1 || month > 12 || day < 1) {
    return false;
  }
  uint8_t maximumDay = daysPerMonth[month - 1];
  bool leapYear = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
  if (month == 2 && leapYear) {
    maximumDay++;
  }
  return day <= maximumDay;
}

uint8_t ZS042MH::calculateDayOfWeek(uint16_t year, uint8_t month, uint8_t day) {
  static const uint8_t offsets[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (!isValidDate(year, month, day)) {
    return 0;
  }
  if (month < 3) {
    year--;
  }
  return ((year + year / 4 - year / 100 + year / 400 + offsets[month - 1] + day) % 7) + 1;
}

bool ZS042MH::setTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) {
  if (!isValidDate(year, month, day) || hour > 23 || minute > 59 || second > 59) {
    return false;
  }
  uint8_t buffer[7];
  buffer[0] = decimalToBcd(second) & 0x7F;
  buffer[1] = decimalToBcd(minute);
  buffer[2] = decimalToBcd(hour);
  buffer[3] = calculateDayOfWeek(year, month, day);
  buffer[4] = decimalToBcd(day);
  buffer[5] = decimalToBcd(month);
  buffer[6] = decimalToBcd((uint8_t)(year - 2000));
  if (!rtcWrite(REG_TIME, buffer, sizeof(buffer))) {
    return false;
  }
  return rtcUpdateRegister(REG_STATUS, STATUS_OSF, 0);
}

bool ZS042MH::getTime(ZS042MHDateTime &dateTime) {
  uint8_t buffer[7];
  if (!rtcRead(REG_TIME, buffer, sizeof(buffer))) {
    return false;
  }

  if ((buffer[0] & 0x80) || (buffer[1] & 0x80) || (buffer[2] & 0x80) || (buffer[3] & 0xF8) || (buffer[4] & 0xC0) || (buffer[5] & 0xE0) || !isValidBcd(buffer[0]) || !isValidBcd(buffer[1]) || !isValidBcd(buffer[4]) || !isValidBcd(buffer[5]) || !isValidBcd(buffer[6])) {
    return false;
  }

  ZS042MHDateTime decoded;
  decoded.second = bcdToDecimal(buffer[0]);
  decoded.minute = bcdToDecimal(buffer[1]);
  if (buffer[2] & 0x40) {
    uint8_t rawHour = buffer[2] & 0x1F;
    if (!isValidBcd(rawHour)) {
      return false;
    }
    uint8_t hour = bcdToDecimal(rawHour);
    if (hour < 1 || hour > 12) {
      return false;
    }
    decoded.hour = (buffer[2] & 0x20) ? (hour % 12) + 12 : hour % 12;
  } else {
    uint8_t rawHour = buffer[2] & 0x3F;
    if (!isValidBcd(rawHour)) {
      return false;
    }
    decoded.hour = bcdToDecimal(rawHour);
  }
  decoded.dayOfWeek = buffer[3];
  decoded.day = bcdToDecimal(buffer[4]);
  decoded.month = bcdToDecimal(buffer[5]);
  decoded.year = 2000 + bcdToDecimal(buffer[6]);
  if (decoded.second > 59 || decoded.minute > 59 || decoded.hour > 23 || decoded.dayOfWeek < 1 || !isValidDate(decoded.year, decoded.month, decoded.day)) {
    return false;
  }

  dateTime = decoded;
  return true;
}

float ZS042MH::getTemperature() {
  uint8_t buffer[2];
  if (!rtcRead(REG_TEMPERATURE, buffer, sizeof(buffer))) {
    return NAN;
  }
  return (float)(int8_t)buffer[0] + ((buffer[1] >> 6) * 0.25f);
}

bool ZS042MH::oscillatorStopped(bool &stopped) {
  uint8_t status;
  if (!rtcRead(REG_STATUS, &status, 1)) {
    return false;
  }
  stopped = status & STATUS_OSF;
  return true;
}

bool ZS042MH::setAlarm1(ZS042MHAlarm1Mode mode, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) {
  if (mode > ZS042MH_A1_MATCH_WEEKDAY_HOUR_MINUTE_SECOND) {
    return false;
  }
  uint8_t buffer[4] = {0, 0, 0, 0};
  switch (mode) {
    case ZS042MH_A1_EVERY_SECOND:
      buffer[0] |= 0x80;
      buffer[1] |= 0x80;
      buffer[2] |= 0x80;
      buffer[3] |= 0x80;
      break;
    case ZS042MH_A1_MATCH_SECOND:
      if (second > 59) {
        return false;
      }
      buffer[0] = decimalToBcd(second);
      buffer[1] |= 0x80;
      buffer[2] |= 0x80;
      buffer[3] |= 0x80;
      break;
    case ZS042MH_A1_MATCH_MINUTE_SECOND:
      if (minute > 59 || second > 59) {
        return false;
      }
      buffer[0] = decimalToBcd(second);
      buffer[1] = decimalToBcd(minute);
      buffer[2] |= 0x80;
      buffer[3] |= 0x80;
      break;
    case ZS042MH_A1_MATCH_HOUR_MINUTE_SECOND:
      if (hour > 23 || minute > 59 || second > 59) {
        return false;
      }
      buffer[0] = decimalToBcd(second);
      buffer[1] = decimalToBcd(minute);
      buffer[2] = decimalToBcd(hour);
      buffer[3] |= 0x80;
      break;
    case ZS042MH_A1_MATCH_DATE_HOUR_MINUTE_SECOND:
      if (day < 1 || day > 31 || hour > 23 || minute > 59 || second > 59) {
        return false;
      }
      buffer[0] = decimalToBcd(second);
      buffer[1] = decimalToBcd(minute);
      buffer[2] = decimalToBcd(hour);
      buffer[3] = decimalToBcd(day);
      break;
    case ZS042MH_A1_MATCH_WEEKDAY_HOUR_MINUTE_SECOND:
      if (day < 1 || day > 7 || hour > 23 || minute > 59 || second > 59) {
        return false;
      }
      buffer[0] = decimalToBcd(second);
      buffer[1] = decimalToBcd(minute);
      buffer[2] = decimalToBcd(hour);
      buffer[3] = decimalToBcd(day) | 0x40;
      break;
    default:
      return false;
  }
  if (!rtcWrite(REG_ALARM_1, buffer, sizeof(buffer))) {
    return false;
  }
  if (!rtcUpdateRegister(REG_CONTROL, 0, CONTROL_INTCN | CONTROL_A1IE)) {
    return false;
  }
  return rtcUpdateRegister(REG_STATUS, STATUS_A1F, 0);
}

bool ZS042MH::setAlarm2(ZS042MHAlarm2Mode mode, uint8_t day, uint8_t hour, uint8_t minute) {
  if (mode > ZS042MH_A2_MATCH_WEEKDAY_HOUR_MINUTE) {
    return false;
  }
  uint8_t buffer[3] = {0, 0, 0};
  switch (mode) {
    case ZS042MH_A2_EVERY_MINUTE:
      buffer[0] |= 0x80;
      buffer[1] |= 0x80;
      buffer[2] |= 0x80;
      break;
    case ZS042MH_A2_MATCH_MINUTE:
      if (minute > 59) {
        return false;
      }
      buffer[0] = decimalToBcd(minute);
      buffer[1] |= 0x80;
      buffer[2] |= 0x80;
      break;
    case ZS042MH_A2_MATCH_HOUR_MINUTE:
      if (hour > 23 || minute > 59) {
        return false;
      }
      buffer[0] = decimalToBcd(minute);
      buffer[1] = decimalToBcd(hour);
      buffer[2] |= 0x80;
      break;
    case ZS042MH_A2_MATCH_DATE_HOUR_MINUTE:
      if (day < 1 || day > 31 || hour > 23 || minute > 59) {
        return false;
      }
      buffer[0] = decimalToBcd(minute);
      buffer[1] = decimalToBcd(hour);
      buffer[2] = decimalToBcd(day);
      break;
    case ZS042MH_A2_MATCH_WEEKDAY_HOUR_MINUTE:
      if (day < 1 || day > 7 || hour > 23 || minute > 59) {
        return false;
      }
      buffer[0] = decimalToBcd(minute);
      buffer[1] = decimalToBcd(hour);
      buffer[2] = decimalToBcd(day) | 0x40;
      break;
    default:
      return false;
  }
  if (!rtcWrite(REG_ALARM_2, buffer, sizeof(buffer))) {
    return false;
  }
  if (!rtcUpdateRegister(REG_CONTROL, 0, CONTROL_INTCN | CONTROL_A2IE)) {
    return false;
  }
  return rtcUpdateRegister(REG_STATUS, STATUS_A2F, 0);
}

bool ZS042MH::disableAlarm(uint8_t alarmNumber) {
  if (alarmNumber != 1 && alarmNumber != 2) {
    return false;
  }
  uint8_t enableBit = alarmNumber == 1 ? CONTROL_A1IE : CONTROL_A2IE;
  uint8_t flagBit = alarmNumber == 1 ? STATUS_A1F : STATUS_A2F;
  if (!rtcUpdateRegister(REG_CONTROL, enableBit, 0)) {
    return false;
  }
  return rtcUpdateRegister(REG_STATUS, flagBit, 0);
}

bool ZS042MH::checkAndClearAlarms(uint8_t &fired) {
  uint8_t status;
  fired = 0;
  if (!rtcRead(REG_STATUS, &status, 1)) {
    return false;
  }
  fired = status & (STATUS_A1F | STATUS_A2F);
  if (fired) {
    return rtcSetRegister(REG_STATUS, status & ~fired);
  }
  return true;
}

bool ZS042MH::setSquareWave(uint16_t rate) {
  uint8_t control;
  if (!rtcRead(REG_CONTROL, &control, 1)) {
    return false;
  }
  control &= ~(CONTROL_RS1 | CONTROL_RS2);
  switch (rate) {
    case 0:
      control |= CONTROL_INTCN;
      break;
    case 1:
      control &= ~CONTROL_INTCN;
      break;
    case 1024:
      control = (control | CONTROL_RS1) & ~CONTROL_INTCN;
      break;
    case 4096:
      control = (control | CONTROL_RS2) & ~CONTROL_INTCN;
      break;
    case 8192:
      control = (control | CONTROL_RS1 | CONTROL_RS2) & ~CONTROL_INTCN;
      break;
    default:
      return false;
  }
  return rtcSetRegister(REG_CONTROL, control);
}

uint16_t ZS042MH::eepromSize() const {
  return _eepromSize;
}

bool ZS042MH::eepromWriteByte(uint16_t address, uint8_t value) {
  return eepromWrite(address, &value, 1);
}

bool ZS042MH::eepromWrite(uint16_t address, const uint8_t *data, uint16_t length) {
  if (!eepromRangeValid(address, length) || (length && data == NULL)) {
    return false;
  }
  while (length) {
    uint8_t pageRoom = EEPROM_PAGE_SIZE - (address % EEPROM_PAGE_SIZE);
    uint8_t chunk = length < pageRoom ? (uint8_t)length : pageRoom;
    if (chunk > WIRE_DATA_CHUNK) {
      chunk = WIRE_DATA_CHUNK;
    }
    _wire.beginTransmission(_eepromAddress);
    _wire.write((uint8_t)(address >> 8));
    _wire.write((uint8_t)address);
    for (uint8_t index = 0; index < chunk; index++) {
      _wire.write(data[index]);
    }
    if (_wire.endTransmission() != 0) {
      return false;
    }
    if (!eepromWaitReady()) {
      return false;
    }
    address += chunk;
    data += chunk;
    length -= chunk;
  }
  return true;
}

bool ZS042MH::eepromRead(uint16_t address, uint8_t *data, uint16_t length) {
  if (!eepromRangeValid(address, length) || (length && data == NULL)) {
    return false;
  }
  while (length) {
    uint8_t chunk = length < WIRE_DATA_CHUNK ? (uint8_t)length : WIRE_DATA_CHUNK;
    _wire.beginTransmission(_eepromAddress);
    _wire.write((uint8_t)(address >> 8));
    _wire.write((uint8_t)address);
    if (_wire.endTransmission(false) != 0 || _wire.requestFrom((int)_eepromAddress, (int)chunk) != chunk) {
      return false;
    }
    for (uint8_t index = 0; index < chunk; index++) {
      data[index] = _wire.read();
    }
    address += chunk;
    data += chunk;
    length -= chunk;
  }
  return true;
}

bool ZS042MH::eepromFill(uint16_t address, uint16_t length, uint8_t value) {
  if (!eepromRangeValid(address, length)) {
    return false;
  }
  uint8_t buffer[WIRE_DATA_CHUNK];
  memset(buffer, value, sizeof(buffer));
  while (length) {
    uint16_t chunk = length < sizeof(buffer) ? length : sizeof(buffer);
    if (!eepromWrite(address, buffer, chunk)) {
      return false;
    }
    address += chunk;
    length -= chunk;
  }
  return true;
}

uint8_t ZS042MH::bcdToDecimal(uint8_t value) {
  return (value >> 4) * 10 + (value & 0x0F);
}

uint8_t ZS042MH::decimalToBcd(uint8_t value) {
  return ((value / 10) << 4) | (value % 10);
}

bool ZS042MH::deviceConnected(uint8_t address) {
  _wire.beginTransmission(address);
  return _wire.endTransmission() == 0;
}

bool ZS042MH::rtcWrite(uint8_t reg, const uint8_t *buffer, uint8_t length) {
  _wire.beginTransmission(_rtcAddress);
  _wire.write(reg);
  for (uint8_t index = 0; index < length; index++) {
    _wire.write(buffer[index]);
  }
  return _wire.endTransmission() == 0;
}

bool ZS042MH::rtcRead(uint8_t reg, uint8_t *buffer, uint8_t length) {
  memset(buffer, 0, length);
  _wire.beginTransmission(_rtcAddress);
  _wire.write(reg);
  if (_wire.endTransmission(false) != 0 || _wire.requestFrom((int)_rtcAddress, (int)length) != length) {
    return false;
  }
  for (uint8_t index = 0; index < length; index++) {
    buffer[index] = _wire.read();
  }
  return true;
}

bool ZS042MH::rtcSetRegister(uint8_t reg, uint8_t value) {
  return rtcWrite(reg, &value, 1);
}

bool ZS042MH::rtcUpdateRegister(uint8_t reg, uint8_t clearMask, uint8_t setMask) {
  uint8_t value;
  if (!rtcRead(reg, &value, 1)) {
    return false;
  }
  value = (value & ~clearMask) | setMask;
  return rtcSetRegister(reg, value);
}

bool ZS042MH::eepromRangeValid(uint16_t address, uint16_t length) const {
  return (uint32_t)address + length <= _eepromSize;
}

bool ZS042MH::eepromWaitReady() {
  uint32_t startedAt = millis();
  do {
    _wire.beginTransmission(_eepromAddress);
    if (_wire.endTransmission() == 0) {
      return true;
    }
  } while ((uint32_t)(millis() - startedAt) < EEPROM_WRITE_TIMEOUT_MS);
  return false;
}
