# ================================================
# AGENT-GENERATED — DO NOT EDIT BY HAND
# Generated from specs/ using esp32-sdd-full-project-generator skill
# Date: 2026-04-12 | Agent: Claude Code
# ================================================

idf_component_register(
    SRCS
        "main.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        nvs_flash
        esp_wifi
        esp_event
        esp_netif
        esp_https_ota
        app_update
        driver
        esp_rom
)

# Embed server CA certificate at build time.
# Operator must replace main/server_cert.pem with their actual CA cert before use.
# Accessible in C via: extern const char server_cert_pem_start[];
target_add_binary_data(${COMPONENT_LIB} "server_cert.pem" TEXT)
