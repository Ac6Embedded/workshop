/** @file
 * @brief Led
 *
 * Provide basic led functions.
 */

/*
 * Copyright (c) 2025 Ac6
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "led.h"

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

int set_led_color(char *color)
{
    if(strcmp(color, "green") == 0) {
        gpio_pin_set_dt(&led_red, LED_OFF);
        gpio_pin_set_dt(&led_blue, LED_OFF);
        gpio_pin_set_dt(&led_green, LED_ON);
    } else if(strcmp(color, "red") == 0) {
        gpio_pin_set_dt(&led_red, LED_ON);
        gpio_pin_set_dt(&led_green, LED_OFF);
        gpio_pin_set_dt(&led_blue, LED_OFF);
    } else if(strcmp(color, "blue") == 0) {
        gpio_pin_set_dt(&led_red, LED_OFF);
        gpio_pin_set_dt(&led_green, LED_OFF);
        gpio_pin_set_dt(&led_blue, LED_ON);
    } else if(strcmp(color, "white") == 0) {
        gpio_pin_set_dt(&led_red, LED_ON);
        gpio_pin_set_dt(&led_green, LED_ON);
        gpio_pin_set_dt(&led_blue, LED_ON);
    } else if(strcmp(color, "purple") == 0) {
        gpio_pin_set_dt(&led_red, LED_ON);
        gpio_pin_set_dt(&led_green, LED_OFF);
        gpio_pin_set_dt(&led_blue, LED_ON);
    } else if(strcmp(color, "yellow") == 0) {
        gpio_pin_set_dt(&led_red, LED_ON);
        gpio_pin_set_dt(&led_green, LED_ON);
        gpio_pin_set_dt(&led_blue, LED_OFF);
    } else if(strcmp(color, "cyan") == 0) {
        gpio_pin_set_dt(&led_red, LED_OFF);
        gpio_pin_set_dt(&led_green, LED_ON);
        gpio_pin_set_dt(&led_blue, LED_ON);
    } else {
        return 1;
    }
    return 0;
}

static int led_init(void)
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

SYS_INIT(led_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);