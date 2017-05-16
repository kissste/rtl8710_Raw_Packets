/*
 * user_init.c
 *
 */
#include "user_config.h"
#include "platform_opts.h"
#include "rtl8195a.h"
#include "FreeRTOS.h"
#include "task.h"
#include "diag.h"
#include "objects.h"
#include "wifi_api.h"
//#include "espfs.h"
//#include "http_server.h"
//#include "httpd.h"
//#include "captdns.h"
#include "netbios/netbios.h"
#include "bitband_io.h"


//extern HttpdBuiltInUrl mainInUrls[];


#ifdef WIFI_RESET_SETTINGS_PIN

LOCAL int check_wifi_settings_reset(void)
{
	gpio_t button_gpio;
	int time_tst = 300; // 3 sec low button
#if WIFI_RESET_SETTINGS_PIN == PB_1
    HalDeinitLogUart(); // sys_log_uart_off();
#endif
	gpio_init(&button_gpio, WIFI_RESET_SETTINGS_PIN);
	gpio_dir(&button_gpio, PIN_INPUT);     // Direction: Input
	gpio_mode(&button_gpio, PullUp);       // Pull-High

	while(time_tst) {
		if(gpio_read(&button_gpio)) {
			break;
		};
		vTaskDelay(10);
		time_tst--;
	};

	gpio_deinit(&button_gpio);
#if WIFI_RESET_SETTINGS_PIN == PB_1
	vTaskDelay(1);
	HalPinCtrlRtl8195A(LOG_UART, 0, 1);  // connect LOG UART //	HalInitLogUart(); // sys_log_uart_on();
#endif
	return time_tst;
}

#endif

void user_init_thrd(void) {

#ifdef WIFI_RESET_SETTINGS_PIN
	if (check_wifi_settings_reset() == 0) {
		rtl_printf("No Load Config\n");
		wifi_cfg.load_flg &= ~BID_WIFI_CFG;
	};
#endif

#ifdef GPIO_LED_PIN
	//GpioLedInit();
#endif
//	release_wakelock(15);

	wifi_init();

	netbios_init();

	//captdnsInit();

	debug_printf("[Before espfsInit]: RAM heap\t%d bytes\tTCM heap\t%d bytes\n",
				xPortGetFreeHeapSize(), tcm_heap_freeSpace());

	//EspFsInitResult e = espFsInit((void*)FLASH_APP_BASE);

	debug_printf("[After espfsInit]: RAM heap\t%d bytes\tTCM heap\t%d bytes\n",
				xPortGetFreeHeapSize(), tcm_heap_freeSpace());

	//httpdInit(&mainInUrls, 80);

	//if (e==0)
	//	xTaskCreate(websocketBcast, "wsbcast", 300, NULL, WEBSOC_PRIORITY, NULL);
	//else
	//	error_printf("Espfs not found.\n");

	debug_printf("[After httpdInit]: RAM heap\t%d bytes\tTCM heap\t%d bytes\n",
				xPortGetFreeHeapSize(), tcm_heap_freeSpace());

	/* Initilaize the console stack */
	console_init();

	/* Kill init thread after all init tasks done */
	vTaskDelete(NULL);
}



