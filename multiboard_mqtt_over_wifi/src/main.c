/*
 * Copyright (c) 2025 Ac6.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mqtt_over_wifi, LOG_LEVEL_DBG);

#include <stdio.h>
#include "wifi.h"
#include "mqtt.h"

int main(void)
{
	int status = 0;
	LOG_INF("Advanced MQTT over Wi-Fi demonstration on [%s]\n", CONFIG_BOARD_TARGET);

	status = wifi_auto_connect();
	if(status < 0) {
		LOG_ERR("Cannot connect to Wi-Fi\n");
		return -1;
	}

	status = start_mqtt_app();
	if(status < 0) {
		LOG_ERR("Cannot start mqtt application\n");
		return -1;
	}

	return 0;
}
