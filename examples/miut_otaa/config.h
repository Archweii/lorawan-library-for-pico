/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 */

#define LORAWAN_DEFAULT_CLASS   CLASS_C

// LoRaWAN region to use, full list of regions can be found at: 
//   http://stackforce.github.io/LoRaMac-doc/LoRaMac-doc-v4.5.1/group___l_o_r_a_m_a_c.html#ga3b9d54f0355b51e85df8b33fd1757eec
#define LORAWAN_REGION          LORAMAC_REGION_AU915

// LoRaWAN Device EUI (64-bit), NULL value will use Default Dev EUI
#define LORAWAN_DEVICE_EUI      "5919da448c0bb6ab"

// LoRaWAN Application / Join EUI (64-bit)
#define LORAWAN_APP_EUI         "7dddb8bff45b9151"

// LoRaWAN Application Key (128-bit)
#define LORAWAN_APP_KEY         "f89f29895fb953a01bf79ff6efb630d4"

// LoRaWAN Channel Mask, NULL value will use the default channel mask 
// for the region
#define LORAWAN_CHANNEL_MASK    NULL

#define LORAWAN_PUBLIC_NETWORK  false


// Custom own program
#define LED_RED         12
#define LED_GREEN       13
#define LED_BLUE        14
#define LED_WHITE       15


#define COLOR_BLACK     0,      0,      0,      0
#define COLOR_WHITE     0,      0,      0,      255
#define COLOR_RED       255,    0,      0,      0
#define COLOR_GREEN     0,      255,    0,      0
#define COLOR_BLUE      0,      0,      255,    0

