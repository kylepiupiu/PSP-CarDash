# Roadmap

## Completed

- Stable PSP homebrew boot/display/input baseline
- Four-page dashboard
- Local simulation mode
- ESP32-S3 open 2.4 GHz SoftAP
- UDP 4210 link
- PCD1 telemetry protocol
- Automatic fallback to Demo mode

## In progress

### High-fidelity UI

Replace the engineering-style text dashboard with a smooth 480×272 raster UI while retaining the proven v0.3 startup/network core.

This work remains experimental until it is validated on real PSP hardware.

### W210 vehicle interface

Reference vehicle: Mercedes-Benz W210 E430 / M113 V8 4.3L.

Before implementing the vehicle adapter, confirm:

- exact model year / market
- 16-pin OBD-II connector population
- presence of Mercedes 38-pin diagnostic connector
- physical layer (ISO 9141 / K-line / KWP2000 as applicable)
- which parameters are actually available

Initial vehicle integration should be read-only.

## Later

- Real RPM / speed / coolant / throttle / system voltage
- DTC reading
- Trip logging
- 0–100 timing from validated speed data
- Multiple vehicle profiles
- Configurable themes
