/*
 * wc_mgr.h
 *
 *  Created on: Feb 16, 2017
 *      Author: sharikov
 */
#ifndef WC_MGR_H_
#define WC_MGR_H_

#define FREERTOS
#define HTTPD_MAX_CONNECTIONS 15

// #define WIFI_RESET_SETTINGS_PIN PB_1 // wifi settings reset: connect PB1 to ground

#define GPIO_LED_PIN       PA_4
#define BITBAND_LED 	BITBAND_A4

#define AP_SCAN_LIST_SIZE 32

#define FLASH_APP_BASE  0xd0000
#define flash_size

#define USE_NETBIOS

#define HTTPD_PRIORITY		(tskIDLE_PRIORITY + 1 + PRIORITIE_OFFSET)
#define CAPDNS_PRIORITY		(tskIDLE_PRIORITY + 0 + PRIORITIE_OFFSET)
#define WEBSOC_PRIORITY		(tskIDLE_PRIORITY + 1 + PRIORITIE_OFFSET)

#endif /* WC_MGR_H_ */
