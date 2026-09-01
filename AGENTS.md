# Arduino library guidance

This repository is an Arduino library for ZS-042/MH modules containing a DS3231 RTC and AT24C32 EEPROM.

## Design boundaries

- Keep device access in the `ZS042MH` class and use its injected `TwoWire` reference.
- Keep serial consoles, application loops, and board-specific interrupt handlers in examples or consuming sketches.
- Return `false` for invalid arguments and I2C failures. Preserve unrelated register bits in read-modify-write operations.
- Do not perform I2C, serial, delay, or other substantial work in interrupt service routines.
- Keep EEPROM operations inside the configured capacity and split writes at AT24C32 page boundaries.
- Keep every DS3231 alarm mask pattern available. Date modes use `DY/DT=0` with days `1..31`; weekday modes use `DY/DT=1` with Sunday `1` through Saturday `7`.
- Keep alarm mode names and declaration order aligned with the fields matched by the DS3231 mask progression.
- Keep alarm flags pollable without an interrupt input and while `INT/SQW` is configured as a clock output.

## Validation

Compile every example for the primary test target:

```powershell
Get-ChildItem examples -Directory | ForEach-Object { arduino-cli compile --fqbn sandeepmistry:nRF5:BBCmicrobitV2 --warnings all --library . $_.FullName }
```

Run strict library and example lint from the repository root:

```powershell
arduino-lint --compliance strict
```

Compile and run the host alarm-register test with a C++11 compiler:

```bash
mkdir -p .build
g++ -std=c++11 -Wall -Wextra -Werror -I extras/tests/host -I src src/ZS042MH.cpp extras/tests/host/alarm_modes.cpp -o .build/alarm_modes_test
./.build/alarm_modes_test
```

The `.build` directory is ignored by Git.

When portability-sensitive code changes, also compile every example for the installed AVR Uno and Nano ESP32 targets:

```powershell
Get-ChildItem examples -Directory | ForEach-Object { arduino-cli compile --fqbn arduino:avr:uno --warnings all --library . $_.FullName }
Get-ChildItem examples -Directory | ForEach-Object { arduino-cli compile --fqbn arduino:esp32:nano_nora --warnings all --library . $_.FullName }
```

Compilation does not verify physical wiring, clone-specific pull-ups, alarm signaling, EEPROM persistence, or the backup cell.

## Releases

- Publish downloadable packages only through GitHub Releases; do not submit this library to Arduino Library Manager or `arduino/library-registry`.
- `.github/workflows/release.yml` is the source of truth for release validation, packaging, and publication.
- Keep `extras/tests/host` aligned with the alarm API and register masks; the release workflow must run it before publishing.
- Install Arduino CLI in the workflow from a pinned official release archive and verify it against Arduino's published checksum; do not use actions that depend on deprecated Node runtimes.
- Keep `library.properties` at a three-part semantic version and tag the release commit with the exact corresponding `v<version>` tag.
- Push the version commit to `master` before pushing its tag. The tag triggers the release workflow.
- Keep generated ZIP and checksum files out of Git. The workflow packages `library.properties`, `keywords.txt`, `LICENSE`, `README.md`, `src`, and `examples` beneath a `ZS042MH` archive root.
- A release is complete only after its workflow passes and both `ZS042MH-<version>.zip` and `ZS042MH-<version>.zip.sha256` appear on the GitHub release.
