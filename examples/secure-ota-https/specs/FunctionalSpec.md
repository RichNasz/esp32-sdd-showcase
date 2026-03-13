# Secure OTA via HTTPS — Button-Triggered Partition Flip Demo

## Overview

Minimal visual demonstration of ESP32-S3 OTA using dual partitions.

The device maintains two firmware images in flash:
- "Program 1" — lights left LED only
- "Program 2" — lights right LED only

Both images are functionally identical except for which status LED they illuminate.

Pressing the button triggers an HTTPS OTA download from a fixed server URL.  
The new binary is written to the **inactive** OTA partition.  
On success the device reboots → runs the newly written image → lights the opposite status LED.  
This creates a clear visual flip between left and right LEDs each time the button is pressed.

A middle LED indicates OTA activity (blinking during transfer) and errors (distinct pattern on failure).

## Supported Boards

- YEJMKJ ESP32-S3-DevKitC-1-N16R8 (default )

## Requirements

- Connect to Wi-Fi (credentials via Kconfig)
- Fetch firmware binary via HTTPS using `esp_https_ota`
- Validate server TLS certificate against bundled CA (PEM embedded at build time)
- Write to inactive OTA partition
- Reboot on success
- On first boot after OTA: call `esp_ota_mark_app_valid_cancel_rollback()` early
- Rollback occurs automatically if new image does not mark itself valid
- Three external status LEDs + one button
- Log progress and state changes over serial
- Blink middle LED during OTA; show error pattern on failure

## Hardware Dependencies

- Board: ESP32-S3-DevKitC-1-N16R8
- External components:
  - 3× LEDs (active high) + current-limiting resistors (~220–330 Ω)
  - 1× push button (connects GPIO to GND when pressed)

## LED & Button Pin Assignments

| Visual Position | Function                          | GPIO | Header approx. location | Notes                     |
|-----------------|-----------------------------------|------|--------------------------|---------------------------|
| Left            | Program 1 active (ota_0)          | 4    | J1 (left header)         | Steady ON when active     |
| Middle          | OTA / Transition / Error          | 5    | J1                       | Blinks during OTA         |
| Right           | Program 2 active (ota_1)          | 6    | J1                       | Steady ON when active     |
| Button          | Trigger OTA (press → GND)         | 7    | J1                       | Internal pull-up          |

## Connectivity

- Wi-Fi STA mode, WPA2-Personal
- HTTPS only
- Server CA certificate embedded at build time

## Success Criteria

- Button press → middle LED blinks → device reboots → opposite status LED lights up
- Server certificate verified; invalid cert prevents download
- After successful OTA, new image marks itself valid → no rollback
- Rollback test: image without `mark_app_valid` call reverts on next boot
- No corruption after multiple flips
- Clear visual indication of which program is running at all times