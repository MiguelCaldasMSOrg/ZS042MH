# Arduino library guidance

This repository is an Arduino library for ZS-042/MH modules containing a DS3231 RTC and AT24C32 EEPROM.

## Design boundaries

- Keep device access in the `ZS042MH` class and use its injected `TwoWire` reference.
- Keep serial consoles, application loops, and board-specific interrupt handlers in examples or consuming sketches.
- Return `false` for invalid arguments and I2C failures. Preserve unrelated register bits in read-modify-write operations.
- Do not perform I2C, serial, delay, or other substantial work in interrupt service routines.
- Keep EEPROM operations inside the configured capacity and split writes at AT24C32 page boundaries.

## Validation

Compile every example for the primary test target:

```powershell
Get-ChildItem examples -Directory | ForEach-Object { arduino-cli compile --fqbn sandeepmistry:nRF5:BBCmicrobitV2 --warnings all --library . $_.FullName }
```

Compilation does not verify physical wiring, clone-specific pull-ups, alarm signaling, EEPROM persistence, or the backup cell.

## Session handoff (2026-08-31)

### Current state

- The Arduino library name is `ZS042MH`; `library.properties` and the primary header `src/ZS042MH.h` now match.
- The directory is `C:\Projects\Arduino\ZS042MH` and the workspace file is `ZS042MH.code-workspace`; both manual renames are complete.
- `README.md` references the renamed workspace file. The workspace file uses `"path": "."`, and VS Code tasks use `${workspaceFolder}`, so their contents did not require path changes.
- This directory is a Git repository on `master`, with `origin` at `https://github.com/MiguelCaldasMSOrg/ZS042MH.git`.
- The public repository is published at `https://github.com/MiguelCaldasMSOrg/ZS042MH`, matching the URL declared in `library.properties`.

### Implementation

- `src/ZS042MH.h` and `src/ZS042MH.cpp` contain the reusable device layer extracted from `C:\Projects\Arduino\ArduinoBBCMicroBitV2RTC\ArduinoBBCMicroBitV2RTC.ino`.
- `ZS042MH` accepts an injected `TwoWire` reference and configurable RTC address, EEPROM address, and EEPROM size. Defaults are DS3231 `0x68`, AT24C32 `0x57`, and 4096 bytes.
- Public functionality includes RTC/device probes, validated date/time reads and writes, temperature, oscillator-stop status, both alarms, alarm flag clearing, square-wave selection, and bounded EEPROM read/write/fill operations.
- The library deliberately excludes the source sketch's serial command parser, printing helpers, ISR, `setup()`, and `loop()`.
- Examples are `ReadWriteTime`, `AlarmInterrupt`, and `EEPROMReadWrite`. The alarm example keeps I2C outside interrupt context; the EEPROM example writes only once during setup.
- Supporting files include `library.properties`, `keywords.txt`, `LICENSE`, `README.md`, `.vscode` tasks/extensions, `.gitignore`, and the VS Code workspace file.

### Verified results

- Arduino CLI version used earlier in setup: 1.5.1.
- Primary board/core: Sandeep Mistry nRF5 0.8.0 with FQBN `sandeepmistry:nRF5:BBCmicrobitV2`.
- All three examples compile successfully with warnings enabled using the command in the Validation section.
- All three examples also compile with warnings enabled for Arduino Uno (`arduino:avr:uno`) and Nano ESP32 (`arduino:esp32:nano_nora`).
- VS Code diagnostics reported no errors in the created workspace files.
- `arduino-lint` 1.3.0 is installed at `C:\Program Files\Arduino CLI\arduino-lint.exe`.
- Run strict lint from the library root as `arduino-lint --compliance strict`, without a trailing `.`. Passing `.` causes rule LS003 to treat the literal dot as the folder name.
- Latest strict lint result: the library and all examples pass with no errors or warnings.
- Hardware behavior has not been tested because no connected module was available during setup.

### Next steps

1. Before a release, test RTC detection/timekeeping, oscillator-stop reporting, alarm signaling on the exact board, alternate EEPROM addressing if applicable, and EEPROM read-back persistence.