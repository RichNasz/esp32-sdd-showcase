# AGENT-GENERATED — do not edit by hand
# Generated from specs/ using esp32-sdd-full-project-generator skill
# Date: 2026-04-12 | Agent: Claude Code

idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        driver          # gpio_config, gpio_set_level, gpio_hold_dis
        esp_driver_rmt  # rmt_new_tx_channel, rmt_new_bytes_encoder (WS2812 path)
        esp_hw_support  # esp_random, esp_sleep_enable_timer_wakeup, esp_deep_sleep_start
)
