/*
 * Copyright (c) 2025 Ac6.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __WIFI_H
#define __WIFI_H

#define WIFI_CONNECT_SSID 			    "FRDM-IMX93"
#define WIFI_CONNECT_SSID_PSK		    "ac6-formation"
#define WIFI_CONNECT_SSID_SECURITY	    WIFI_SECURITY_TYPE_PSK

int wifi_auto_connect(void);

#endif