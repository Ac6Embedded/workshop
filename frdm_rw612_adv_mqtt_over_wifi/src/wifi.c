/*
 * Copyright (c) 2025 Ac6.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(mqtt_over_wifi);

#include "wifi.h"
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/wifi_utils.h>

#define WIFI_MGMT_EVENTS ( 				\
    NET_EVENT_WIFI_SCAN_RESULT |		\
    NET_EVENT_WIFI_SCAN_DONE |			\
    NET_EVENT_WIFI_CONNECT_RESULT |		\
    NET_EVENT_WIFI_DISCONNECT_RESULT)

static struct net_mgmt_event_callback cb;
static struct k_sem net_cb_sem;

static void wifi_check_connect_result( struct net_if *iface, struct net_mgmt_event_callback *cb) {
	const struct wifi_status *status = (const struct wifi_status *) cb->info;
	if (!status->status) {
		/* Connected, release semaphore */
		k_sem_give(&net_cb_sem);
	}
}

void wifi_event_listener( struct net_mgmt_event_callback *cb, uint32_t mgmt_event, struct net_if *iface ) {
	switch( mgmt_event ) {
		case NET_EVENT_WIFI_CONNECT_RESULT:
			wifi_check_connect_result( iface, cb );
			break;
		default:
			LOG_WRN("NET_EVENT_WIFI Unknown");
			break;
	}
}

int wifi_auto_connect(void) {
	struct wifi_connect_req_params wifi_args;
	wifi_args.security = WIFI_CONNECT_SSID_SECURITY;
	wifi_args.channel = WIFI_CHANNEL_ANY;
	wifi_args.psk = WIFI_CONNECT_SSID_PSK;
	wifi_args.psk_length = strlen(WIFI_CONNECT_SSID_PSK);
	wifi_args.ssid = WIFI_CONNECT_SSID;
	wifi_args.ssid_length = strlen(WIFI_CONNECT_SSID);

	/* Init semaphore */
	k_sem_init(&net_cb_sem, 0, 1);

	/* Configure Callback */
	net_mgmt_init_event_callback(&cb, wifi_event_listener, WIFI_MGMT_EVENTS );
	net_mgmt_add_event_callback(&cb);

	struct net_if *iface = net_if_get_wifi_sta();
	if( net_mgmt( NET_REQUEST_WIFI_CONNECT, iface, &wifi_args, sizeof(wifi_args) ) ) {
		LOG_ERR("Failed to request connection to SSID %s", WIFI_CONNECT_SSID);
		return -ENOEXEC;
	}

	/* Take semaphore to block till connected */
	k_sem_take(&net_cb_sem, K_FOREVER);
	/* Wait for the interface to be up */
	k_sleep(K_SECONDS(6));

	struct net_if_ipv4 *ipv4 = iface->config.ip.ipv4;
	static char buf[NET_IPV4_ADDR_LEN];
	net_addr_ntop( AF_INET, (const char *)&ipv4->unicast[0].ipv4.address.in_addr, buf, NET_IPV4_ADDR_LEN);
	LOG_INF("Connection successful to SSID: %s", WIFI_CONNECT_SSID);
	LOG_INF("Assigned IP address [%s]", buf);

	return 0;
}
