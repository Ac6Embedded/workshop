/** @file
 * @brief Led shell module
 *
 * Provide basic led shell commands.
 */

/*
 * Copyright (c) 2025 Ac6
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <zephyr/logging/log.h>
 LOG_MODULE_REGISTER(led_shell, LOG_LEVEL_DBG);
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/shell/shell.h>

/* The devicetree node identifier for the "led0" alias. */
#define LED_GREEN_NODE DT_ALIAS(led0)
/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET(LED_GREEN_NODE, gpios);

static int cmd_led_toggle(const struct shell *sh, size_t argc, char *argv[])
{
    return gpio_pin_toggle_dt(led_active);
}

SHELL_STATIC_SUBCMD_SET_CREATE(led_cmds,
    SHELL_CMD(toggle, NULL, "Toggle LED", cmd_led_toggle),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(led, &led_cmds, "LED control commands", NULL);

static int led_shell_init(void)
{
	int ret;
    if (!gpio_is_ready_dt(&led_green)) {
        return 0;
    }
    
    ret = gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        return 0;
    }

	return 0;
}

SYS_INIT(led_shell_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);