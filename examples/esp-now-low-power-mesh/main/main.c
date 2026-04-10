/* ================================================
   AGENT-GENERATED — DO NOT EDIT BY HAND
   Generated from specs/ using esp32-sdd-full-project-generator skill
   Date: 2026-04-08 | Agent: Claude Code
   ================================================ */

#include "sdkconfig.h"
#include "sensor.h"
#include "relay.h"
#include "gateway.h"
#include "sensor_relay.h"

/**
 * @brief Application entry point — dispatches to the role selected at build time.
 *
 * NODE_ROLE is a Kconfig choice; exactly one of CONFIG_NODE_ROLE_SENSOR,
 * CONFIG_NODE_ROLE_RELAY, or CONFIG_NODE_ROLE_GATEWAY is set to y. Each role
 * function handles its own initialisation and never returns.
 */
void app_main(void)
{
#if CONFIG_NODE_ROLE_SENSOR
    sensor_run();
#elif CONFIG_NODE_ROLE_RELAY
    relay_run();
#elif CONFIG_NODE_ROLE_GATEWAY
    gateway_run();
#elif CONFIG_NODE_ROLE_SENSOR_RELAY
    sensor_relay_run();
#else
#error "No NODE_ROLE selected. Set CONFIG_NODE_ROLE_SENSOR, _RELAY, _GATEWAY, or _SENSOR_RELAY."
#endif
}
