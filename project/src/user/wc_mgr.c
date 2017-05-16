#include "FreeRTOS.h"
#include "task.h"
#include "diag.h"
#include "objects.h"
#include "osdep_service.h"
#include "device_lock.h"
#include "semphr.h"
#include "tcm_heap.h"


#if CONFIG_WLAN
#include "wifi_conf.h"
#include "wlan_intf.h"
#include "wifi_constants.h"
#include "wifi_lib.h"

#include <wlan/wlan_test_inc.h>
#include <wifi/wifi_conf.h>
#include <wifi/wifi_util.h>
#endif
#include "lwip_netconf.h"
#include <platform/platform_stdlib.h>
#include <dhcp/dhcps.h>
#include <lwip_netconf.h>
#include "tcpip.h"

#include "main.h"
#include "wc_mgr.h"
#include "http_server.h"

//rtw_mode_t wifi_mode = RTW_MODE_STA_AP; //RTW_MODE__NONE;


#if CONFIG_LWIP_LAYER
extern struct netif xnetif[NET_IF_NUM];
#endif




typedef int (*wlan_init_done_ptr)(void);
typedef int (*write_reconnect_ptr)(uint8_t *data, uint32_t len);
extern init_done_ptr p_wlan_init_done_callback;
//extern write_reconnect_ptr p_write_reconnect_ptr;

static rtw_ap_info_t ap = {0};

static rtw_network_info_t wifi = {
		{0},    // ssid
		{0},    // bssid
		0,      // security
		NULL,   // password
		0,      // password len
		-1      // key id
};

rtw_wifi_setting_t wifi_setting = {RTW_MODE_STA_AP, {0}, 0, RTW_SECURITY_OPEN, {0}};



#if 0
static void init_wifi_struct(void)
{
	memset(wifi.ssid.val, 0, sizeof(wifi.ssid.val));
	memset(wifi.bssid.octet, 0, ETH_ALEN);
	//memset(password, 0, sizeof(password));
	wifi.ssid.len = 0;
	wifi.password = NULL;
	wifi.password_len = 0;
	wifi.key_id = -1;
	memset(ap.ssid.val, 0, sizeof(ap.ssid.val));
	ap.ssid.len = 0;
	ap.password = NULL;
	ap.password_len = 0;
	ap.channel = 1;
}

static void get_wifi_setting_from_if()
{
	const char *ifname = WLAN0_NAME;

	if(rltk_wlan_running(WLAN1_IDX))
	{//STA_AP_MODE
		ifname = WLAN1_NAME;
	}

	wifi_get_setting(ifname, &wifi_setting);

	rtl_printf("LoadWifiSetting(): wifi_setting.ssid=%s\n", wifi_setting.ssid);
	rtl_printf("LoadWifiSetting(): wifi_setting.channel=%d\n", wifi_setting.channel);
	rtl_printf("LoadWifiSetting(): wifi_setting.security_type=%d\n", wifi_setting.security_type);
	rtl_printf("LoadWifiSetting(): wifi_setting.password=%s\n", wifi_setting.password);
}
#endif

void show_info(uint8_t wmask) {
	rtw_wifi_setting_t Setting;

	if (wmask & 1) {
		wifi_get_setting(WLAN0_NAME, &Setting);
		wifi_show_setting(WLAN0_NAME, &Setting);
	}
	if (wmask & 2) {
		wifi_get_setting(WLAN1_NAME, &Setting);
		wifi_show_setting(WLAN1_NAME, &Setting);
	}
}

_WEAK void connect_start(void)
{
	rtl_printf("%s\n", __FUNCTION__);
}

_WEAK void connect_close(void)
{
	rtl_printf("%s\n", __FUNCTION__);
}

int wlan_reconnect(u8 *data, uint32_t len) {
	rtl_printf("%s\n", __FUNCTION__);
	return 0;
}
int write_reconnect_cb(u8 *data, uint32_t len)
{
	rtl_printf("%s\n", __FUNCTION__);
	return 0;
}

extern void (*p_wlan_autoreconnect_hdl)(rtw_security_t, char*, int, char*, int, int);
extern void wifi_autoreconnect_hdl  (rtw_security_t, char*, int, char*, int, int);
// (not work if lib_wlan_mp.a ?)
int sta_autoreconnect_setup(void)
{
	p_wlan_autoreconnect_hdl = wifi_autoreconnect_hdl;
    //WLAN0_NAME, mode, retry_times, timeout
    return wext_set_autoreconnect(WLAN0_NAME, 1, 20, 7);
}

void reset_wifi_settings(void) {
	char* ssid;

	rtl_printf("%s\n", __func__);

	//--------- Default wi-fi settigns ---------
	// STATION settings
	ssid = "HOMEAP";
	wifi.ssid.len = strlen(ssid);
	memset((void*)&wifi.ssid.val[0], 0, sizeof(&wifi.ssid.val));
	memcpy((void*)&wifi.ssid.val[0], (void*)ssid, wifi.ssid.len);
	wifi.security_type = RTW_SECURITY_OPEN; //RTW_SECURITY_WPA2_AES_PSK;
	wifi.password = "0123456789"; //
	wifi.password_len = strlen(wifi.password);
	wifi.key_id =0;

	// AP settings
	ssid = "RTL8710";
	ap.ssid.len = strlen(ssid);
	memset(ap.ssid.val, 0, sizeof(ap.ssid.val));
	memcpy((void*)&ap.ssid.val[0], (void*)ssid, ap.ssid.len);
	ap.security_type = RTW_SECURITY_OPEN;  // RTW_SECURITY_WPA2_AES_PSK
	ap.channel = 1;
	ap.password = "0123456789";
	ap.password_len = strlen(ap.password);
	//------------------------------------------
}
void load_wifi_settings(void) {
	rtl_printf("%s\n", __func__);
#if 0
	// ...
	// load settings from flash
	// ...
#else
	reset_wifi_settings();
#endif
}


void wc_start(void) {
	int  ret;
	void *semaphore = NULL;
	int timeout = 10000/200;




	p_wlan_init_done_callback = NULL;



	LwIP_Init();

	if (wifi_on(RTW_MODE_STA_AP) < 0) {
		printf("ERROR: Wifi on failed!\n");
		goto exit_fail;
	}

	// stop dhcp server and client
	dhcps_deinit();
	LwIP_DHCP(0, DHCP_STOP);
	LwIP_DHCP(1, DHCP_STOP);

	rtl_printf("Starting AP ...\n");
	ret = wifi_start_ap(
			ap.ssid.val,		//char  *ssid,
			ap.security_type,	//rtw_security_t ecurity_type,
			ap.password, 		//char *password,
			ap.ssid.len,		//int ssid_len,
			ap.password_len, 	//int password_len,
			ap.channel			//int channel
	);
	if (ret != RTW_SUCCESS) {
		rtl_printf("ERROR: Operation failed!\n\n");
		return;
	}
	while(1) {
		char essid[33];

		if(wext_get_ssid(WLAN1_NAME, (unsigned char *) essid) > 0) {
			if(strcmp((const char *) essid, (const char *)ap.ssid.val) == 0) {
				rtl_printf("%s started\n", ap.ssid.val);
				ret = RTW_SUCCESS;
				break;
			}
		}

		if(timeout == 0) {
			rtl_printf("ERROR: Start AP timeout!\n");
			ret = RTW_TIMEOUT;
			break;
		}
		//vTaskDelay(1 * configTICK_RATE_HZ);
		vTaskDelay(200/portTICK_RATE_MS);
		timeout --;
	}
	rtl_printf("[AP_on]: RAM heap\t%d bytes\tTCM heap\t%d bytes\n",
			xPortGetFreeHeapSize(), tcm_heap_freeSpace());
#if CONFIG_LWIP_LAYER
	LwIP_UseStaticIP(&xnetif[1]);
#ifdef CONFIG_DONT_CARE_TP
	pnetiff->flags |= NETIF_FLAG_IPSWITCH;
#endif
	dhcps_init(&xnetif[1]);
#endif

	show_info(2);

	sta_autoreconnect_setup();

	rtl_printf("[Wifi_on]: RAM heap\t%d bytes\tTCM heap\t%d bytes\n",
			xPortGetFreeHeapSize(), tcm_heap_freeSpace());

	ret = wifi_connect(wifi.ssid.val,
			wifi.security_type,
			wifi.password,
			wifi.ssid.len,
			wifi.password_len,
			wifi.key_id,
			semaphore);

	rtl_printf("Connected to AP\n");
	rtl_printf("[STA_connected]: RAM heap\t%d bytes\tTCM heap\t%d bytes\n",
			xPortGetFreeHeapSize(), tcm_heap_freeSpace());
	if(ret != RTW_SUCCESS) {
		rtl_printf("ERROR: Operation failed! Error=%d\n", ret);
		//goto exit_fail;
	} else {
		// Start DHCPClient
		LwIP_DHCP(0, DHCP_START);
		rtl_printf("Got IP\n");

#if CONFIG_WLAN_CONNECT_CB
		//	extern void connect_start(void);
		connect_start();
#endif

	}



	show_info(3);

	httpd_start();



	exit_fail:
	vTaskDelete(NULL);
}



