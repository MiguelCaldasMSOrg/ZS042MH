# ZS042MH Arduino library

An Arduino library for the DS3231 real-time clock and AT24C32 EEPROM found on common ZS-042/MH modules. It provides checked I2C operations for date and time, temperature, alarms, square-wave output, and the 4 KiB EEPROM without coupling the device layer to `Serial` or a particular interrupt pin.

## Hardware

The default 7-bit I2C addresses are `0x68` for the DS3231 and `0x57` for the AT24C32. EEPROM address jumpers can select an address from `0x50` through `0x57`; pass a different address to the constructor when needed.

For a BBC micro:bit V2, connect:

| ZS-042/MH | BBC micro:bit V2 | Purpose |
|---|---|---|
| `VCC` | `3V` | 3.3 V power and logic reference |
| `GND` | `GND` | Common ground |
| `SDA` | `P20` | I2C data |
| `SCL` | `P19` | I2C clock |
| `SQW` / `INT/SQW` | `P2` | Optional alarm or square-wave signal |
| `32K` | Not connected | Unused independent output |

Disconnect power before wiring. Power the module from 3.3 V when connected directly to 3.3 V controllers. Many clones pull I2C and alarm lines up to the module supply, making 5 V operation unsafe without verified level shifting.

ZS-042/MH clone designs vary. Some include a resistor/diode charging path to the coin cell. Do not install a non-rechargeable CR2032 until the exact board has been inspected and any charging path has been safely disabled. A rechargeable cell also requires a suitable charger.

## Arduino IDE installation

Download or clone this repository, then use one of these methods:

1. Download the `ZS042MH-<version>.zip` asset from the [latest GitHub release](https://github.com/MiguelCaldasMSOrg/ZS042MH/releases/latest). In Arduino IDE, choose **Sketch > Include Library > Add .ZIP Library** and select that file.
2. Place the repository folder in the sketchbook `libraries` directory, then restart Arduino IDE.

Use the attached `ZS042MH-<version>.zip` asset rather than GitHub's automatically generated **Source code** archives. This library is distributed through GitHub Releases and is not registered with Arduino Library Manager.

Include and initialize the library:

```cpp
#include <ZS042MH.h>

ZS042MH module;

void setup() {
  module.begin();
}
```

`begin()` initializes the injected I2C bus. To use a different bus or EEPROM address:

```cpp
ZS042MH module(Wire, 0x68, 0x50);
```

The examples are available under **File > Examples > ZS042MH**:

- `ReadWriteTime` reads the clock and temperature and shows how to set the time.
- `AlarmInterrupt` configures Alarm 1 and keeps I2C work outside the ISR.
- `PollingAlarms` checks both alarm flags without an interrupt input connection.
- `SquareWaveClock` provides a 1 Hz clock on `INT/SQW`.
- `EEPROMReadWrite` writes and reads one NUL-terminated message once at startup.

## API

### Clock and calendar

| Method | Behavior |
|---|---|
| `rtcConnected()` | Probe the configured DS3231 address. |
| `setTime(year, month, day, hour, minute, second)` | Validate and write a date/time in 24-hour mode, then clear the oscillator-stop flag. |
| `getTime(dateTime)` | Validate and read the clock into `ZS042MHDateTime`; leave it unchanged and return `false` for malformed register data. |
| `getTemperature()` | Return degrees Celsius at 0.25-degree resolution, or `NAN` on I2C failure. |
| `oscillatorStopped(stopped)` | Report the DS3231 oscillator-stop flag without clearing it. |
| `isValidDate(...)` | Validate dates from 2000 through 2099. |
| `calculateDayOfWeek(...)` | Return 1 for Sunday through 7 for Saturday, or 0 for an invalid date. |

`ZS042MHDateTime` contains `year`, `month`, `day`, `dayOfWeek`, `hour`, `minute`, and `second`. `setTime()` always writes 24-hour data. `getTime()` returns a 24-hour value and can decode a clock previously configured for 12-hour mode. Years are limited to 2000 through 2099; a set DS3231 century bit is rejected.

A successful read confirms valid register encoding and I2C transport, not that the stored clock is trustworthy. Inspect `oscillatorStopped()` before relying on the value after first power-up or backup-power loss.

### Alarms and square wave

| Method | Behavior |
|---|---|
| `setAlarm1(mode, day, hour, minute, second)` | Configure any Alarm 1 match pattern, enable interrupt mode, and clear its old flag. |
| `setAlarm2(mode, day, hour, minute)` | Configure any Alarm 2 match pattern, enable interrupt mode, and clear its old flag. |
| `disableAlarm(1 or 2)` | Disable the selected alarm and clear its flag. |
| `checkAndClearAlarms(fired)` | Return and clear asserted flags; test `ZS042MH::ALARM_1` and `ZS042MH::ALARM_2`. |
| `setSquareWave(rate)` | Accept `0`, `1`, `1024`, `4096`, or `8192` Hz; `0` selects alarm interrupt mode. |

Alarm modes are:

| Mode | Match behavior | Arguments used |
|---|---|---|
| `ZS042MH_A1_EVERY_SECOND` | Every second | None |
| `ZS042MH_A1_MATCH_SECOND` | Second each minute | `second` |
| `ZS042MH_A1_MATCH_MINUTE_SECOND` | Minute and second each hour | `minute`, `second` |
| `ZS042MH_A1_MATCH_HOUR_MINUTE_SECOND` | Hour, minute, and second each day | `hour`, `minute`, `second` |
| `ZS042MH_A1_MATCH_DATE_HOUR_MINUTE_SECOND` | Date, hour, minute, and second each month | `day`, `hour`, `minute`, `second` |
| `ZS042MH_A1_MATCH_WEEKDAY_HOUR_MINUTE_SECOND` | Weekday, hour, minute, and second each week | `day`, `hour`, `minute`, `second` |
| `ZS042MH_A2_EVERY_MINUTE` | Every minute at second `00` | None |
| `ZS042MH_A2_MATCH_MINUTE` | Minute each hour | `minute` |
| `ZS042MH_A2_MATCH_HOUR_MINUTE` | Hour and minute each day | `hour`, `minute` |
| `ZS042MH_A2_MATCH_DATE_HOUR_MINUTE` | Date, hour, and minute each month | `day`, `hour`, `minute` |
| `ZS042MH_A2_MATCH_WEEKDAY_HOUR_MINUTE` | Weekday, hour, and minute each week | `day`, `hour`, `minute` |

All DS3231 alarm match patterns are exposed. For weekday modes, `day` uses the same numbering as `ZS042MHDateTime::dayOfWeek`: Sunday is `1` and Saturday is `7`. For date modes, `day` is `1` through `31`. Parameters not listed for the selected mode are ignored and are not validated.

The DS3231 `INT/SQW` pin is active-low and open-drain. It remains low while an enabled alarm flag is set. Date alarms repeat monthly, and Alarm 2 has one-minute resolution. Alarm flags are set on a match even when the pin is not connected to an interrupt input or is providing a square wave, so `checkAndClearAlarms()` can be polled in either case.

Switching to a nonzero square wave repurposes `INT/SQW`, so alarms cannot signal through the pin in that mode. Existing alarm-enable bits remain set, but they can drive the pin only when interrupt mode is selected. Square-wave clock rates are `1`, `1024`, `4096`, and `8192` Hz. The output is open-drain and requires a suitable pull-up; verify the module's onboard pull-up voltage before connecting it to another device.

### EEPROM

| Method | Behavior |
|---|---|
| `eepromConnected()` | Probe the configured EEPROM address. |
| `eepromSize()` | Return the configured EEPROM capacity. |
| `eepromWriteByte(address, value)` | Bounds-check and write one byte. |
| `eepromWrite(address, data, length)` | Bounds-check, split at 32-byte pages and Wire-safe chunks, and ACK-poll each write for up to 20 ms. |
| `eepromRead(address, data, length)` | Bounds-check and read in Wire-safe chunks. |
| `eepromFill(address, length, value)` | Fill a checked address range. |

EEPROM calls are blocking. A multi-chunk operation can fail after earlier chunks have completed; there is no rollback or automatic read-back verification. Zero-length reads and writes succeed when the address is within or exactly at the configured capacity. Avoid unnecessary writes because EEPROM has finite endurance.

All Boolean device methods return `false` for invalid arguments or I2C failures. Alarm setup and multi-chunk EEPROM writes can be partially applied if a later I2C operation fails.

## Development and validation

The library is architecture-neutral. Its primary development target is BBC micro:bit V2 with Sandeep Mistry's nRF5 core:

```powershell
arduino-cli config add board_manager.additional_urls https://sandeepmistry.github.io/arduino-nRF5/package_nRF5_boards_index.json
arduino-cli core update-index
arduino-cli core install sandeepmistry:nRF5
arduino-cli compile --fqbn sandeepmistry:nRF5:BBCmicrobitV2 --warnings all --library . examples/ReadWriteTime
```

Portability is also checked on the installed Arduino Uno and Nano ESP32 cores:

```powershell
arduino-cli compile --fqbn arduino:avr:uno --warnings all --library . examples/ReadWriteTime
arduino-cli compile --fqbn arduino:esp32:nano_nora --warnings all --library . examples/ReadWriteTime
```

The host test in `extras/tests/host` uses a fake `TwoWire` bus to verify the exact register bytes, validation paths, and preserved control/status bits for all alarm patterns. The release workflow compiles and runs it with warnings treated as errors.

Open `ZS042MH.code-workspace` in VS Code. `Ctrl+Shift+B` compiles a prompted example path. The other tasks upload an example or refresh `.build/compile_commands.json`. The upload task asks for the micro:bit COM port; the Sandeep Mistry recipe identifies the board from that port and flashes through its CMSIS-DAP interface.

Compile checks cannot validate the attached module, I2C pull-ups, interrupt wiring, backup cell, or EEPROM persistence. Test those behaviors on the exact hardware.

## GitHub releases

Releases are created by `.github/workflows/release.yml`; no release assets need to be built or uploaded manually. To publish one:

1. Set the semantic version in `library.properties` and commit and push that change to `master`.
2. Create and push the matching `v<version>` tag. For example, `version=1.0.0` requires tag `v1.0.0`.
3. Wait for the **Release Arduino library** workflow to complete, then verify the release assets on GitHub.

```powershell
git tag -a v1.0.0 -m "ZS042MH 1.0.0"
git push origin v1.0.0
```

The workflow rejects mismatched or non-semantic versions, runs strict Arduino lint without Library Manager checks, verifies every alarm pattern with the host test, compiles every example for the primary target, and attaches `ZS042MH-<version>.zip` plus `ZS042MH-<version>.zip.sha256` to the GitHub release. Re-running the workflow replaces those assets safely.

Install the attached ZIP in Arduino IDE with **Sketch > Include Library > Add .ZIP Library**. The package contains only the library metadata, source, examples, license, and README under a `ZS042MH` root directory.

## License

MIT
