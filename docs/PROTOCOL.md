# PCD1 Telemetry Protocol

PSP CarDash v0.3 uses a deliberately simple ASCII UDP protocol so it can be inspected with a serial terminal, packet capture or a basic desktop script.

## Transport

- ESP32-S3 SoftAP
- SSID: \`PSP-CAR-DASH\`
- Security: open 2.4 GHz network
- ESP32 AP IP: \`192.168.4.1\`
- Broadcast: \`192.168.4.255:4210\`
- Default send rate: 10 Hz

## Packet

\`\`\`text
PCD1,seq,speed,rpm,coolant,throttle,voltage10,intake,load,fuel10,oiltemp,oilpress10
\`\`\`

Example:

\`\`\`text
PCD1,145,86,3250,92,27,142,31,38,68,96,38
\`\`\`

## Fields

| Field | Meaning | Unit / encoding |
|---|---|---|
| \`seq\` | Packet sequence | integer |
| \`speed\` | Vehicle speed | km/h |
| \`rpm\` | Engine speed | r/min |
| \`coolant\` | Coolant temperature | °C |
| \`throttle\` | Throttle position | % |
| \`voltage10\` | System voltage × 10 | 142 = 14.2 V |
| \`intake\` | Intake air temperature | °C |
| \`load\` | Engine load | % |
| \`fuel10\` | Fuel consumption × 10 | 68 = 6.8 L/100 km |
| \`oiltemp\` | Oil temperature | °C |
| \`oilpress10\` | Oil pressure × 10 | 38 = 3.8 bar |

## Current status

v0.3 sends simulated data from the ESP32. Future vehicle adapters can populate the same PCD1 fields from validated OBD/K-line data without changing the PSP transport layer.
