# Power Budget Template

**Classification:** Human-authored shared spec
**Applies to:** All low-power examples (deep sleep, light sleep, ULP co-processor)

---

## How to Use This Template

Copy the tables below into the example's `FunctionalSpec.md` under a `## Power Budget` section. Fill in the values for the specific hardware. Leave rows that don't apply with `—`.

The generator reads these values to:

- Select the appropriate `sdkconfig.defaults` power options.
- Configure sleep wakeup sources and timing.
- Validate that the estimated current draw meets the target.

---

## Power Budget Table

```markdown
## Power Budget

| Mode | Typical Current | Duration | Energy per Cycle |
|---|---|---|---|
| Active (CPU @ 240 MHz) | ___ mA | ___ ms | ___ µAh |
| Active (CPU @ 80 MHz) | ___ mA | ___ ms | ___ µAh |
| Modem-sleep (Wi-Fi associated) | ___ mA | ___ ms | ___ µAh |
| Light sleep | ___ µA | ___ s | ___ µAh |
| Deep sleep (RTC only) | ___ µA | ___ s | ___ µAh |
| Deep sleep + ULP | ___ µA | ___ s | ___ µAh |
| **Total per duty cycle** | — | ___ s | ___ µAh |
| **Estimated daily energy** | — | 24 h | ___ mAh |

**Battery target:** ___ mAh at ___ V → estimated life: ___ days (at ___ duty cycle)
**Sleep wakeup source:** Timer / GPIO / ULP / Touch (choose one or more)
**Wakeup interval:** ___ seconds
```

---

## Reference Current Draw (ESP32 Datasheet, v5.4 ERA)

These are datasheet-typical values for reference when filling in the template.

### ESP32 (Xtensa LX6)

| Mode | Typical |
|---|---|
| Active, CPU @ 240 MHz, Wi-Fi Tx | 240 mA peak |
| Active, CPU @ 240 MHz, no RF | 68 mA |
| Active, CPU @ 80 MHz, no RF | 31 mA |
| Modem sleep (associated, 100 ms beacon interval) | 20 mA avg |
| Light sleep | 0.8 mA |
| Deep sleep, RTC timer + RTC memory | 10 µA |
| Deep sleep, ULP running | 150 µA |

### ESP32-S3 (Xtensa LX7)

| Mode | Typical |
|---|---|
| Active, CPU @ 240 MHz, Wi-Fi Tx | 310 mA peak |
| Active, CPU @ 240 MHz, no RF | 95 mA |
| Active, CPU @ 80 MHz, no RF | 43 mA |
| Light sleep | 1.3 mA |
| Deep sleep, RTC | 7 µA |
| Deep sleep, ULP-RISC-V | 100 µA |

### ESP32-C3 (RISC-V)

| Mode | Typical |
|---|---|
| Active, 160 MHz, Wi-Fi Tx | 270 mA peak |
| Active, 160 MHz, no RF | 33 mA |
| Light sleep | 0.13 mA |
| Deep sleep, RTC | 5 µA |

---

## sdkconfig.defaults Snippets for Common Power Modes

The full-project-generator will emit these based on the FunctionalSpec power budget. Included here for reference.

### Deep Sleep with Timer Wakeup

```
CONFIG_PM_ENABLE=n
CONFIG_ESP_SLEEP_POWER_DOWN_FLASH=y
CONFIG_ESP_SLEEP_GPIO_RESET_WORKAROUND=y
CONFIG_RTC_CLK_SRC_INT_RC=y         # Use internal 150 kHz RC (saves ~5 µA vs XTAL)
```

### Light Sleep with FreeRTOS Tickless Idle

```
CONFIG_PM_ENABLE=y
CONFIG_FREERTOS_USE_TICKLESS_IDLE=y
CONFIG_FREERTOS_IDLE_TIME_BEFORE_SLEEP=3  # ticks before entering light sleep
CONFIG_ESP_SLEEP_POWER_DOWN_FLASH=n       # keep flash for light sleep resume
```

### ULP Co-processor (ESP32 FSM ULP)

```
CONFIG_ULP_COPROC_ENABLED=y
CONFIG_ULP_COPROC_TYPE_FSM=y
CONFIG_ULP_COPROC_RESERVE_MEM=4096
```

### ULP Co-processor (ESP32-S3 RISC-V ULP)

```
CONFIG_ULP_COPROC_ENABLED=y
CONFIG_ULP_COPROC_TYPE_RISCV=y
CONFIG_ULP_COPROC_RESERVE_MEM=4096
```

---

## Battery Life Estimation Formula

```
daily_mAh = (active_mA × active_s + sleep_µA/1000 × sleep_s) × cycles_per_day / 3600

days = battery_mAh × 0.8 / daily_mAh   # 80% usable capacity rule of thumb
```

Document both the formula and the parameter values in the `FunctionalSpec.md` so regeneration is fully reproducible.

---

## Design Constraints

- Active phase must complete all sensor reads, processing, and transmission before entering sleep. Never extend active time by waiting for peripherals that could be pre-configured.
- Minimize wake count. Consolidate multiple sensor reads into a single wake cycle rather than waking separately for each sensor.
- RTC GPIO must be used for wakeup from deep sleep — regular GPIO cannot wake from deep sleep.
- Flash is powered down during deep sleep (`CONFIG_ESP_SLEEP_POWER_DOWN_FLASH=y`) unless ULP needs to write to flash (rare and discouraged).
- All peripherals (SPI, I2C, UART) must be de-initialized before calling `esp_deep_sleep_start()`.
