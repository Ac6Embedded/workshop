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
#define LED_RED_NODE DT_ALIAS(led1)
#define LED_BLUE_NODE DT_ALIAS(led2)

#define LED_ON 0
#define LED_OFF 1

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET(LED_GREEN_NODE, gpios);
static const struct gpio_dt_spec led_red = GPIO_DT_SPEC_GET(LED_RED_NODE, gpios);
static const struct gpio_dt_spec led_blue = GPIO_DT_SPEC_GET(LED_BLUE_NODE, gpios);
int lastRedValue = LED_ON,  lastGreenValue= LED_ON, lastBlueValue = LED_ON;


static int cmd_led_toggle(const struct shell *sh, size_t argc, char *argv[])
{
    int redValue = gpio_pin_get_dt(&led_red);
    int greenValue = gpio_pin_get_dt(&led_green);
    int blueValue = gpio_pin_get_dt(&led_blue);

    if(redValue == LED_ON || greenValue == LED_ON || blueValue == LED_ON) {
        lastRedValue = redValue;
        lastGreenValue= greenValue;
        lastBlueValue = blueValue;
        gpio_pin_set_dt(&led_red, LED_OFF);
        gpio_pin_set_dt(&led_green, LED_OFF);
        gpio_pin_set_dt(&led_blue, LED_OFF);
    } else {
        gpio_pin_set_dt(&led_red, lastRedValue);
        gpio_pin_set_dt(&led_green, lastGreenValue);
        gpio_pin_set_dt(&led_blue, lastBlueValue);
    }
    return 0;
}

static int cmd_led_color(const struct shell *sh, size_t argc, char *argv[])
{
    if(argc <= 1) {
        shell_print(sh, "Missing color argument, eg. led color green\n\r");
        return 1;
    } else {
        if(strcmp(argv[1], "green") == 0) {
            gpio_pin_set_dt(&led_red, LED_OFF);
            gpio_pin_set_dt(&led_blue, LED_OFF);
            gpio_pin_set_dt(&led_green, LED_ON);
        } else if(strcmp(argv[1], "red") == 0) {
            gpio_pin_set_dt(&led_red, LED_ON);
            gpio_pin_set_dt(&led_green, LED_OFF);
            gpio_pin_set_dt(&led_blue, LED_OFF);
        } else if(strcmp(argv[1], "blue") == 0) {
            gpio_pin_set_dt(&led_red, LED_OFF);
            gpio_pin_set_dt(&led_green, LED_OFF);
            gpio_pin_set_dt(&led_blue, LED_ON);
        } else if(strcmp(argv[1], "white") == 0) {
            gpio_pin_set_dt(&led_red, LED_ON);
            gpio_pin_set_dt(&led_green, LED_ON);
            gpio_pin_set_dt(&led_blue, LED_ON);
        } else if(strcmp(argv[1], "purple") == 0) {
            gpio_pin_set_dt(&led_red, LED_ON);
            gpio_pin_set_dt(&led_green, LED_OFF);
            gpio_pin_set_dt(&led_blue, LED_ON);
        } else if(strcmp(argv[1], "yellow") == 0) {
            gpio_pin_set_dt(&led_red, LED_ON);
            gpio_pin_set_dt(&led_green, LED_ON);
            gpio_pin_set_dt(&led_blue, LED_OFF);
        } else if(strcmp(argv[1], "cyan") == 0) {
            gpio_pin_set_dt(&led_red, LED_OFF);
            gpio_pin_set_dt(&led_green, LED_ON);
            gpio_pin_set_dt(&led_blue, LED_ON);
        } else {
            shell_print(sh,"Unknown color argument, eg. led color <green|red|blue|yellow|purple|cyan|white>\n\r");
            return 2;
        }
    }
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(led_cmds,
    SHELL_CMD(toggle, NULL, "Toggle LED", cmd_led_toggle),
    SHELL_CMD(color, NULL, "Set color <green|red|blue|yellow|purple|cyan|white>", cmd_led_color),
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

    if (!gpio_is_ready_dt(&led_red)) {
        return 0;
    }
    
    ret = gpio_pin_configure_dt(&led_red, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        return 0;
    }

    if (!gpio_is_ready_dt(&led_blue)) {
        return 0;
    }
    
    ret = gpio_pin_configure_dt(&led_blue, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        return 0;
    }

	return 0;
}

SYS_INIT(led_shell_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);