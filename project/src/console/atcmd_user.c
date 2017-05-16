#include <platform_opts.h>

#ifdef CONFIG_AT_USR

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "at_cmd/log_service.h"
#include "at_cmd/atcmd_wifi.h"
#include <lwip_netconf.h>
#include "tcpip.h"
#include <dhcp/dhcps.h>
#include <wifi/wifi_conf.h>
#include <wifi/wifi_util.h>
#include "tcm_heap.h"
#include "rtl8195a/rtl_libc.h"

#include "sleep_ex_api.h"

#include "lwip/tcp_impl.h"

extern char str_rom_57ch3Dch0A[]; // "=========================================================\n" 57

#define printf rtl_printf // DiagPrintf

/* RAM/TCM/Heaps info */
extern void ShowMemInfo(void);
/*
void ShowMemInfo(void)
{
	printf("\nCLK CPU\t\t%d Hz\nRAM heap\t%d bytes\nTCM heap\t%d bytes\n",
			HalGetCpuClk(), xPortGetFreeHeapSize(), tcm_heap_freeSpace());
}
 */
//------------------------------------------------------------------------------
// Mem, Tasks info
//------------------------------------------------------------------------------
LOCAL void fATST(int argc, char *argv[]) {
		ShowMemInfo();
#if 0 //CONFIG_DEBUG_LOG > 1
		dump_mem_block_list();
		tcm_heap_dump();
#endif;
		printf("\n");
#if (configGENERATE_RUN_TIME_STATS == 1)
		char *cBuffer = pvPortMalloc(512);
		if(cBuffer != NULL) {
			vTaskGetRunTimeStats((char *)cBuffer);
			printf("%s", cBuffer);
		}
		vPortFree(cBuffer);
#endif
#if defined(configUSE_TRACE_FACILITY) && (configUSE_TRACE_FACILITY == 1) && (configUSE_STATS_FORMATTING_FUNCTIONS == 1)
	{
		char * pcWriteBuffer = malloc(1024);
		if(pcWriteBuffer) {
			vTaskList((char*)pcWriteBuffer);
			printf("\nTask List:\n");
			printf(&str_rom_57ch3Dch0A[7]); // "==========================================\n"
	        printf("Name\t  Status Priority HighWaterMark TaskNumber\n%s\n", pcWriteBuffer);
			free(pcWriteBuffer);
		}
	}
#endif
}
/*-------------------------------------------------------------------------------------
 Копирует данные из области align(4) (flash, registers, ...) в область align(1) (ram)
--------------------------------------------------------------------------------------*/
extern void copy_align4_to_align1(unsigned char * pd, void * ps, unsigned int len);
/*
static void copy_align4_to_align1(unsigned char * pd, void * ps, unsigned int len)
{
	union {
		unsigned char uc[4];
		unsigned int ud;
	}tmp;
	unsigned int *p = (unsigned int *)((unsigned int)ps & (~3));
	unsigned int xlen = (unsigned int)ps & 3;
	//	unsigned int size = len;

	if(xlen) {
		tmp.ud = *p++;
		while (len)  {
			len--;
			*pd++ = tmp.uc[xlen++];
			if(xlen & 4) break;
		}
	}
	xlen = len >> 2;
	while(xlen) {
		tmp.ud = *p++;
		*pd++ = tmp.uc[0];
		*pd++ = tmp.uc[1];
		*pd++ = tmp.uc[2];
		*pd++ = tmp.uc[3];
		xlen--;
	}
	if(len & 3) {
		tmp.ud = *p;
		pd[0] = tmp.uc[0];
		if(len & 2) {
			pd[1] = tmp.uc[1];
			if(len & 1) {
				pd[2] = tmp.uc[2];
			}
		}
	}
	//	return size;
}
*/
int print_hex_dump(uint8_t *buf, int len, unsigned char k) {
	uint32_t ss[2];
	ss[0] = 0x78323025; // "%02x"
	ss[1] = k;	// ","...'\0'
	uint8_t * ptr = buf;
	int result = 0;
	while (len--) {
		if (len == 0)
			ss[1] = 0;
		result += printf((uint8_t *) &ss, *ptr++);
	}
	return result;
}

extern char str_rom_hex_addr[]; // in *.ld "[Addr]   .0 .1 .2 .3 .4 .5 .6 .7 .8 .9 .A .B .C .D .E .F\n"

void dump_bytes(uint32 addr, int size)
{
	uint8 buf[17];
	u32 symbs_line = sizeof(buf)-1;
	printf(str_rom_hex_addr);
	while (size) {
		if (symbs_line > size) symbs_line = size;
		printf("%08X ", addr);
		copy_align4_to_align1(buf, addr, symbs_line);
		print_hex_dump(buf, symbs_line, ' ');
		int i;
		for(i = 0 ; i < symbs_line ; i++) {
			if(buf[i] < 0x20 || buf[i] > 0x7E) {
				buf[i] = '.';
			}
		}
		buf[symbs_line] = 0;
		i = (sizeof(buf)-1) - symbs_line;
		while(i--) printf("   ");
		printf(" %s\r\n", buf);
		addr += symbs_line;
		size -= symbs_line;
	}
}
//------------------------------------------------------------------------------
// Dump byte register
//------------------------------------------------------------------------------
LOCAL void fATSB(int argc, char *argv[])
{
/*
	int size = 16;
	uint32 addr = Strtoul(argv[1],0,16);
	if (argc > 2) {
		size = Strtoul(argv[2],0,10);
		if (size <= 0 || size > 16384)
			size = 16;
	}
	if(addr + size > SPI_FLASH_BASE) {
		flash_turnon();
		dump_bytes(addr, size);
		SpicDisableRtl8195A();
	}
	else {
		dump_bytes(addr, size);
	}
*/
}

//------------------------------------------------------------------------------
// Dump dword register
//------------------------------------------------------------------------------
LOCAL void fATSD(int argc, char *argv[])
{
/*
	if (argc > 2) {
		int size = Strtoul(argv[2],0,10);
		if (size <= 0 || size > 16384)
			argv[2] = "16";
	}
*/
	flash_turnon();
	CmdDumpWord(argc-1, (unsigned char**)(argv+1));
	SpicDisableRtl8195A();
}
//------------------------------------------------------------------------------
// Write dword register
//------------------------------------------------------------------------------
LOCAL void fATSW(int argc, char *argv[])
{
	CmdWriteWord(argc-1, (unsigned char**)(argv+1));
}

/* Get one byte from the 4-byte address */
#define ip4_addr1(ipaddr) (((u8_t*)(ipaddr))[0])
#define ip4_addr2(ipaddr) (((u8_t*)(ipaddr))[1])
#define ip4_addr3(ipaddr) (((u8_t*)(ipaddr))[2])
#define ip4_addr4(ipaddr) (((u8_t*)(ipaddr))[3])
/* These are cast to u16_t, with the intent that they are often arguments
 * to printf using the U16_F format from cc.h. */
#define ip4_addr1_16(ipaddr) ((u16_t)ip4_addr1(ipaddr))
#define ip4_addr2_16(ipaddr) ((u16_t)ip4_addr2(ipaddr))
#define ip4_addr3_16(ipaddr) ((u16_t)ip4_addr3(ipaddr))
#define ip4_addr4_16(ipaddr) ((u16_t)ip4_addr4(ipaddr))

#define IP2STR(ipaddr) ip4_addr1_16(ipaddr), \
    ip4_addr2_16(ipaddr), \
    ip4_addr3_16(ipaddr), \
    ip4_addr4_16(ipaddr)

#define IPSTR "%d.%d.%d.%d"

extern const char * const tcp_state_str[];
/*
static const char * const tcp_state_str[] = {
  "CLOSED",
  "LISTEN",
  "SYN_SENT",
  "SYN_RCVD",
  "ESTABLISHED",
  "FIN_WAIT_1",
  "FIN_WAIT_2",
  "CLOSE_WAIT",
  "CLOSING",
  "LAST_ACK",
  "TIME_WAIT"
};
*/
/******************************************************************************
 * FunctionName : debug
 * Parameters   :
 * Returns      :
*******************************************************************************/
void print_udp_pcb(void)
{
  struct udp_pcb *pcb;
  bool prt_none = true;
  rtl_printf("UDP pcbs:\n");
  for(pcb = udp_pcbs; pcb != NULL; pcb = pcb->next) {
	  rtl_printf("flg:%02x\t" IPSTR ":%d\t" IPSTR ":%d\trecv:%p\n", pcb->flags, IP2STR(&pcb->local_ip), pcb->local_port, IP2STR(&pcb->remote_ip), pcb->remote_port, pcb->recv );
	  prt_none = false;
  };
  if(prt_none) rtl_printf("none\n");
}
/******************************************************************************
 * FunctionName : debug
 * Parameters   :
 * Returns      :
*******************************************************************************/
void print_tcp_pcb(void)
{
  struct tcp_pcb *pcb;
  rtl_printf("Active PCB states:\n");
  bool prt_none = true;
  for(pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
     rtl_printf("Port %d|%d\tflg:%02x\ttmr:%p\t%s\n", pcb->local_port, pcb->remote_port, pcb->flags, pcb->tmr, tcp_state_str[pcb->state]);
     prt_none = false;
  };
  if(prt_none) rtl_printf("none\n");
  rtl_printf("Listen PCB states:\n");
  prt_none = true;
  for(pcb = (struct tcp_pcb *)tcp_listen_pcbs.pcbs; pcb != NULL; pcb = pcb->next) {
    rtl_printf("Port %d|%d\tflg:%02x\ttmr:%p\t%s\n", pcb->local_port, pcb->remote_port, pcb->flags, pcb->tmr, tcp_state_str[pcb->state]);
    prt_none = false;
  };
  if(prt_none) rtl_printf("none\n");
  rtl_printf("TIME-WAIT PCB states:\n");
  prt_none = true;
  for(pcb = tcp_tw_pcbs; pcb != NULL; pcb = pcb->next) {
    rtl_printf("Port %d|%d\tflg:%02x\ttmr:%p \t%s\n", pcb->local_port, pcb->remote_port, pcb->flags, pcb->tmr, tcp_state_str[pcb->state]);
    prt_none = false;
  };
  if(prt_none) rtl_printf("none\n");
}
/******************************************************************************
 * FunctionName : debug
 * Parameters   :
 * Returns      :
*******************************************************************************/
LOCAL void fATLW(int argc, char *argv[]) 	// Info Lwip
{
	print_udp_pcb();
	print_tcp_pcb();
}
//------------------------------------------------------------------------------
// Deep sleep
//------------------------------------------------------------------------------
LOCAL void fATDS(int argc, char *argv[])
{
    uint32 sleep_ms = 10000;
    if(argc > 1) sleep_ms = atoi(argv[1]);
#if 0    
    if(argc > 2) {
            printf("%u ms waiting low level on PB_1 before launching Deep-Sleep...\n", sleep_ms);
            // turn off log uart
            HalDeinitLogUart(); // sys_log_uart_off();

           // initialize wakeup pin
           gpio_t gpio_wake;
           gpio_init(&gpio_wake, PB_1);
           gpio_dir(&gpio_wake, PIN_INPUT);
           gpio_mode(&gpio_wake, PullDown);
           TickType_t sttime = xTaskGetTickCount();

           do {
              if(gpio_read(&gpio_wake) == 0) {
                   // Enter deep sleep... Wait give rising edge at PB_1 to wakeup system.
                   deepsleep_ex(DSLEEP_WAKEUP_BY_GPIO, 0);
              };
              vTaskDelay(1);
           } while(xTaskGetTickCount() - sttime < sleep_ms);
           HalInitLogUart(); // sys_log_uart_on();
            printf("No set pin low in deep sleep!\n");
    }
    else {
        printf("Deep-Sleep %u ms\n", sleep_ms);
        HalLogUartWaitTxFifoEmpty();
        // Enter deep sleep... Wait timer ms
        deepsleep_ex(DSLEEP_WAKEUP_BY_TIMER, sleep_ms);
    }
#else
    HalLogUartWaitTxFifoEmpty();
    deepsleep_ex(DSLEEP_WAKEUP_BY_TIMER, sleep_ms);
#endif    
}
/*------------------------------------------------------------------------------
 * power saving mode
 *----------------------------------------------------------------------------*/
LOCAL void fATSP(int argc, char *argv[])
{
	if(argc > 2) {
		switch (argv[1][0]) {
		case 'a': // acquire
		{
			acquire_wakelock(atoi(argv[2]));
			break;
		}
		case 'r': // release
		{
			release_wakelock(atoi(argv[2]));
			break;
		}
		};
	};
	printf("WakeLock Status %d\n", get_wakelock_status());
}
//------------------------------------------------------------------------------
#include "wlan_lib.h"
#include "hal_com_reg.h" 

extern struct netif xnetif[NET_IF_NUM];

u8 RandSK256() {
	u8 buf;
	rtw_get_random_bytes(&buf, 1);
	return buf;
}

u8 RandSK(u8 max) {
	u8 buf;
	do {
		rtw_get_random_bytes(&buf, 1);
	} while (buf>max);
	return buf;
}
#ifdef CONFIG_PROMISC
void fATWM(int argc, char *argv[]){ 
        argv[0] = "wifi_promisc";        
		printf("[ATWM]x: _AT_WLAN_PROMISC_\n");
		char mac[100];
		wifi_get_mac_address(&mac);
		printf("\n\r%s\r\n", mac);
		//printf("set\n\r");
		//char mac2[100]="123456789012";
		//wifi_set_mac_address(&mac2);
		//printf("get\n\r");
		//wifi_get_mac_address(&mac);
		//printf("\n\r%s\r\n", mac);
		if(1) {
			int out_buf_size = 40;
			unsigned char buf[40];
			u8 *mac = (u8 *)LwIP_GetMAC(&xnetif[0]);
			memcpy(buf,' ',40);
			//sprintf(buf, ",,,B,%02x%02x%02x%02x%02x%02x,", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
			sprintf(buf, "0123456789");
			struct pbuf * p = pbuf_alloc(PBUF_TRANSPORT, out_buf_size * sizeof(char), PBUF_REF);
			p->payload = buf;
			
			struct udp_pcb *broadcast_pcb;
			struct ip_addr forward_ip;
			#define fwd_port 5001

			broadcast_pcb = udp_new();
			//38.132.37.5
			//IP4_ADDR(&forward_ip, 38, 132, 37, 5);
			//192.168.1.3
			IP4_ADDR(&forward_ip, 192, 168, 1, 3);
			//IP4_ADDR(&forward_ip, 192, 168, 1, 1);
			
			int ret = udp_sendto(broadcast_pcb, p, &forward_ip, fwd_port); //dest port
			pbuf_free(p);	
			printf(">>>button UDP message sent %d==>\n", ret);		
			//test_rtk();
				printf("&rltk_wlan_info= %p\n", &rltk_wlan_info);
				dump_bytes((u32)&rltk_wlan_info, 64);
				printf("[0]\n");
				_adapter *ad = rltk_wlan_info[0].dev;
				printf("adapter = %p\n", ad);
				dump_bytes((u32)ad, 16);
				printf("bSurpriseRemoved [%p] = %d, +%d\n", &ad->bSurpriseRemoved, ad->bSurpriseRemoved, (u32)&ad->bSurpriseRemoved - (u32)ad);
				dump_bytes((u32)&ad->bSurpriseRemoved, 32);
				printf("mlmepriv +%d, mlmeextpriv +%d, cmdpriv +d%\n",
						(u32)(&ad->mlmepriv) - (u32)(ad),
						(u32)(&ad->mlmeextpriv) - (u32)(ad),
						(u32)(&ad->cmdpriv) - (u32)(ad));
				printf("xmitpriv +%d, recvpriv +%d, stapriv +%d\n",
						(u32)(&ad->xmitpriv) - (u32)(ad), (u32)(&ad->recvpriv) - (u32)(ad),
						(u32)(&ad->stapriv) - (u32)(ad));
				printf("bDriverStopped %d, hw_init_completed %d, cmdThread %d\n",
						(u32)(&ad->bDriverStopped) - (u32)(ad),
						(u32)(&ad->hw_init_completed) - (u32)(ad),
						(u32)(&ad->cmdThread) - (u32)(ad));
				printf("intf_start %d, bup %d, pcodatapriv %d\n",
						(u32)(&ad->intf_start) - (u32)(ad), (u32)(&ad->bup) - (u32)(ad),
						(u32)(&ad->pcodatapriv) - (u32)(ad));
				printf("Sizeof(adapter) = %d\n", sizeof(struct _ADAPTER));
				if (sizeof(struct _ADAPTER) != 6088) {
					printf("Error: Check aligned WiFi struct!\n");
				}		
				printf("[1]\n");
				ad = rltk_wlan_info[1].dev;
				printf("adapter = %p\n", ad);
				dump_bytes((u32)ad, 16);
				printf("bSurpriseRemoved [%p] = %d, +%d\n", &ad->bSurpriseRemoved, ad->bSurpriseRemoved, (u32)&ad->bSurpriseRemoved - (u32)ad);
				dump_bytes((u32)&ad->bSurpriseRemoved, 32);
				printf("mlmepriv +%d, mlmeextpriv +%d, cmdpriv +d%\n",
						(u32)(&ad->mlmepriv) - (u32)(ad),
						(u32)(&ad->mlmeextpriv) - (u32)(ad),
						(u32)(&ad->cmdpriv) - (u32)(ad));
				printf("xmitpriv +%d, recvpriv +%d, stapriv +%d\n",
						(u32)(&ad->xmitpriv) - (u32)(ad), (u32)(&ad->recvpriv) - (u32)(ad),
						(u32)(&ad->stapriv) - (u32)(ad));
				printf("bDriverStopped %d, hw_init_completed %d, cmdThread %d\n",
						(u32)(&ad->bDriverStopped) - (u32)(ad),
						(u32)(&ad->hw_init_completed) - (u32)(ad),
						(u32)(&ad->cmdThread) - (u32)(ad));
				printf("intf_start %d, bup %d, pcodatapriv %d\n",
						(u32)(&ad->intf_start) - (u32)(ad), (u32)(&ad->bup) - (u32)(ad),
						(u32)(&ad->pcodatapriv) - (u32)(ad));
				printf("Sizeof(adapter) = %d\n", sizeof(struct _ADAPTER));
				if (sizeof(struct _ADAPTER) != 6088) {
					printf("Error: Check aligned WiFi struct!\n");
				}		
				printf("adhoc\n");
				printf("rltk_wlan_info[0].dev\n");
				dump_bytes(&(rltk_wlan_info[0]),32);
				dump_bytes(&(rltk_wlan_info[0].mac),6);
				printf("rltk_wlan_info[1].dev\n");
				dump_bytes(&(rltk_wlan_info[1]),32);
				dump_bytes(&(rltk_wlan_info[1].mac),6);
				
			printf("prep A\n");
			//return;
			char wlan0[6] = "wlan0";
			char mac1[6] = "123456";
			char ssid[6] = "123456";
			u8 alfa[65] = "1234567890qwertyuiopasdfghjklzxcvbnm QWERTYUIOPASDFGHJKLZXCVBNM_.";
			u8 packet[128] = { 0x80, 0x00, 0x00, 0x00, 
                /*4*/   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
                /*10*/  0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                /*16*/  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 
                /*22*/  0xc0, 0x6c, 
                /*24*/  0x83, 0x51, 0xf7, 0x8f, 0x0f, 0x00, 0x00, 0x00, 
                /*32*/  0x64, 0x00, 
                /*34*/  0x01, 0x04, 
                /* SSID */
                /*36*/  0x00, 0x06, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72,
                        0x01, 0x08, 0x82, 0x84,
                        0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, 0x03, 0x01, 
                /*56*/  0x04};                      
			u8 channel;
			//_adapter *ad0 = rltk_wlan_info[0].dev;
			while (1) {
				if(0) {
					channel = 1; //RandSK(11)+1; 
					// Source MAC Address
					packet[10] = packet[16] = 1;//RandSK256();
					packet[11] = packet[17] = 1;//RandSK256();
					packet[12] = packet[18] = 1;//RandSK256();
					packet[13] = packet[19] = 1;//RandSK256();
					packet[14] = packet[20] = 1;//RandSK256();
					packet[15] = packet[21] = 1;//RandSK256();			
				} else {
					channel = RandSK(11)+1; 
					// Source MAC Address
					packet[10] = packet[16] = RandSK256();
					packet[11] = packet[17] = RandSK256();
					packet[12] = packet[18] = RandSK256();
					packet[13] = packet[19] = RandSK256();
					packet[14] = packet[20] = RandSK256();
					packet[15] = packet[21] = RandSK256();			
				}
				// Randomize SSID (Fixed size 6. Lazy right?)
				packet[38] = alfa[RandSK(65)];
				packet[39] = alfa[RandSK(65)];
				packet[40] = alfa[RandSK(65)];
				packet[41] = alfa[RandSK(65)];
				packet[42] = alfa[RandSK(65)];
				packet[43] = alfa[RandSK(65)];
				
				packet[56] = channel;
				wext_set_channel(wlan0, channel);
				rtw_msleep_os(100);			
				const TickType_t xDelay = 1000 / portTICK_PERIOD_MS; //1000ms
				//while(1) {
				memcpy(&mac1,&packet[10],6);
				memcpy(&ssid,&packet[38],6);
				printf("> ch,mac,ssid = %d,%12X,%s", channel,mac,ssid);										
				for(int i=0;i<4;i++) {
					//rtw_send_mgnt(ad0, &packet, 57, NULL);
					wext_send_mgnt(wlan0, &packet, 57, NULL);
					printf(".");
					taskYIELD();
					//vTaskDelay( xDelay );
					if(0) {
						ShowMemInfo();
						dump_mem_block_list();
						tcm_heap_dump();
					}
					//rtw_msleep_os(100);
				}
			}		
			if(1) {
				printf("rltk_wlan_info = %p\n", &rltk_wlan_info);
				int size = 512;
				int addr = 0;
				uint8_t *blk_data = (uint8_t *)malloc(size);
				memset(blk_data, 0xff, size);
				if(blk_data) {
					uint8_t * ptr = blk_data;
					//Hal_ReadEFuse(*(_adapter **)(rltk_wlan_info[0].dev->priv), 0, 0, 512, ptr, 1);
					//rtw_flash_map_update(*(_adapter **)(rltk_wlan_info->priv), 512);
					u32 symbs_line = 16;
					while(addr < size) {
						if(symbs_line > size) symbs_line = size;
						printf("%08X ", addr);
						print_hex_dump(ptr, symbs_line, ' ');
						printf("\r\n");
						addr += symbs_line;
						ptr += symbs_line;
						size -= symbs_line;
						if(size == 0) break;
					}
					free(blk_data);
				}		
			}
		}
        if(0){
          printf("[ATWM]Usage: ATWM=DURATION_SECONDS[with_len]\n");
#if CONFIG_INIC_CMD_RSP
		inic_c2h_msg("ATWM", RTW_BADARG, NULL, 0);
#endif
          return;
        }
		cmd_promisc(2,argv);
		return;
#if CONFIG_INIC_CMD_RSP
		inic_frame_tail = inic_frame = pvPortMalloc(sizeof(struct inic_eth_frame)*MAX_INIC_FRAME_NUM);
		if(inic_frame == NULL){
			inic_c2h_msg("ATWM", RTW_BUFFER_UNAVAILABLE_TEMPORARY, NULL, 0);
			return;
		}
#endif
#ifdef CONFIG_PROMISC
		wifi_init_packet_filter();
#endif     
		//promisc_test(1000, 0);
#if CONFIG_INIC_CMD_RSP
		if(inic_frame)
			vPortFree(inic_frame);
		inic_frame_tail = NULL;
		inic_frame_cnt = 0;
#endif		
}
#endif
//------------------------------------------------------------------------------
MON_RAM_TAB_SECTION COMMAND_TABLE console_commands_at[] = {
		{"ATST", 0, fATST, ": Memory info"},
		{"ATLW", 0, fATLW, ": LwIP Info"},
		{"ATWM", 0, fATWM, ": Prom"},
		{"ATSB", 1, fATSB, "=<ADDRES(hex)>[,COUNT(dec)]: Dump byte register"},
		{"ATSD", 1, fATSD, "=<ADDRES(hex)>[,COUNT(dec)]: Dump dword register"},
		{"ATSW", 2, fATSW, "=<ADDRES(hex)>,<DATA(hex)>: Set register"},
		{"ATDS", 0, fATDS, "=[TIME(ms)]: Deep sleep"},
		{"ATSP", 0, fATSP, "=<a,r>,<wakelock_status:1|2|4|8>: Power"}
};

#endif //#ifdef CONFIG_AT_USR
