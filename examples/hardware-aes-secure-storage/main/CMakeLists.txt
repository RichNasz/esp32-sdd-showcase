# AGENT-GENERATED — do not edit by hand; regenerate via esp32-sdd-full-project-generator
idf_component_register(
    SRCS
        "main.c"
        "secure_storage.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        driver
        esp_driver_rmt
        esp_hw_support
        esp_timer
        mbedtls
        nvs_flash
)
