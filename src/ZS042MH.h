#ifndef ZS042MH_H
#define ZS042MH_H

#include <Arduino.h>
#include <Wire.h>

struct ZS042MHDateTime {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t dayOfWeek;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
};

enum ZS042MHAlarm1Mode {
  ZS042MH_A1_EVERY_SECOND,
  ZS042MH_A1_MATCH_SECOND,
  ZS042MH_A1_MATCH_MINUTE_SECOND,
  ZS042MH_A1_MATCH_HOUR_MINUTE_SECOND,
  ZS042MH_A1_MATCH_DATE_HOUR_MINUTE_SECOND,
  ZS042MH_A1_MATCH_WEEKDAY_HOUR_MINUTE_SECOND
};

enum ZS042MHAlarm2Mode {
  ZS042MH_A2_EVERY_MINUTE,
  ZS042MH_A2_MATCH_MINUTE,
  ZS042MH_A2_MATCH_HOUR_MINUTE,
  ZS042MH_A2_MATCH_DATE_HOUR_MINUTE,
  ZS042MH_A2_MATCH_WEEKDAY_HOUR_MINUTE
};

class ZS042MH {
 public:
  static const uint8_t DEFAULT_RTC_ADDRESS = 0x68;
  static const uint8_t DEFAULT_EEPROM_ADDRESS = 0x57;
  static const uint16_t DEFAULT_EEPROM_SIZE = 4096;
  static const uint8_t ALARM_1 = 0x01;
  static const uint8_t ALARM_2 = 0x02;

  explicit ZS042MH(TwoWire &wire = Wire, uint8_t rtcAddress = DEFAULT_RTC_ADDRESS, uint8_t eepromAddress = DEFAULT_EEPROM_ADDRESS, uint16_t eepromSize = DEFAULT_EEPROM_SIZE);
  ZS042MH(uint8_t rtcAddress, uint8_t eepromAddress);

  void begin();
  bool rtcConnected();
  bool eepromConnected();

  static bool isValidDate(uint16_t year, uint8_t month, uint8_t day);
  static uint8_t calculateDayOfWeek(uint16_t year, uint8_t month, uint8_t day);

  bool setTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);
  bool getTime(ZS042MHDateTime &dateTime);
  float getTemperature();
  bool oscillatorStopped(bool &stopped);

  bool setAlarm1(ZS042MHAlarm1Mode mode, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);
  bool setAlarm2(ZS042MHAlarm2Mode mode, uint8_t day, uint8_t hour, uint8_t minute);
  bool disableAlarm(uint8_t alarmNumber);
  bool checkAndClearAlarms(uint8_t &fired);
  bool setAlarmInterruptMode();
  bool setSquareWave(uint16_t rate);

  uint16_t eepromSize() const;
  bool eepromWriteByte(uint16_t address, uint8_t value);
  bool eepromWrite(uint16_t address, const uint8_t *data, uint16_t length);
  bool eepromRead(uint16_t address, uint8_t *data, uint16_t length);
  bool eepromFill(uint16_t address, uint16_t length, uint8_t value);

 private:
  static const uint8_t EEPROM_PAGE_SIZE = 32;
  static const uint8_t WIRE_DATA_CHUNK = 16;
  static const uint8_t EEPROM_WRITE_TIMEOUT_MS = 20;

  TwoWire &_wire;
  uint8_t _rtcAddress;
  uint8_t _eepromAddress;
  uint16_t _eepromSize;

  static uint8_t bcdToDecimal(uint8_t value);
  static uint8_t decimalToBcd(uint8_t value);
  bool deviceConnected(uint8_t address);
  bool rtcWrite(uint8_t reg, const uint8_t *buffer, uint8_t length);
  bool rtcRead(uint8_t reg, uint8_t *buffer, uint8_t length);
  bool rtcSetRegister(uint8_t reg, uint8_t value);
  bool rtcUpdateRegister(uint8_t reg, uint8_t clearMask, uint8_t setMask);
  bool eepromRangeValid(uint16_t address, uint16_t length) const;
  bool eepromWaitReady();
};

#endif
