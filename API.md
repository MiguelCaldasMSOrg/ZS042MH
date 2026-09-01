# ZS042MH API reference

Include the library with:

```cpp
#include <ZS042MH.h>
```

## Error model

Unless a method documents another result, Boolean methods return `true` only when all validation and I2C operations performed by that call succeed. They return `false` for invalid arguments, an address that does not acknowledge, a short read, another I2C transfer failure, or an EEPROM write-cycle timeout. The API does not expose a more specific numeric error code.

Output parameters are only changed on success unless stated otherwise. Operations that require multiple I2C transfers are not transactional: a later failure can leave earlier register or EEPROM writes applied.

## Types and constants

### `ZS042MHDateTime`

```cpp
struct ZS042MHDateTime {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t dayOfWeek;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
};
```

`year` is from `2000` through `2099`; `month` is `1` through `12`; `day` is the calendar day; `dayOfWeek` is Sunday `1` through Saturday `7`; `hour` is `0` through `23`; and `minute` and `second` are `0` through `59`.

### Alarm modes

`ZS042MHAlarm1Mode` contains:

| Value | Fields matched |
|---|---|
| `ZS042MH_A1_EVERY_SECOND` | None |
| `ZS042MH_A1_MATCH_SECOND` | Second |
| `ZS042MH_A1_MATCH_MINUTE_SECOND` | Minute, second |
| `ZS042MH_A1_MATCH_HOUR_MINUTE_SECOND` | Hour, minute, second |
| `ZS042MH_A1_MATCH_DATE_HOUR_MINUTE_SECOND` | Date, hour, minute, second |
| `ZS042MH_A1_MATCH_WEEKDAY_HOUR_MINUTE_SECOND` | Weekday, hour, minute, second |

`ZS042MHAlarm2Mode` contains:

| Value | Fields matched |
|---|---|
| `ZS042MH_A2_EVERY_MINUTE` | None; fires at second `00` |
| `ZS042MH_A2_MATCH_MINUTE` | Minute |
| `ZS042MH_A2_MATCH_HOUR_MINUTE` | Hour, minute |
| `ZS042MH_A2_MATCH_DATE_HOUR_MINUTE` | Date, hour, minute |
| `ZS042MH_A2_MATCH_WEEKDAY_HOUR_MINUTE` | Weekday, hour, minute |

### Class constants

| Constant | Value | Meaning |
|---|---:|---|
| `ZS042MH::DEFAULT_RTC_ADDRESS` | `0x68` | Default DS3231 7-bit I2C address |
| `ZS042MH::DEFAULT_EEPROM_ADDRESS` | `0x57` | Default AT24C32 7-bit I2C address |
| `ZS042MH::DEFAULT_EEPROM_SIZE` | `4096` | Default EEPROM capacity in bytes |
| `ZS042MH::ALARM_1` | `0x01` | Alarm 1 bit returned by `checkAndClearAlarms()` |
| `ZS042MH::ALARM_2` | `0x02` | Alarm 2 bit returned by `checkAndClearAlarms()` |

## Construction and bus setup

### `ZS042MH()`

```cpp
explicit ZS042MH(
    TwoWire &wire = Wire,
    uint8_t rtcAddress = DEFAULT_RTC_ADDRESS,
    uint8_t eepromAddress = DEFAULT_EEPROM_ADDRESS,
    uint16_t eepromSize = DEFAULT_EEPROM_SIZE);

ZS042MH(uint8_t rtcAddress, uint8_t eepromAddress);
```

- `wire`: Arduino `TwoWire` bus used by every operation. `Wire` is the Arduino core's global default I2C bus. The object is retained by reference and must outlive the `ZS042MH` instance.
- `rtcAddress`: DS3231 7-bit I2C address. Normally `0x68`.
- `eepromAddress`: AT24C32 7-bit I2C address. Common modules permit `0x50` through `0x57`, depending on their address jumpers.
- `eepromSize`: accessible EEPROM capacity in bytes. It controls bounds checking; it does not detect the physical device capacity.

The address-only overload uses `Wire` and `DEFAULT_EEPROM_SIZE`. The constructors store configuration only and perform no I2C. They do not validate the supplied addresses or capacity.

Examples:

```cpp
ZS042MH rtcEeprom;                                  // Wire, RTC 0x68, EEPROM 0x57
ZS042MH addressedRtcEeprom(0x68, 0x50);             // Wire, EEPROM 0x50
ZS042MH secondaryBusRtcEeprom(Wire1);               // Board-provided secondary bus
ZS042MH customRtcEeprom(Wire1, 0x68, 0x50, 4096);  // Custom bus and configuration
```

### `begin()`

```cpp
void begin();
```

Calls `begin()` on the configured `TwoWire` object. For a default-constructed instance this is `Wire.begin()`. It returns no status and does not probe either device. Calling it can initialize or reinitialize a bus shared with other code.

Some Arduino cores require board-specific pin or bus configuration. In that case, initialize the selected `TwoWire` object as required by the core rather than calling this parameterless wrapper.

## Connection probes

### `rtcConnected()`

```cpp
bool rtcConnected();
```

Starts an empty transmission to the configured RTC address. Returns `true` when the address acknowledges and `false` otherwise. This proves that an I2C device acknowledged; it does not verify that the device is a DS3231 or that its time is valid.

### `eepromConnected()`

```cpp
bool eepromConnected();
```

Starts an empty transmission to the configured EEPROM address. Returns `true` when the address acknowledges and `false` otherwise. An AT24C32 may temporarily fail to acknowledge while completing a write cycle.

## Date helpers

These helpers are `static`; they do not use I2C and may be called without a `ZS042MH` instance.

### `isValidDate()`

```cpp
static bool isValidDate(uint16_t year, uint8_t month, uint8_t day);
```

- `year`: full year from `2000` through `2099`.
- `month`: month from `1` through `12`.
- `day`: calendar day, checked against the month and leap-year rules.

Returns `true` for a valid supported date and `false` otherwise. It has no side effects.

### `calculateDayOfWeek()`

```cpp
static uint8_t calculateDayOfWeek(uint16_t year, uint8_t month, uint8_t day);
```

The parameters have the same ranges as `isValidDate()`. Returns Sunday as `1` through Saturday as `7`, or `0` when the date is invalid. It has no side effects.

## Clock and temperature

### `setTime()`

```cpp
bool setTime(uint16_t year, uint8_t month, uint8_t day,
             uint8_t hour, uint8_t minute, uint8_t second);
```

- `year`, `month`, `day`: a valid date from `2000-01-01` through `2099-12-31`.
- `hour`: 24-hour value from `0` through `23`.
- `minute`: value from `0` through `59`.
- `second`: value from `0` through `59`.

Writes all seven DS3231 timekeeping registers in 24-hour format, including the calculated weekday. It then clears the oscillator-stop flag while preserving unrelated status bits. Returns `false` without I2C for invalid values. An I2C failure while clearing the flag can return `false` after the new time has already been written.

### `getTime()`

```cpp
bool getTime(ZS042MHDateTime &dateTime);
```

- `dateTime`: destination for the decoded date and time.

Reads and validates all seven timekeeping registers. Both DS3231 12-hour and 24-hour encodings are accepted; the result always uses 24-hour time. Returns `true` and replaces `dateTime` when transport and register data are valid. Returns `false` for an I2C error, short read, invalid BCD, unsupported century bit, invalid date, weekday, or time. On failure, `dateTime` is unchanged.

This method does not inspect or clear the oscillator-stop flag. Use `oscillatorStopped()` before trusting time retained across a power loss.

### `getTemperature()`

```cpp
float getTemperature();
```

Reads the DS3231 temperature registers and returns degrees Celsius at `0.25` degree resolution. Negative temperatures are supported. Returns `NAN` on an I2C error or short read; use `isnan()` to test for failure. It does not start a forced temperature conversion.

### `oscillatorStopped()`

```cpp
bool oscillatorStopped(bool &stopped);
```

- `stopped`: receives `true` when the DS3231 oscillator-stop flag is set, otherwise `false`.

Returns `true` after a successful status-register read and `false` on I2C failure. On failure, `stopped` is unchanged. The method does not clear the flag; `setTime()` clears it after writing a valid time.

## Alarms

### `setAlarm1()`

```cpp
bool setAlarm1(ZS042MHAlarm1Mode mode, uint8_t day,
               uint8_t hour, uint8_t minute, uint8_t second);
```

- `mode`: one of the Alarm 1 modes listed above.
- `day`: date `1` through `31` for the date mode, or Sunday `1` through Saturday `7` for the weekday mode. Ignored by other modes.
- `hour`: `0` through `23` when matched; ignored by modes that do not match the hour.
- `minute`: `0` through `59` when matched; ignored by modes that do not match the minute.
- `second`: `0` through `59` when matched; ignored by `ZS042MH_A1_EVERY_SECOND`.

Writes the four Alarm 1 registers, enables Alarm 1 (`A1IE`), and clears the old Alarm 1 flag. It does not change the `INT/SQW` output mode or square-wave rate, so the schedule can be configured while the pin continues to output a square wave. Call `setAlarmInterruptMode()` separately when enabled alarms should drive `INT/SQW`. Alarm 2 enable state and unrelated control/status bits are preserved. Returns `false` for an unknown mode, an invalid parameter used by that mode, or any I2C failure. Ignored parameters are not validated. Because setup uses multiple transfers, `false` can be returned after alarm registers or control bits have changed.

### `setAlarm2()`

```cpp
bool setAlarm2(ZS042MHAlarm2Mode mode, uint8_t day,
               uint8_t hour, uint8_t minute);
```

- `mode`: one of the Alarm 2 modes listed above.
- `day`: date `1` through `31` for the date mode, or Sunday `1` through Saturday `7` for the weekday mode. Ignored by other modes.
- `hour`: `0` through `23` when matched; ignored by modes that do not match the hour.
- `minute`: `0` through `59` when matched; ignored by `ZS042MH_A2_EVERY_MINUTE`.

Writes the three Alarm 2 registers, enables Alarm 2 (`A2IE`), and clears the old Alarm 2 flag. It does not change the `INT/SQW` output mode or square-wave rate. Call `setAlarmInterruptMode()` separately when enabled alarms should drive `INT/SQW`. Alarm 1 enable state and unrelated bits are preserved. Return and partial-application behavior match `setAlarm1()`.

### `disableAlarm()`

```cpp
bool disableAlarm(uint8_t alarmNumber);
```

- `alarmNumber`: `1` for Alarm 1 or `2` for Alarm 2. The `ALARM_1` and `ALARM_2` bit constants currently have the same numeric values, but callers should pass the documented alarm number.

Clears the selected alarm-enable bit and then clears its status flag, preserving unrelated bits. Returns `false` for any other number or an I2C failure. A failure while clearing the flag can occur after the alarm has already been disabled.

### `checkAndClearAlarms()`

```cpp
bool checkAndClearAlarms(uint8_t &fired);
```

- `fired`: receives a bit mask. Test it with `ZS042MH::ALARM_1` and `ZS042MH::ALARM_2`; both bits can be set.

Sets `fired` to `0` before reading the status register. If the read succeeds, it reports the asserted alarm flags and clears only those flags, preserving all other status bits. Returns `true` when the read and any required clear succeed. If the initial read fails, returns `false` with `fired == 0`. If clearing fails, returns `false` while `fired` still reports the flags observed by the successful read.

Flags can be polled even without an interrupt wire and while `INT/SQW` is configured for square-wave output.

## INT/SQW output mode

### `setAlarmInterruptMode()`

```cpp
bool setAlarmInterruptMode();
```

Configures `INT/SQW` for alarm interrupts. It does not configure an alarm schedule or enable either alarm. Existing alarm-enable, square-wave rate, and unrelated control bits are preserved. Returns `false` for an I2C failure.

### `setSquareWave()`

```cpp
bool setSquareWave(uint16_t rate);
```

- `rate`: `1`, `1024`, `4096`, or `8192` hertz.

Configures `INT/SQW` as a square-wave output at the requested frequency. Existing alarm-enable and unrelated control bits are preserved. Returns `false` without performing I2C for an unsupported rate, including `0`, or returns `false` for an I2C failure.

The pin is open-drain and needs a suitable pull-up. Alarm flags continue to latch in square-wave mode, but alarms cannot drive the pin until interrupt mode is restored with `setAlarmInterruptMode()`.

## EEPROM

EEPROM addresses are zero-based. For the default 4096-byte capacity, valid byte addresses are `0` through `4095`. Reads and writes are split into 16-byte Wire-safe chunks; writes are also split at 32-byte AT24C32 page boundaries. After each write chunk, the library polls for an acknowledge for up to 20 ms.

### `eepromSize()`

```cpp
uint16_t eepromSize() const;
```

Returns the capacity supplied to the constructor. It performs no I2C and does not query the physical EEPROM.

### `eepromWriteByte()`

```cpp
bool eepromWriteByte(uint16_t address, uint8_t value);
```

- `address`: destination byte address, less than `eepromSize()`.
- `value`: byte to write.

Writes one byte and waits for the EEPROM write cycle to complete. Returns `false` for an out-of-range address, an I2C failure, or a write-cycle timeout.

### `eepromWrite()`

```cpp
bool eepromWrite(uint16_t address, const uint8_t *data, uint16_t length);
```

- `address`: first destination byte address.
- `data`: source buffer containing at least `length` bytes. It may be `NULL` only when `length` is zero.
- `length`: number of bytes to write.

The full range `[address, address + length)` must fit within the configured capacity. Returns `true` after all chunks are written and acknowledge polling succeeds. Returns `false` for an invalid range, a null nonempty buffer, I2C failure, or timeout. If a later chunk fails, earlier chunks remain written.

A zero-length write performs no I2C and returns `true` when `address` is less than or equal to `eepromSize()`; `address == eepromSize()` is valid only for a zero-length operation.

### `eepromRead()`

```cpp
bool eepromRead(uint16_t address, uint8_t *data, uint16_t length);
```

- `address`: first source byte address.
- `data`: destination buffer with room for at least `length` bytes. It may be `NULL` only when `length` is zero.
- `length`: number of bytes to read.

The full range must fit within the configured capacity. Returns `true` after all chunks are read. Returns `false` for an invalid range, a null nonempty buffer, an I2C failure, or a short read. On failure after one or more chunks, the corresponding prefix of `data` has already been replaced; unread bytes are unchanged.

A zero-length read has the same no-I2C and boundary behavior as a zero-length write.

### `eepromFill()`

```cpp
bool eepromFill(uint16_t address, uint16_t length, uint8_t value);
```

- `address`: first destination byte address.
- `length`: number of bytes to fill.
- `value`: byte written to every location.

Fills the checked range using repeated EEPROM writes. Returns `true` after the whole range is written, or for a valid zero-length range. Returns `false` for an invalid range, I2C failure, or timeout. A later failure leaves the already completed prefix filled.

## Internal helpers

BCD conversion, raw RTC register access, read-modify-write register updates, EEPROM range checking, and EEPROM acknowledge polling are private implementation details. They are not callable API. Register updates used by the public methods preserve unrelated bits, and the raw I2C helpers map Wire transfer failures to the public error behavior described above.