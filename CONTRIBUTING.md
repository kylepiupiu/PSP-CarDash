# Contributing

Contributions are welcome, especially real-hardware testing.

## Good contributions

- PSP model compatibility reports
- ESP32 board compatibility reports
- Fixes that preserve the stable PSP runtime
- Vehicle protocol adapters
- Read-only OBD/K-line integrations
- UI improvements
- Documentation

## Test reports

Please include:

1. PSP model
2. firmware / custom firmware
3. PSPDEV version/build environment
4. ESP32 board and Arduino core version
5. exact steps
6. expected result
7. actual result
8. logs or photos when useful

## Stability rule

The stable v0.3 PSP boot/display/network architecture has been validated on real hardware. Avoid replacing that entire stack unless there is a strong reason and hardware test evidence.

## Vehicle safety

Do not submit code that sends vehicle-control commands by default. Initial vehicle support should be read-only telemetry.
