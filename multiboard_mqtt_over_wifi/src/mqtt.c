/*
 * Copyright (c) 2025 Ac6.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(mqtt_over_wifi);

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/random/random.h>
#include <zephyr/shell/shell.h>

#include <string.h>
#include <errno.h>

#include "mqtt.h"
#include "led.h"

#if defined(USERSPACE)
#include <zephyr/app_memory/app_memdomain.h>
K_APPMEM_PARTITION_DEFINE(app_partition);
struct k_mem_domain app_domain;
#define APP_BMEM K_APP_BMEM(app_partition)
#define APP_DMEM K_APP_DMEM(app_partition)
#else
#define APP_BMEM
#define APP_DMEM
#endif

/* Buffers for MQTT client. */
static uint8_t rx_buffer[MQTT_MESSAGE_BUFFER_SIZE];
static uint8_t tx_buffer[MQTT_MESSAGE_BUFFER_SIZE];
static uint8_t payload_buf[MQTT_PAYLOAD_BUFFER_SIZE];

/* The mqtt client struct */
static APP_BMEM struct mqtt_client client;
/* File descriptor */
static APP_BMEM struct pollfd fds[1];
/* MQTT Broker details. */
static struct sockaddr_storage broker;

/**@brief Function to get the payload of recived data.
 */
static int get_received_payload(struct mqtt_client *c, size_t length)
{
	int ret;
	int err = 0;

	/* Return an error if the payload is larger than the payload buffer.
	 * Note: To allow new messages, we have to read the payload before returning.
	 */
	if (length > sizeof(payload_buf)) {
		err = -EMSGSIZE;
	}

	/* Truncate payload until it fits in the payload buffer. */
	while (length > sizeof(payload_buf)) {
		ret = mqtt_read_publish_payload_blocking(
				c, payload_buf, (length - sizeof(payload_buf)));
		if (ret == 0) {
			return -EIO;
		} else if (ret < 0) {
			return ret;
		}

		length -= ret;
	}

	ret = mqtt_readall_publish_payload(c, payload_buf, length);
	if (ret) {
		return ret;
	}

	return err;
}

/**@brief Function to subscribe to the configured topic
 */

static int subscribe(struct mqtt_client *const c)
{
	struct mqtt_topic subscribe_topics[] = {
		{
			.topic = {
				.utf8 = MQTT_BROADCAST_TOPIC,
				.size = strlen(MQTT_BROADCAST_TOPIC)
			},
			.qos = MQTT_QOS_1_AT_LEAST_ONCE
		},
		{
			.topic = {
				.utf8 = MQTT_SUB_MY_TOPIC,
				.size = strlen(MQTT_SUB_MY_TOPIC)
			},
			.qos = MQTT_QOS_1_AT_LEAST_ONCE
		}
	};

	const struct mqtt_subscription_list subscription_list = {
		.list = subscribe_topics,
		.list_count = 2,
		.message_id = 1234
	};

	return mqtt_subscribe(c, &subscription_list);
}

/**@brief Function to print strings without null-termination
 */
static void data_print(uint8_t *prefix, uint8_t *data, size_t len)
{
	char buf[len + 1];

	memcpy(buf, data, len);
	buf[len] = 0;
	LOG_INF("%s%s", (char *)prefix, (char *)buf);
}

static void handle_payload(uint8_t *data, size_t len)
{
	int ret;
	char buf[len + 1];
	memcpy(buf, data, len);
	buf[len] = 0;

	ret = set_led_color(buf);
	if (ret) {
		LOG_ERR("Cannot change LED color");
	} else {
		LOG_INF("LED color changed into %s", buf);
	}
}

int data_publish(enum mqtt_qos qos, char *topic,
	uint8_t *data, size_t len)
{
	struct mqtt_publish_param param;

	param.message.topic.qos = qos;
	param.message.topic.topic.utf8 = topic;
	param.message.topic.topic.size = strlen(topic);
	param.message.payload.data = data;
	param.message.payload.len = len;
	param.message_id = sys_rand32_get();
	param.dup_flag = 0;
	param.retain_flag = 0;

	data_print("Publishing: ", data, len);
	LOG_INF("to topic: %s len: %u",
		topic,
		(unsigned int)strlen(topic));

	return mqtt_publish(&client, &param);
}

static const uint8_t* client_id_get(void)
{
	static uint8_t client_id[MAX(sizeof(MQTT_CLIENT_ID),
				     CLIENT_ID_LEN)];

	if (strlen(MQTT_CLIENT_ID) > 0) {
		snprintf(client_id, sizeof(client_id), "%s",
			 MQTT_CLIENT_ID);
		goto exit;
	}

	uint32_t id = sys_rand32_get();
	snprintf(client_id, sizeof(client_id), "%s-%010u", CONFIG_BOARD, id);

exit:
	LOG_DBG("client_id = %s", (char *)client_id);

	return client_id;
}

void mqtt_evt_handler(struct mqtt_client *const c,
		      const struct mqtt_evt *evt)
{
	int err;

	switch (evt->type) {
	case MQTT_EVT_CONNACK:
		if (evt->result != 0) {
			LOG_ERR("MQTT connect failed: %d", evt->result);
			break;
		}

		LOG_INF("MQTT client connected");
		subscribe(c);
		break;

	case MQTT_EVT_DISCONNECT:
		LOG_INF("MQTT client disconnected: %d", evt->result);
		break;

	case MQTT_EVT_PUBLISH:
	{
		const struct mqtt_publish_param *p = &evt->param.publish;
		/* Print the length of the recived message */
		LOG_INF("MQTT PUBLISH result=%d len=%d",
			evt->result, p->message.payload.len);

		/* Extract the data of the received message */
		err = get_received_payload(c, p->message.payload.len);
		
		/* Send acknowledgment to the broker on receiving QoS1 publish message */
		if (p->message.topic.qos == MQTT_QOS_1_AT_LEAST_ONCE) {
			const struct mqtt_puback_param ack = {
				.message_id = p->message_id
			};

			mqtt_publish_qos1_ack(c, &ack);
		}

		if (err >= 0) {
			if(strcmp(p->message.topic.topic.utf8, MQTT_BROADCAST_TOPIC) == 0) {
				data_print("Received from broadcast: ", payload_buf, p->message.payload.len);
			} else if(strcmp(p->message.topic.topic.utf8, MQTT_SUB_MY_TOPIC) == 0) {
				data_print("Received: ", payload_buf, p->message.payload.len);
			}
		// Payload buffer is smaller than the received data 
		} else if (err == -EMSGSIZE) {
			LOG_ERR("Received payload (%d bytes) is larger than the payload buffer size (%d bytes).",
				p->message.payload.len, sizeof(payload_buf));
		// Failed to extract data, disconnect 
		} else {
			LOG_ERR("get_received_payload failed: %d", err);
			LOG_INF("Disconnecting MQTT client...");

			err = mqtt_disconnect(c);
			if (err) {
				LOG_ERR("Could not disconnect: %d", err);
			}
		}
	} break;

	case MQTT_EVT_PUBACK:
		if (evt->result != 0) {
			LOG_ERR("MQTT PUBACK error: %d", evt->result);
			break;
		}

		LOG_INF("PUBACK packet id: %u", evt->param.puback.message_id);
		break;

	case MQTT_EVT_SUBACK:
		if (evt->result != 0) {
			LOG_ERR("MQTT SUBACK error: %d", evt->result);
			break;
		}

		LOG_INF("SUBACK packet id: %u", evt->param.suback.message_id);
		break;

	case MQTT_EVT_PINGRESP:
		if (evt->result != 0) {
			LOG_ERR("MQTT PINGRESP error: %d", evt->result);
		}
		break;

	default:
		LOG_INF("Unhandled MQTT event type: %d", evt->type);
		break;
	}
}

static void broker_init(void)
{
#if defined(NET_IPV6)
	struct sockaddr_in6 *broker6 = (struct sockaddr_in6 *)&broker;

	broker6->sin6_family = AF_INET6;
	broker6->sin6_port = htons(MQTT_BROKER_PORT);
	inet_pton(AF_INET6, SERVER_ADDR, &broker6->sin6_addr);

#if defined(SOCKS)
	struct sockaddr_in6 *proxy6 = (struct sockaddr_in6 *)&socks5_proxy;

	proxy6->sin6_family = AF_INET6;
	proxy6->sin6_port = htons(SOCKS5_PROXY_PORT);
	inet_pton(AF_INET6, SOCKS5_PROXY_ADDR, &proxy6->sin6_addr);
#endif
#else
	struct sockaddr_in *broker4 = (struct sockaddr_in *)&broker;

	broker4->sin_family = AF_INET;
	broker4->sin_port = htons(MQTT_BROKER_PORT);
	inet_pton(AF_INET, SERVER_ADDR, &broker4->sin_addr);
#if defined(SOCKS)
	struct sockaddr_in *proxy4 = (struct sockaddr_in *)&socks5_proxy;

	proxy4->sin_family = AF_INET;
	proxy4->sin_port = htons(SOCKS5_PROXY_PORT);
	inet_pton(AF_INET, SOCKS5_PROXY_ADDR, &proxy4->sin_addr);
#endif
#endif
}

static void client_init(struct mqtt_client *client)
{
	mqtt_client_init(client);

	broker_init();

	/* MQTT client configuration */
	client->broker = &broker;
	client->evt_cb = mqtt_evt_handler;
	client->client_id.utf8 = client_id_get();
	client->client_id.size = strlen(client->client_id.utf8);
	client->password = NULL;
	client->user_name = NULL;
	client->protocol_version = MQTT_VERSION_3_1_1;

	/* MQTT buffers configuration */
	client->rx_buf = rx_buffer;
	client->rx_buf_size = sizeof(rx_buffer);
	client->tx_buf = tx_buffer;
	client->tx_buf_size = sizeof(tx_buffer);

	/* MQTT transport configuration */
	client->transport.type = MQTT_TRANSPORT_NON_SECURE;

#if defined(SOCKS)
	mqtt_client_set_proxy(client, &socks5_proxy,
			      socks5_proxy.sa_family == AF_INET ?
			      sizeof(struct sockaddr_in) :
			      sizeof(struct sockaddr_in6));
#endif
}

static int fds_init(struct mqtt_client *client)
{
	if (client->transport.type == MQTT_TRANSPORT_NON_SECURE) {
		fds[0].fd = client->transport.tcp.sock;
	} else {
		return -ENOTSUP;
	}

	fds[0].events = POLLIN;
	return 0;
}

int start_mqtt_app(void)
{
	int err;
	uint32_t connect_attempt = 0;

	client_init(&client);

do_connect:
	if (connect_attempt++ > 0) {
		LOG_INF("Reconnecting in %d seconds...", MQTT_RECONNECT_DELAY);
		k_sleep(K_SECONDS(MQTT_RECONNECT_DELAY));
	}
	err = mqtt_connect(&client);
	if (err) {
		LOG_ERR("Error in mqtt_connect: %d", err);
		goto do_connect;
	}

	err = fds_init(&client);
	if (err) {
		LOG_ERR("Error in fds_init: %d", err);
		return -2;
	}

	while (1) {
		err = poll(fds, 1, mqtt_keepalive_time_left(&client));
		if (err < 0) {
			LOG_ERR("Error in poll(): %d", errno);
			break;
		}

		err = mqtt_live(&client);
		if ((err != 0) && (err != -EAGAIN)) {
			LOG_ERR("Error in mqtt_live: %d", err);
			break;
		}

		if ((fds[0].revents & POLLIN) == POLLIN) {
			err = mqtt_input(&client);
			if (err != 0) {
				LOG_ERR("Error in mqtt_input: %d", err);
				break;
			}
		}

		if ((fds[0].revents & POLLERR) == POLLERR) {
			LOG_ERR("POLLERR");
			break;
		}

		if ((fds[0].revents & POLLNVAL) == POLLNVAL) {
			LOG_ERR("POLLNVAL");
			break;
		}
	}

	LOG_INF("Disconnecting MQTT client");

	err = mqtt_disconnect(&client);
	if (err) {
		LOG_ERR("Could not disconnect MQTT client: %d", err);
	}
	goto do_connect;
}

static int get_mqtt_topic(char* topic, const char *dest) 
{
	int ret = 0;
	if(strcmp(dest, "all") == 0) {
		ret = snprintf(topic, 21, "%s", MQTT_BROADCAST_TOPIC);
	} else {
		ret = snprintf(topic, 21, "%s%s", MQTT_PUB_TOPIC_PREFIX, dest);
	}
	return ret;
}

static int cmd_mqtt_echo(const struct shell *sh, size_t argc, char *argv[])
{
	if(argc <= 2) {
        shell_print(sh, "Missing argument, eg. mqtt echo <n|all> <message>\n\r");
        return 1;
    } else {
		char topic[21];
		int ret = get_mqtt_topic(topic, argv[1]);
		if(ret < 0) {
			shell_print(sh, "Missing echo destination, eg. mqtt echo <n|all> <message>\n\r");
       		return 2;
		}
		data_publish(MQTT_QOS_1_AT_LEAST_ONCE, topic, argv[2], strlen(argv[2]));
	}
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(mqtt_cmds,
    SHELL_CMD(echo, NULL, "Echo message", cmd_mqtt_echo),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(mqtt, &mqtt_cmds, "MQTT commands", NULL);