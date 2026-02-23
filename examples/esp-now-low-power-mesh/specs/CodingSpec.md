# ==================== BOARD SELECTION ====================
# Change only this section, then regenerate the project
#
# Board: seeed-xiao-esp32s3          ← ACTIVE / DEFAULT
#
# =======================================================
# Institutional Memory: shared-specs/AIGenLessonsLearned.md — read before regenerating

# Coding Specification — ESP-NOW Low-Power Mesh

## Architecture

Compile-time dual-role firmware. A single Kconfig choice (NODE_ROLE_SENSOR vs NODE_ROLE_GATEWAY)
selects the active role at build time — not at runtime. Sensor nodes wake, transmit one ESP-NOW
packet, wait for the send-callback acknowledgement via a semaphore, blink the LED, and enter
deep sleep. The gateway node is always-on, registers a receive callback, and logs incoming
packets with an LED blink per packet. Both roles share the same Wi-Fi and ESP-NOW initialization
sequence.

## Key Constraints

- ESP-NOW requires Wi-Fi to be initialized in STA mode even though no AP association occurs.
  This is an ESP-IDF requirement; omitting Wi-Fi initialization causes ESP-NOW init to fail.
- Fixed channel: all nodes must operate on the same Wi-Fi channel. Channel mismatch is the
  most common cause of delivery failures in ESP-NOW networks.
- Payload: a compact packed struct (node_id, msg_count, fake_temp) constrained to ≤ 250 bytes
  (the ESP-NOW maximum). A compile-time static assert enforces this at build time.
- Sensor sleep duration: 20 seconds.
- Send acknowledgement timeout: 200 ms maximum before the sensor logs a warning and sleeps.
- All configurable parameters (gateway MAC address, node ID) must be exposed via Kconfig.

## Preferred Libraries and APIs

Use the built-in ESP-NOW API (esp_now.h). It operates below the IP stack and requires no AP
association, DHCP, or TCP session — making it ideal for low-power peer-to-peer messaging where
full Wi-Fi connectivity would be excessive overhead.

Use a FreeRTOS binary semaphore to synchronise the ESP-NOW send callback with app_main. This
avoids busy-waiting and prevents the race conditions that a global flag would introduce.

## Non-Functional Requirements

- Sensor nodes must deinitialize ESP-NOW and stop Wi-Fi cleanly before entering deep sleep.
  Leaving the radio active defeats the power budget of the duty-cycled sensor pattern.
- Gateway LED blink must be non-blocking. The receive callback must not be delayed by LED
  timing — use a task notification to drive the blink from a separate task.
- esp_now_send() failure must be logged and the device must proceed to sleep. Do not use
  ESP_ERROR_CHECK on the send call — a failed send is recoverable and must not abort the device.

## Gotchas

- ESP-NOW peers must be registered before calling esp_now_send(). Sending to an unregistered
  peer returns an error that is easy to misread as a channel problem.
- Wi-Fi mode must be WIFI_MODE_STA for ESP-NOW to function on ESP32-S3. WIFI_MODE_NULL is
  insufficient even though no AP association is made.
- The gateway MAC Kconfig string must be parsed into a byte array at runtime. MAC addresses
  provided in the wrong format (wrong separator, wrong case) silently produce incorrect peer
  registration rather than an error.
- gpio_hold_dis() must be called on the sensor node before driving the LED GPIO after deep
  sleep, as pad state may be held from the previous cycle.

## File Layout (non-standard files)

- main/Kconfig.projbuild — NODE_ROLE choice, GATEWAY_MAC string, NODE_ID integer
