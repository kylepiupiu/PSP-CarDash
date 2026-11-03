# Hardware Notes

## PSP

The stable reference has been tested on PSP-3000-series hardware running homebrew.

The PSP Wi-Fi stack predates modern WPA2/WPA3 environments. The reference ESP32 firmware therefore creates an open 2.4 GHz access point for compatibility.

## ESP32

Reference board:

- ESP32-S3 DevKitC-1
- 2.4 GHz SoftAP
- UDP broadcast at 10 Hz

No additional hardware is required for v0.3 because telemetry is simulated.

## Vehicle interface

Do not wire an ESP32 GPIO directly to a vehicle diagnostic/K-line/CAN pin.

A real vehicle adapter requires the correct physical-layer transceiver plus appropriate electrical protection. The exact W210 interface will be selected only after the connector/protocol is verified on the reference car.
