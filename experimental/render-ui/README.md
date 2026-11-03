# Experimental Render UI

Status: **not yet promoted to stable**.

The goal is a smooth, image-quality 480×272 dashboard inspired by classic Mercedes instrumentation:

- black/gold visual system
- analog-style RPM / temperature areas
- large central speed display
- W210 reference-car artwork
- live ESP32 status and telemetry overlays

The experimental renderer is intentionally kept separate from the stable v0.3 source because earlier attempts that replaced the proven PSP display path caused black screens on real hardware.

Current rule:

> Keep the v0.3 boot/network core frozen. Change only the presentation layer, then validate on real PSP hardware before promotion.

Once the high-fidelity UI passes real-device testing, it will become the next stable release.
