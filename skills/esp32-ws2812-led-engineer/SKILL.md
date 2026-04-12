---
name: esp32-ws2812-led-engineer
description: Generates the canonical WS2812B RMT driver block for ESP32 examples. Emits timing constants, ws2812_write(), RMT init sequence, Kconfig symbol, CMakeLists dependency, and per-target sdkconfig.defaults fragments. Handles primary/secondary board pattern for same-chip multi-board support.
version: "1.0.0"
author: Richard Naszcyniec
keywords: sdd, esp32, ws2812, rgb, led, rmt, neopixel, addressable-led, multi-board
---

<!-- ================================================
     AGENT-GENERATED — DO NOT EDIT BY HAND
     Generated from specs/ using esp32-sdd-documentation-generator skill
     Date: 2026-04-12 | Agent: Claude Code
     ================================================ -->

# ESP32 WS2812 LED Engineer

## When to use
Any time a FunctionalSpec lists a board with a WS2812 RGB LED (e.g. Espressif
ESP32-C6-DevKitC-1-N8 GPIO 8, YEJMKJ ESP32-S3-DevKitC-1-N16R8 GPIO 48). Invoked
automatically as a sub-workflow by `esp32-sdd-full-project-generator` when a WS2812 LED
is identified in any referenced board-spec. Can also be run standalone to add WS2812
support to an existing example.

## Workflow (follow exactly)

1. **Read board specs** — for each board listed in the FunctionalSpec that has a WS2812
   LED, read its `board-specs/<vendor>/<board>.md`. Extract: GPIO number, SoC target name
   (`idf.py set-target` value), and console config (USB CDC, USB Serial/JTAG, or UART
   bridge). Confirm the LED type is WS2812/WS2812B (single-wire NRZ, GRB byte order) —
   this skill is not suitable for APA102 (SPI) or other addressable LED protocols.

2. **Identify primary/secondary board assignments** — only one `sdkconfig.defaults.<target>`
   file exists per SoC target. When two boards share a target (e.g. DevKitC + XIAO on
   `esp32c6`), the board with the WS2812 LED is **primary** and its config goes in
   `sdkconfig.defaults.<target>`. The board with a simple GPIO LED is **secondary** and
   requires a one-time `idf.py menuconfig` step after `set-target`. Document this
   explicitly in the FunctionalSpec under `## Multi-Manufacturer Same-Chip Handling` or
   `## Supported Boards` notes, listing the menuconfig knobs the secondary-board user must set.

3. **Emit the RMT timing constants** — these values are derived from the WS2812B datasheet
   and must not be recomputed or altered. Use verbatim in the generated source file:

   ```c
   #define WS2812_RMT_RESOLUTION_HZ  (10 * 1000 * 1000)  /* 10 MHz → 1 tick = 100 ns */
   #define WS2812_T0H_TICKS  3   /* 300 ns high for a 0-bit */
   #define WS2812_T0L_TICKS  9   /* 900 ns low  for a 0-bit */
   #define WS2812_T1H_TICKS  9   /* 900 ns high for a 1-bit */
   #define WS2812_T1L_TICKS  3   /* 300 ns low  for a 1-bit */
   ```

   Both timings are within the WS2812B ±150 ns tolerance band at 10 MHz resolution.

4. **Emit the RMT static data and `ws2812_write()` function**:

   ```c
   static rmt_channel_handle_t s_rmt_chan;
   static rmt_encoder_handle_t s_bytes_enc;
   static const rmt_transmit_config_t s_tx_cfg = {
       .loop_count      = 0,
       .flags.eot_level = 0,  /* hold line LOW after frame — WS2812 reset condition */
   };

   static void ws2812_write(uint8_t r, uint8_t g, uint8_t b)
   {
       uint8_t grb[3] = {g, r, b};  /* WS2812B expects GRB order, not RGB */
       rmt_transmit(s_rmt_chan, s_bytes_enc, grb, sizeof(grb), &s_tx_cfg);
       rmt_tx_wait_all_done(s_rmt_chan, pdMS_TO_TICKS(100));
   }
   ```

   The GRB byte swap is mandatory. RGB order produces wrong colours with no compile-time
   warning. The blocking `rmt_tx_wait_all_done` call ensures no frame collision occurs
   when `ws2812_write` is called in rapid succession.

5. **Emit the RMT init sequence** — adapt the function name to match the example's
   architecture (`ws2812_init` for a standalone peripheral, `led_init` when the same
   function handles both GPIO and WS2812 paths via `#if CONFIG_EXAMPLE_LED_WS2812`):

   ```c
   /* Call this function name at whatever matches the example architecture */
   static void ws2812_init(void)
   {
       rmt_tx_channel_config_t tx_cfg = {
           .gpio_num          = LED_GPIO,          /* = CONFIG_EXAMPLE_LED_GPIO */
           .clk_src           = RMT_CLK_SRC_DEFAULT,
           .resolution_hz     = WS2812_RMT_RESOLUTION_HZ,
           .mem_block_symbols = 64,
           .trans_queue_depth = 4,
       };
       ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_cfg, &s_rmt_chan));

       rmt_bytes_encoder_config_t enc_cfg = {
           .bit0 = { .level0 = 1, .duration0 = WS2812_T0H_TICKS,
                     .level1 = 0, .duration1 = WS2812_T0L_TICKS },
           .bit1 = { .level0 = 1, .duration0 = WS2812_T1H_TICKS,
                     .level1 = 0, .duration1 = WS2812_T1L_TICKS },
           .flags.msb_first = 1,
       };
       ESP_ERROR_CHECK(rmt_new_bytes_encoder(&enc_cfg, &s_bytes_enc));
       ESP_ERROR_CHECK(rmt_enable(s_rmt_chan));

       ws2812_write(0, 0, 0);  /* ensure LED starts dark regardless of prior state */
       ESP_LOGI(TAG, "WS2812 RMT init on GPIO %d (10 MHz, GRB order)", LED_GPIO);
   }
   ```

   The trailing `ws2812_write(0,0,0)` call and the `ESP_LOGI` are mandatory: the write
   guarantees a clean start state; the log gives the developer console confirmation that
   the RMT driver initialised (distinguishes software init from a missing solder jumper).

6. **Emit Kconfig symbol additions** inside `menu "Example Configuration"` in
   `main/Kconfig.projbuild`:

   ```kconfig
   config EXAMPLE_LED_GPIO
       int "LED GPIO number"
       default <primary-board-gpio>
       range 0 48
       help
           GPIO driving the onboard LED.
           Overridden per-target via sdkconfig.defaults.<target>.
           WS2812 boards: set via sdkconfig.defaults (primary) or menuconfig (secondary).

   config EXAMPLE_LED_WS2812
       bool "Use WS2812 RGB LED (RMT-driven)"
       default n
       help
           When enabled, the LED is driven via the RMT peripheral using the WS2812B
           NRZ protocol (GRB byte order, 800 kHz). EXAMPLE_LED_ACTIVE_LEVEL is ignored.
           Enable for WS2812 primary boards via sdkconfig.defaults.<target>.
           Secondary GPIO-LED boards leave this disabled (default).

   config EXAMPLE_LED_ACTIVE_LEVEL
       int "LED active level (0 = active-LOW, 1 = active-HIGH)"
       default 0
       range 0 1
       help
           Logic level that turns the GPIO LED ON.
           Not used when EXAMPLE_LED_WS2812 is enabled.
   ```

7. **Emit CMakeLists.txt and sdkconfig.defaults fragments**:

   - Add `esp_driver_rmt` to `REQUIRES` in `main/CMakeLists.txt`. Always include it
     unconditionally — avoids conditional CMake complexity and is harmless when the
     `#if CONFIG_EXAMPLE_LED_WS2812` guard disables the RMT code paths.

   - For each WS2812 primary board, write or update `sdkconfig.defaults.<target>`:

     ```ini
     # AGENT-GENERATED — do not edit by hand
     # Target: <chip> (<Board Name> — PRIMARY)
     # Board-spec: board-specs/<vendor>/<board>.md
     #
     # WS2812 RGB LED on GPIO <n>. <console note>.
     # <Secondary board> users: run `idf.py menuconfig` and set
     #   EXAMPLE_LED_WS2812=n, EXAMPLE_LED_GPIO=<secondary-gpio>
     #   <and any console switch if needed>.

     CONFIG_EXAMPLE_LED_GPIO=<gpio>
     CONFIG_EXAMPLE_LED_WS2812=y
     # CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y   ← add only if board needs it
     ```

8. **Apply mandatory brightness and visibility rules** (sourced from
   `shared-specs/AIGenLessonsLearned.md` — non-negotiable):

   - **Minimum brightness: 64/255 per active channel.** Values of 8/255 (~3%) are
     invisible under normal room lighting and indistinguishable from a hardware fault
     (e.g. unsoldered LED jumper). Always validate that CodingSpec-specified colours
     meet this floor before generating the source.
   - **Minimum blink count: 3 repetitions** for any pass/fail/status indicator. A
     single 500 ms flash is impossible to observe reliably in a real demo environment.
   - **Scope**: these rules govern the *application-layer* colours and timing that the
     caller specifies in CodingSpec.md. This skill does not hardcode colours — it
     enforces the rules in review and flags any CodingSpec values that fall below the
     minimums before generating code.
   - Log the RMT resolution and GRB byte-order note at INFO level during init (step 5
     above) so the console independently confirms the driver started correctly.
