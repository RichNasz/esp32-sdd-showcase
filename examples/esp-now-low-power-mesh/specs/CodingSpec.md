# ==================== BOARD SELECTION ====================
# Change only this section, then regenerate the project
#
# Board: seeed-xiao-esp32s3          ← ACTIVE / DEFAULT
#
# =======================================================
# Institutional Memory: shared-specs/AIGenLessonsLearned.md — read before regenerating

# Coding Specification — ESP-NOW Low-Power Mesh with WiFi RSSI Sensing

## Architecture

Compile-time three-role firmware. A single Kconfig choice (`NODE_ROLE`) selects
`SENSOR`, `RELAY`, or `GATEWAY` at build time — not at runtime. All three roles share
the same payload struct definition and the same WiFi + ESP-NOW initialization pattern.

**Sensor** nodes wake from deep sleep, run a WiFi scan to collect per-SSID RSSI data,
stop the scan driver, then initialize ESP-NOW on the fixed channel to transmit one
packet. The critical sequencing constraint is that the scan driver must be fully stopped
before ESP-NOW is initialized — both consume the same radio hardware and cannot be
active simultaneously.

**Relay** nodes run continuously in receive mode. On receiving a packet whose
(node_id, msg_count) pair has not been seen before, they increment the hop count and
forward the packet unicast to the gateway. A fixed-size circular dedup table in static
memory prevents relay storms when a packet is overheard by multiple relays.

**Gateway** nodes run continuously. They broadcast a small beacon packet to the ESP-NOW
broadcast address at a fixed interval so that sensor and relay nodes can discover the
gateway MAC address at runtime — no compile-time MAC hardcoding is required. They also
register a receive callback that logs all incoming packets, including hop count and
per-SSID RSSI values.

**Sensor-relay** nodes combine both roles using ESP-IDF power management light sleep.
The radio remains alive in DTIM periodic listen mode — ESP-NOW receive callbacks fire
normally while the CPU is halted. The relay forwarding logic (dedup table, hop increment,
gateway unicast) runs identically to the standalone relay role. A periodic `esp_timer`
fires every `CONFIG_SENSOR_RELAY_SCAN_INTERVAL_SEC` seconds; its callback gives a
FreeRTOS semaphore that app_main's steady-state loop takes to run the scan+send cycle.
During the scan+send window (~2–3 s), the recv_cb is temporarily unregistered —
relay is unavailable for this brief period. State (msg_count, gateway MAC) lives in
static variables, not RTC memory, because the node never deep-sleeps.

## Node Identity (MAC-Derived)

Each node reads its WiFi STA MAC at boot using the ESP-IDF `esp_read_mac` API with the
`ESP_MAC_WIFI_STA` type. The last byte (`mac[5]`) is used as `node_id`. This value is
logged alongside the full MAC address on every boot so operators can cross-reference
node IDs to physical hardware from serial output alone. No Kconfig node ID entry is
needed — do not add one. If `mac[5]` is `0x00` or `0xFF`, fall back to `mac[4]`.

The MAC must be read after WiFi is initialized (after `esp_wifi_init()`) to guarantee
the eFuse-derived MAC is available. Store `node_id` in a local variable for the
duration of the boot cycle; it does not need to be in RTC memory because the MAC is
always the same across resets.

## Gateway Discovery

### Beacon packet format

The `espnow_beacon_t` struct carries two fields: a monotonic sequence number (`seq`)
and a `node_type` byte (`ESPNOW_NODE_GATEWAY = 0` or `ESPNOW_NODE_RELAY = 1`). Receive
callbacks distinguish beacons from data payloads by `data_len` (`sizeof(espnow_beacon_t) == 2`
vs `sizeof(espnow_payload_t) == 8`).

### Two-phase discovery (sensor nodes)

The sensor stores its next-hop MAC in RTC-retained memory so it survives deep sleep
cycles. The cache holds both the MAC (`s_next_hop_mac`) and the type of node it points
to (`s_next_hop_type`: gateway or relay). On first boot, or after a send failure
invalidates the cache, the node runs a two-phase discovery window:

1. Listen on the ESP-NOW channel for the full `ESPNOW_DISCOVERY_TIMEOUT_MS` (2000 ms).
2. On receiving a gateway beacon (`node_type == ESPNOW_NODE_GATEWAY`): record sender MAC,
   set `s_next_hop_type = ESPNOW_NODE_GATEWAY`, give the semaphore immediately.
   The semaphore give unblocks the `xSemaphoreTake` early if the gateway responds quickly,
   but the discovery window still ran for however long it took.
3. On receiving a relay beacon (`node_type == ESPNOW_NODE_RELAY`) while still waiting:
   save the sender MAC to a local `s_relay_candidate[]` buffer (not in RTC memory —
   not persisted across sleep cycles).
4. After the semaphore take times out (full 2000 ms elapsed, no gateway beacon):
   check `s_relay_candidate`. If non-zero, copy it to `s_next_hop_mac`, set
   `s_next_hop_type = ESPNOW_NODE_RELAY`, and log that the node is routing via relay.
5. If neither beacon type was received: log a warning and go to sleep.

Gateway beacons are always preferred. The sensor only falls back to a relay if the
gateway is unreachable within the discovery window.

### Relay and sensor-relay beacon broadcasting

Relay and sensor-relay nodes broadcast their own beacons **only after they have a valid
gateway MAC**. This prevents sensors from routing through a relay that is itself
disconnected from the gateway. The relay beacon timer is started:
- Immediately after successful gateway discovery (within the 2000 ms window), or
- From within the recv_cb on the next gateway beacon, if discovery timed out.

A `s_relay_beacon_started` flag prevents the timer from being started twice. The relay
beacon is sent to the broadcast address (`FF:FF:FF:FF:FF:FF`) every
`ESPNOW_BEACON_INTERVAL_MS`, which must be registered as a peer before the timer starts.

## Packet Routing and Dedup

The relay dedup table is a small circular buffer in static memory (not RTC memory —
it resets on reboot). Each entry holds a (node_id, msg_count) pair. Before forwarding,
the relay scans the table; a match causes a silent drop. On a miss, the relay writes the
pair to the next slot, increments hop_count, and forwards to the gateway. The table size
should be large enough that entries are not evicted before their TTL in the network
expires — sixteen entries is a reasonable starting point given the 60-second sensor cycle.

## Key Constraints

- The WiFi scan driver and ESP-NOW cannot be active simultaneously. The scan must
  complete and the scan driver must be stopped before ESP-NOW initialization. If these
  overlap, ESP-NOW initialization will fail.
- All nodes must operate on the same channel. Configure a fixed channel via Kconfig
  (`CONFIG_ESPNOW_CHANNEL`, default 1) and set it explicitly after WiFi start.
  Channel mismatch is the most common silent failure in ESP-NOW networks.
- The payload struct must be declared `__attribute__((packed))` and validated with a
  compile-time assertion against the 250-byte ESP-NOW maximum payload limit.
- Sensor sleep duration is configurable via Kconfig (`CONFIG_SENSOR_SLEEP_SEC`,
  default 60 seconds).
- Send callback timeout is 200 ms. On expiry, log a warning, invalidate the gateway
  MAC cache, and proceed to sleep. Never block indefinitely.
- All configurable parameters (target SSIDs, ESP-NOW channel, sleep duration) must be
  exposed via Kconfig in `main/Kconfig.projbuild`. Node identity is derived from the
  hardware MAC — do not add a node ID Kconfig entry.

## FreeRTOS Task Architecture

### SENSOR role — no additional tasks

The sensor's wake→scan→send→sleep flow is entirely linear and runs in `app_main`.
No steady-state loop exists, so no separate tasks are required and no watchdog feed
is needed in the loop body. Before entering deep sleep the sensor must remove itself
from the task watchdog (per `shared-specs/coding-conventions.md`). The app_main stack
must be sized to accommodate WiFi scan result buffers and the full ESP-NOW init/deinit
call stack.

- `app_main`: priority 1 (ESP-IDF default). Stack set via `CONFIG_ESP_MAIN_TASK_STACK_SIZE=6144`
  in `sdkconfig.defaults` (the per-sensor role file).

### RELAY role — two tasks

- `app_main`: priority 1. Stack set via `CONFIG_ESP_MAIN_TASK_STACK_SIZE=4096` in
  `sdkconfig.defaults.relay`. Handles init and gateway discovery, then suspends
  indefinitely on a FreeRTOS primitive — `portMAX_DELAY` is acceptable here because
  waiting is its only steady-state job. All forwarding logic runs in the ESP-IDF
  internal ESP-NOW receive callback context. Feeds the task watchdog in its
  steady-state wait loop.
- `mesh_led_task`: priority 2, stack `CONFIG_MESH_LED_TASK_STACK_SIZE` (default 2048).
  Waits on a task notification sent from the receive callback; drives the LED blink
  non-blocking. Running at priority 2 (above app_main) ensures blink responsiveness
  regardless of any other priority-1 activity.

### GATEWAY role — two tasks

- `app_main`: priority 1. Stack set via `CONFIG_ESP_MAIN_TASK_STACK_SIZE=4096` in
  `sdkconfig.defaults.gateway`. Handles init, draws the TUI header, starts the beacon
  `esp_timer`, then enters a steady-state main loop: `ulTaskNotifyTake(pdTRUE, 1000 ms)`.
  The loop drains a FreeRTOS packet queue and calls `print_packet_line()` for each
  entry, then refreshes the status row. This makes `app_main` the sole owner of all
  terminal output — no cursor races possible. Feeds the task watchdog each iteration.
- `mesh_led_task`: priority 2, stack `CONFIG_MESH_LED_TASK_STACK_SIZE` (default 2048).
  Same design as the relay LED task — task notification from receive callback,
  non-blocking blink.

### SENSOR_RELAY role — two tasks

- `app_main`: priority 1, stack 6144 (same as sensor — WiFi scan buffers are allocated
  in app_main context). Handles init, PM configuration, gateway discovery, scan timer
  creation, then enters a steady-state loop: `xSemaphoreTake(s_scan_sem, 5000 ms)`.
  On timeout: feed task watchdog and loop. On semaphore: run scan+send cycle inline
  (unregister recv_cb → deinit ESP-NOW → scan → reinit ESP-NOW → send → re-register
  recv_cb). Light sleep is entered automatically by tickless idle between semaphore waits.
- `mesh_led_task`: priority 2, stack 2048. Same design as relay/gateway — task
  notification from recv_cb, non-blocking blink.

### Stack size rationale

| Role | Task | Stack (bytes) | Reason |
| --- | --- | --- | --- |
| SENSOR | app_main | 6144 | WiFi scan buffers (up to 20 `wifi_ap_record_t`), ESP-NOW init/deinit frames, semaphore handling |
| RELAY | app_main | 4096 | Init frames only; runtime is a blocking wait with no scan buffer on stack |
| GATEWAY | app_main | 4096 | Same as relay — no scan buffer; beacon send frames are shallow |
| RELAY / GATEWAY | mesh_led_task | 2048 | Task notification wait + gpio_set_level + vTaskDelay; well within 2 KB |
| SENSOR_RELAY | app_main | 6144 | WiFi scan buffers same as sensor; scan runs in app_main context |
| SENSOR_RELAY | mesh_led_task | 2048 | Same as relay/gateway |

`CONFIG_ESP_MAIN_TASK_STACK_SIZE` is an ESP-IDF system symbol set in the per-role
`sdkconfig.defaults` file. `CONFIG_MESH_LED_TASK_STACK_SIZE` is a custom Kconfig
entry in `main/Kconfig.projbuild` (see File Layout).

## Light Sleep Configuration (SENSOR_RELAY role only)

Enable `CONFIG_PM_ENABLE=y` and `CONFIG_FREERTOS_USE_TICKLESS_IDLE=y` in
`sdkconfig.defaults.sensor-relay`. These two symbols activate the tickless idle
path in FreeRTOS, which automatically enters light sleep during `vTaskDelay` and
semaphore-wait idle periods. No explicit `esp_light_sleep_start()` call is needed —
the PM subsystem handles entry and exit transparently.

Call `esp_pm_configure()` once after WiFi init with chip-appropriate frequency limits.
In IDF 5.x the struct is `esp_pm_config_t` (unified across all targets):

- `max_freq_mhz`: set to the chip's default max (e.g. 160 for ESP32, 160 for ESP32-S3,
  160 for ESP32-C6). Use `CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ` if available.
- `min_freq_mhz`: 10 MHz minimum for light sleep entry (XTAL oscillator threshold).
- `light_sleep_enable`: `true`.

Add `CONFIG_ESP_WIFI_STA_DISCONNECTED_PM_ENABLE=y` so the radio duty-cycles while
not associated to an AP (ESP-NOW never associates, so this applies throughout runtime).

Do not attempt to use deep sleep in this role — the radio must remain live for
ESP-NOW receive between scan cycles. Deep sleep would kill the radio and break relay.

## WiFi Scan Strategy

The sensor node runs an active WiFi scan for the configured SSIDs before initializing
ESP-NOW. The scan should target all channels with a short per-channel dwell time
(around 100 ms maximum) to keep the total scan window within budget. After the scan,
the driver processes the returned AP records and matches each configured SSID string
against the results. If a configured SSID appears multiple times (multiple BSSIDs),
use the record with the highest RSSI. Unmatched SSID slots in the payload are set to
`INT8_MIN` (–128 dBm), which serves as the sentinel value for "not found".

The scan must be explicitly stopped and the channel reset to `CONFIG_ESPNOW_CHANNEL`
before ESP-NOW initialization. Leaving the radio on whatever channel the scan last
visited will break ESP-NOW delivery for any node operating on a different channel.

## Gateway TUI Display

The gateway renders a real-time terminal user interface using ANSI escape sequences
written directly via `esp_rom_printf`. This section documents the design rules that
must be followed when modifying or extending the display code.

### Why `esp_rom_printf` instead of `printf`

`printf` and `putchar` on ESP-IDF go through the VFS console layer, which enforces
line-buffering regardless of `setvbuf` calls. Bytes are only flushed when a `\n` is
written. `esp_rom_printf` writes directly to the ROM UART routine, bypassing VFS
entirely, so every byte appears on the wire immediately. Use `esp_rom_printf` for all
TUI output. Never mix it with `printf` in the same output path.

### ANSI escape sequence compatibility

Use **DEC save/restore cursor** sequences (`ESC 7` / `ESC 8`), not the SCO variants
(`\033[s` / `\033[u`). macOS Terminal.app silently ignores the SCO sequences, causing
RESTORE to behave as a no-op and leaving the cursor in the wrong position. In C string
literals, write these as `"\0337"` (ESC + ASCII '7') and `"\0338"` (ESC + ASCII '8').
Note: `\0337` is parsed as octal `\033` (three digits: 0, 3, 3 = ESC) followed by
the character `7` — not as four-digit octal `\0337`.

### Line endings in the scroll region

`esp_rom_printf` emits raw bytes. A bare `\n` (LF) advances the cursor one row without
returning to column 0. Always use `\r\n` (CR + LF) when advancing lines in the scroll
region. The `\r` guarantees the cursor returns to column 0 so the next line starts at
the left margin regardless of where the previous content ended.

### Erase-after-write for the status line

When refreshing an in-place row (such as the status line), write the new content
**first**, then erase to end-of-line (`\033[K`). The reverse order — erase then write —
creates a visible blank flash at 115200 baud because the erase is instantaneous but the
subsequent content takes ~8 ms to transmit. With erase-after-write the user sees the
content update in-place with no blank phase; only the unused tail of the row is cleared.

### Single-task screen ownership

All terminal writes must execute in a single task context. The ESP-NOW receive callback
runs in the WiFi task and the beacon timer callback runs in the `esp_timer` task — if
either calls an ANSI cursor-positioning function, the sequences will interleave with the
main task's output and produce garbled cursor movement. The correct pattern:

- Callbacks update shared state variables and call `xTaskNotifyGive` + `xQueueSend`.
- The main task wakes via `ulTaskNotifyTake(pdTRUE, timeout)`, drains the queue, and
  performs all screen writes sequentially.

### Scroll region setup

Set the scroll region with `\033[6;500r` (DECSTBM) immediately after drawing the fixed
header. The large bottom value (500) is a portable idiom meaning "to the end of the
screen" — terminals clamp it to the actual screen height. After setting the scroll
region, position the cursor at the first scrollable row (`\033[6;1H`) and hide it
(`\033[?25l`, DECTCEM off). Absolute cursor positioning (`\033[R;CH`) continues to
work outside the scroll region as long as DECOM (origin mode) is not set, which is the
default.

### Pre-format with snprintf; output with esp_rom_printf

`esp_rom_printf` on RISC-V ESP32 targets (C6, H2) does not reliably support width and
precision format specifiers such as `%-14s` or `%02u`. Pre-format all content into a
`char` buffer with `snprintf`, then pass the buffer to `esp_rom_printf` using only
`"%s"` or `"%s\r\n"` as the format string. This also avoids `uint32_t` vs `unsigned
int` width warnings: cast `uint32_t` arguments to `(unsigned)` when passing to
`snprintf`.

## Per-Role sdkconfig.defaults Files

Generate three sdkconfig files so that each role can be built cleanly without modifying
source files:

- `sdkconfig.defaults` — common settings plus sensor role (`CONFIG_NODE_ROLE_SENSOR=y`).
  This is the default build used by automated tests.
- `sdkconfig.defaults.relay` — relay role override only.
- `sdkconfig.defaults.gateway` — gateway role override only.
- `sdkconfig.defaults.sensor-relay` — sensor-relay role override including light sleep
  PM symbols (`CONFIG_PM_ENABLE`, `CONFIG_FREERTOS_USE_TICKLESS_IDLE`,
  `CONFIG_ESP_WIFI_STA_DISCONNECTED_PM_ENABLE`).

The `SDKCONFIG_DEFAULTS` environment variable accepted by idf.py supports semicolon-
separated lists, allowing the base file and a role override to be layered without
editing either.

## Build, Flash, and Monitor Reference

This section documents the exact commands for every role × board combination. All
commands must be run from within `examples/esp-now-low-power-mesh/`.

### Prerequisites

Before the first build for a given role or after switching chip targets, always clear
the cached sdkconfig. Failing to do so will silently use the previous role's settings:

```sh
rm -f sdkconfig sdkconfig.old
```

Set the correct chip target **once** per target switch. This writes CMake cache files;
running it again with the same target is a no-op:

```sh
SDKCONFIG_DEFAULTS="<layer1>;<layer2>;<layer3>" idf.py set-target <chip>
```

### SDKCONFIG_DEFAULTS layer order

Always specify layers in this order: `common ; board ; role`. The last file wins on
any key that appears in multiple files.

| Role | Board | SDKCONFIG_DEFAULTS value |
| --- | --- | --- |
| SENSOR (default) | Adafruit HUZZAH32 (esp32) | `"sdkconfig.defaults;sdkconfig.defaults.esp32"` |
| SENSOR (default) | ESP32-C6-DevKitC-1-N8 | `"sdkconfig.defaults;sdkconfig.defaults.esp32c6"` |
| RELAY | Adafruit HUZZAH32 (esp32) | `"sdkconfig.defaults;sdkconfig.defaults.esp32;sdkconfig.defaults.relay"` |
| RELAY | ESP32-C6-DevKitC-1-N8 | `"sdkconfig.defaults;sdkconfig.defaults.esp32c6;sdkconfig.defaults.relay"` |
| GATEWAY | Adafruit HUZZAH32 (esp32) | `"sdkconfig.defaults;sdkconfig.defaults.esp32;sdkconfig.defaults.gateway"` |
| GATEWAY | ESP32-C6-DevKitC-1-N8 | `"sdkconfig.defaults;sdkconfig.defaults.esp32c6;sdkconfig.defaults.gateway"` |
| SENSOR_RELAY | Adafruit HUZZAH32 (esp32) | `"sdkconfig.defaults;sdkconfig.defaults.esp32;sdkconfig.defaults.sensor-relay"` |
| SENSOR_RELAY | ESP32-C6-DevKitC-1-N8 | `"sdkconfig.defaults;sdkconfig.defaults.esp32c6;sdkconfig.defaults.sensor-relay"` |

The same layering pattern applies to any board listed in the Supported Boards table.
Substitute the appropriate `sdkconfig.defaults.<board>` file for your hardware.

### Build command

After `set-target`, build with the same `SDKCONFIG_DEFAULTS` value. Example for
gateway on ESP32-C6:

```sh
SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32c6;sdkconfig.defaults.gateway" \
  idf.py build
```

### Flash command

Identify the serial port for your connected device before flashing. Port names vary
by OS and USB adapter:

| OS | Typical format | Example |
| --- | --- | --- |
| macOS (USB-UART bridge, e.g. CP2104) | `/dev/cu.usbserial-XXXXXXXX` | `/dev/cu.usbserial-01611D4D` |
| macOS (native USB Serial/JTAG) | `/dev/cu.usbmodem<N>` | `/dev/cu.usbmodem1101` |
| Linux | `/dev/ttyUSBN` or `/dev/ttyACMN` | `/dev/ttyUSB0` |
| Windows | `COMN` | `COM3` |

Run `ls /dev/cu.*` (macOS) or `ls /dev/tty*` (Linux) before and after plugging in the
device to identify the correct port. On Windows, use Device Manager.

Flash command (substitute your port):

```sh
SDKCONFIG_DEFAULTS="<layers>" idf.py -p /dev/cu.usbserial-XXXXXXXX flash
```

**Do not combine `flash` and `monitor` in a single `idf.py flash monitor` command from
automated tooling** — `idf.py monitor` requires an interactive TTY. Flash first, then
open the monitor separately in a terminal.

### Monitor command

Open the serial monitor in a terminal after flashing:

```sh
idf.py -p /dev/cu.usbserial-XXXXXXXX monitor
```

Exit the monitor with `Ctrl-]`.

### Switching roles or targets (full reset procedure)

When reflashing a board with a different role or a different chip target:

```sh
rm -f sdkconfig sdkconfig.old
SDKCONFIG_DEFAULTS="<new layers>" idf.py set-target <new chip>
SDKCONFIG_DEFAULTS="<new layers>" idf.py build
idf.py -p /dev/cu.usbserial-XXXXXXXX flash
```

The `set-target` call regenerates the CMake cache for the new chip. Skipping it after
a target change produces a build that silently targets the wrong chip and will fail
to flash.

## Preferred Libraries and APIs

Use the built-in ESP-NOW API. It operates below the IP stack and requires no AP
association, DHCP, or TCP — ideal for low-power peer-to-peer messaging where full
WiFi connectivity would be excessive overhead.

Use a FreeRTOS binary semaphore to synchronize the ESP-NOW send callback with the
main task. This avoids busy-waiting and prevents the race condition that a global
`volatile bool` flag introduces when the callback fires before the wait starts.

Use a FreeRTOS task notification to drive the gateway and relay LED blinks from a
dedicated low-priority task. The receive callback must not be delayed by LED timing.

## Non-Functional Requirements

- Sensor must call `gpio_hold_dis()` on the LED GPIO before driving it after waking
  from deep sleep. Pad state may be held from the previous cycle.
- Relay and gateway LED blinks must be non-blocking. The receive callback must return
  as fast as possible — never block in a receive callback.
- `esp_now_send()` failure must be logged; never use `ESP_ERROR_CHECK` on it. A failed
  send is a recoverable event and must not abort the device.
- Relay must not hold any FreeRTOS semaphore or mutex across the receive callback.
  Callbacks execute in a system task context with limited stack.

## Gotchas

- **WiFi scan before ESP-NOW**: calling `esp_now_init()` while a WiFi scan is active
  will fail. The scan must be fully stopped first.
- **Channel must be set explicitly**: after WiFi start, set the channel to
  `CONFIG_ESPNOW_CHANNEL` before any ESP-NOW operation. Do not assume the channel
  remains at the desired value after a scan.
- **Peer registration before send**: `esp_now_send()` to an unregistered peer returns
  an error that is easy to confuse with a channel mismatch. Register all destination
  MACs before sending — including the broadcast address used for gateway beacons.
- **WIFI_MODE_STA required**: `WIFI_MODE_NULL` is insufficient for ESP-NOW on
  ESP32-S3, even when no AP association is made.
- **nvs_flash_init() before WiFi**: omitting this call causes WiFi init to fail
  because PHY calibration data cannot be stored. See `shared-specs/AIGenLessonsLearned.md`.
- **RTC memory initialization**: on power-on reset, explicitly validate the RTC-stored
  gateway MAC with a known magic number stored alongside it, rather than assuming
  all-zeros means "not discovered". On a warm reset the cache may contain stale data.

## File Layout (non-standard files)

- `main/Kconfig.projbuild` — `NODE_ROLE` choice (SENSOR / RELAY / GATEWAY /
  SENSOR_RELAY), `ESPNOW_CHANNEL` integer (default 1, range 1–13),
  `SENSOR_SLEEP_SEC` integer (default 60, range 10–3600),
  `SENSOR_RELAY_SCAN_INTERVAL_SEC` integer (default 60, range 10–3600),
  `TARGET_SSID_1` / `TARGET_SSID_2` / `TARGET_SSID_3` strings (up to 32 chars,
  empty string means disabled), `MESH_LED_TASK_STACK_SIZE` integer (default 2048,
  range 1024–8192). No `NODE_ID` entry — node identity is MAC-derived.
- `sdkconfig.defaults` — common + sensor role defaults.
- `sdkconfig.defaults.relay` — relay role override.
- `sdkconfig.defaults.gateway` — gateway role override.
- `sdkconfig.defaults.sensor-relay` — sensor-relay role override with light sleep config.
- `main/sensor_relay.c` + `main/sensor_relay.h` — SENSOR_RELAY role implementation.
