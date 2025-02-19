/*
 * Copyright (c) 2025 Ac6.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __MQTT_H
#define __MQTT_H

#include <zephyr/net/mqtt.h>

#define MQTT_MESSAGE_BUFFER_SIZE 	128
#define MQTT_PAYLOAD_BUFFER_SIZE 	128

#define MQTT_SUB_MY_TOPIC           "frdm-rw612-1"
#define MQTT_PUB_TOPIC_PREFIX		"frdm-rw612-"
#define MQTT_BROADCAST_TOPIC 		"frdm-rw612-broadcast"
#define MQTT_BROKER_HOSTNAME 		"localhost"

#define MQTT_CLIENT_ID              "frdm-rw612"
#define MQTT_BROKER_PORT            1883
#define MQTT_RECONNECT_DELAY 		30

#define SERVER_ADDR                 "192.168.1.1"
#define RANDOM_LEN                  10
#define CLIENT_ID_LEN               sizeof(CONFIG_BOARD) + 1 + RANDOM_LEN

int start_mqtt_app(void);
int data_publish(enum mqtt_qos qos, char *topic, uint8_t *data, size_t len);

#endif