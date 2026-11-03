# PSP CarDash

[English](README.md) | [简体中文](README.zh-CN.md) | [日本語](README.ja.md)

<p align="center"><img src="assets/icon.jpg" width="180" alt="PSP CarDash icon"></p>

Turn a Sony PSP into a real-time vehicle dashboard using an ESP32 as the telemetry bridge.

> **Stable baseline: v0.3 ESP32 Link**
>
> Reference vehicle: **Mercedes-Benz W210 E430 / M113 V8 4.3L**
>
> Real vehicle OBD/K-line integration is not enabled yet. The current stable release uses simulated telemetry over ESP32 → Wi-Fi → UDP → PSP.

## Why this project

Old PSP hardware has a good screen, physical controls and enough networking capability to become a surprisingly useful dedicated vehicle display. PSP CarDash keeps the PSP side lightweight and uses an ESP32-S3 as the bridge between modern sensors/vehicle interfaces and the PSP.

~~~text
Vehicle / OBD (future)
        │
     ESP32-S3
        │
   2.4 GHz Wi-Fi
        │
     UDP 4210
        │
       PSP
~~~

## Current stable features

The v0.3 baseline has been tested on real PSP hardware and supports:

- Four pages: Drive / Engine / Trip / Diagnostics
- ESP32-S3 SoftAP link
- UDP telemetry at 10 Hz
- Local demo mode when ESP32 is unavailable
- Runtime switching between Demo and ESP32 data
- PSP Wi-Fi profile selection
- Graceful fallback if the ESP32 link drops
- W210 E430 / M113 reference profile

## Hardware

### Required

- Sony PSP with homebrew support
- ESP32-S3 development board
- USB cable for ESP32 flashing

### Reference development hardware

- PSP-3000 series
- ESP32-S3 DevKitC-1
- 2.4 GHz open SoftAP for PSP compatibility

The PSP Wi-Fi stack is old. For maximum compatibility, the reference ESP32 firmware creates an **open 2.4 GHz access point**:

~~~text
SSID: PSP-CAR-DASH
AP IP: 192.168.4.1
UDP: 4210
~~~

## Repository layout

~~~text
PSP-CarDash/
├── psp/            PSP homebrew source and Makefile
├── esp32/          ESP32-S3 telemetry bridge
├── tools/          Desktop UDP simulator
├── docs/           Protocol, hardware notes and roadmap
├── assets/         Project artwork
└── experimental/   Work-in-progress UI experiments
~~~

## PSP controls

| Control | Action |
|---|---|
| Left / Right | Switch dashboard page |
| Start | Toggle local demo animation |
| Select | Freeze / unfreeze values |
| L / R trigger | Select PSP Wi-Fi profile while disconnected |
| Triangle | Connect / disconnect ESP32 Link |
| Circle | Exit |
| Home | Standard PSP system exit |

## Quick start

### 1. Build the PSP application

Install [PSPDEV](https://pspdev.github.io/) and make sure `psp-gcc` is available.

~~~bash
export PSPDEV=/opt/pspdev
export PATH="$PSPDEV/bin:$PATH"

cd psp
make clean
make
~~~

Copy `psp/EBOOT.PBP` to:

~~~text
/PSP/GAME/PSPCARDASH/EBOOT.PBP
~~~

### 2. Flash the ESP32

Open `esp32/PSP_CAR_DASH_ESP32_LINK.ino` with Arduino IDE / Arduino CLI and flash it to the ESP32-S3.

The ESP32 creates:

~~~text
SSID: PSP-CAR-DASH
Security: OPEN
UDP: 4210
Rate: 10 Hz
~~~

### 3. Create the PSP Wi-Fi profile

On the PSP XMB:

~~~text
Settings
 -> Network Settings
 -> Infrastructure Mode
 -> New Connection
 -> Scan
 -> PSP-CAR-DASH
 -> Security: None
 -> Save
~~~

Launch PSP CarDash, select the saved Wi-Fi profile using L/R, then press **Triangle**.

A working link should show on the Diagnostics page:

~~~text
AP STATE   4 / 4
PSP IP     192.168.4.x
UDP PORT   4210
PACKETS    increasing
~~~

## Telemetry protocol

v0.3 uses a deliberately simple ASCII packet:

~~~text
PCD1,seq,speed,rpm,coolant,throttle,voltage10,intake,load,fuel10,oiltemp,oilpress10
~~~

Example:

~~~text
PCD1,145,86,3250,92,27,142,31,38,68,96,38
~~~

See [docs/PROTOCOL.md](docs/PROTOCOL.md).

## Desktop testing

You can test the PSP receiver without ESP32 hardware:

~~~bash
python3 tools/pcd_sender.py
~~~

## Project status

### Stable — v0.3 ESP32 Link

- PSP runtime: tested
- Page navigation: tested
- ESP32 SoftAP: tested
- UDP telemetry: tested
- Demo fallback: tested

### Experimental — high-fidelity Render UI

A smooth 480×272 raster dashboard UI is under development. It is intentionally kept out of the stable source until it passes the same real-hardware validation.

See [experimental/render-ui/README.md](experimental/render-ui/README.md).

## Roadmap

- [x] PSP stable runtime
- [x] ESP32-S3 Wi-Fi bridge
- [x] UDP telemetry protocol
- [x] Local demo/fallback mode
- [ ] High-fidelity rendered dashboard UI
- [ ] Verify W210 diagnostic connector and physical layer
- [ ] W210 standard OBD-II / K-line telemetry
- [ ] Real RPM / speed / coolant / throttle / voltage
- [ ] DTC diagnostics
- [ ] Trip recording
- [ ] Additional vehicle profiles

## Vehicle integration safety

The current stable code does **not** transmit vehicle control commands.

Future vehicle support should start as read-only telemetry. Verify connector pinout, protocol, voltage levels and protection/isolation before attaching experimental hardware to a vehicle diagnostic or communication bus.

## Contributing

Issues, real-hardware test reports, protocol adapters and UI improvements are welcome.

See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

Code is released under the MIT License. See [LICENSE](LICENSE).

## Trademark notice

This is an independent, unofficial hobby project. Sony, PlayStation Portable / PSP, Mercedes-Benz, W210 and related names/logos are trademarks of their respective owners. No affiliation or endorsement is implied.
