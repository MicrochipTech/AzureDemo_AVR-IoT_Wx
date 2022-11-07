/*!
 @file
 	app_main.c

 @brief
 	 This module contains user Application related functions

 @author		M. Abdelmawla
 @date		24 MAY 2013
 @version	1.0
 */

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
 INCLUDES
 *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/


#include "driver/source/m2m_hif.h"
#include "driver/include/m2m_wifi.h"
#include "m2m_test_config.h"
#include "nmi_uart.h"
#include "nmi_gpio.h"
#include "nmi_spi.h"
#include "nmi_btn.h"

#ifdef NMC1003A0
#define WAKEUP_RTS
#endif
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
 MACROS
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
#ifdef NMC1003A0
#define AT_UART 	UART2

#else
#define AT_UART 	UART1
#endif

#define AT_UART_WAEKUP WAKE_GPIO_4

uint8 gbWifiConnected;
static uint8 gau8MACAdress[] = MAC_ADDRESS;
static sint8 gacDeviceName[] = M2M_DEVICE_NAME;

//#define AT_ECHO_ENABLE			/*!< define this MACRO in case you need to see the command characters echoed during typing */

#define AT_CMD_FIFO_SZ			(64)
#define AT_MAX_CMD_SZ			(8)

/*!< AT strings */
#define AT_CMD_START	"AT+"
#define AT_CMD_OK		"\r\n+OK\r\n"
#define AT_CMD_ERR		"\r\n+ERR_000\r\n"

/*!< AT Errors */
#define AT_ERR					(-1)	/*!< Not AT command */
#define AT_ERR_INVALID_CMD		(-2)	/*!< Invalid AT command */

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
 DATA TYPES
 *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

typedef struct {
	uint8 buf[AT_CMD_FIFO_SZ];
	uint16 r_idx;
	uint16 w_idx;
	tstrOsSemaphore sem;
} tstrFifo;

typedef int (*tpf_at_cmd_handler)(tstrFifo*);

typedef struct {
	char cmd[AT_MAX_CMD_SZ];
	tpf_at_cmd_handler handler;
} tstrAtCmd;

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
GLOBALS
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

static uint8 		gau8StackApp[2 * 512];
static tstrOsTask 	gstrTaskApp;
static uint8 		gau8StackM2m[2 * 512];
static tstrOsTask 	gstrTaskM2m;
static tstrFifo 	gstrRxFifo;
tstrOsSemaphore 	gstrAppSem;
static uint8 done = 1;

/*************************************
 * Utility functions
 ************************************/
static int sint16_to_str(char *s, sint16 val)
{
	int i = 4;
	if (val < 0)
	{
		s[0] = '-';
		val = 0 - val;
		i = 5;
	}
	s[i] = val % 10 + 0x30;
	val /= 10;
	s[i - 1] = val % 10 + 0x30;
	val /= 10;
	s[i - 2] = val % 10 + 0x30;
	val /= 10;
	s[i - 3] = val % 10 + 0x30;
	val /= 10;
	s[i - 4] = val % 10 + 0x30;
	return i;
}

static void at_send_err(sint16 err_code)
{
	nm_uart_send(AT_UART,(void*) (AT_CMD_ERR), sizeof(AT_CMD_ERR));
	nm_uart_flush(AT_UART);
#if 0
	int i;
	char tmp[sizeof("\r\n" AT_CMD_ERR "        ")] = "\r\n" AT_CMD_ERR "        ";

	i = sint16_to_str(&tmp[sizeof("\r\n" AT_CMD_ERR)], err_code);
	m2m_memcpy((uint8*) &tmp[sizeof("\r\n" AT_CMD_ERR) + 1 + i], (uint8*) "\r\n", sizeof("\r\n"));
	nm_uart_send(AT_UART,(void*) tmp, sizeof(tmp));
#endif
}

static void at_send_ok(void)
{
	nm_uart_send(AT_UART,(void*) (AT_CMD_OK), sizeof(AT_CMD_OK));
	nm_uart_flush(AT_UART);
}

static void fifo_init(tstrFifo *pstrFifo)
{
	pstrFifo->r_idx = 0;
	pstrFifo->w_idx = 0;
	m2m_memset(pstrFifo->buf, 0, AT_CMD_FIFO_SZ);
	app_os_sem_init(&(pstrFifo->sem), "FIFO", 0);
}

static inline void fifo_put(tstrFifo *pstrFifo, uint8 val)
{
	pstrFifo->buf[pstrFifo->w_idx] = val;
	pstrFifo->w_idx++;
	if (AT_CMD_FIFO_SZ == pstrFifo->w_idx)
		pstrFifo->w_idx = 0;
	app_os_sem_up(&(pstrFifo->sem));
}

static void fifo_get(tstrFifo *pstrFifo, uint8 *buf, uint16 sz)
{
	int i = 0;
	while (sz > 0) {
		app_os_sem_down(&(pstrFifo->sem));
		buf[i++] = pstrFifo->buf[pstrFifo->r_idx];
		pstrFifo->r_idx++;
		if (AT_CMD_FIFO_SZ == pstrFifo->r_idx)
			pstrFifo->r_idx = 0;
		sz--;
	}
}

static int fifo_token_get(tstrFifo *pstrFifo, uint8 *buf, uint16 sz,
		uint16 *ret_sz, char *delimiter, uint8 delimiter_sz)
{
	int i = 0, k;
	int del_found = 0;
	while (sz > 0)
	{
		app_os_sem_down(&(pstrFifo->sem));
		buf[i] = pstrFifo->buf[pstrFifo->r_idx];
		pstrFifo->r_idx++;
		if (AT_CMD_FIFO_SZ == pstrFifo->r_idx)
			pstrFifo->r_idx = 0;
		sz--;
		for (k = 0; k < delimiter_sz; k++)
		{
			if (buf[i] == delimiter[k])
			{
				del_found = 1;
				goto EXIT;
			}
			else
			{
				del_found = 0;
			}
		}
		i++;
	}
	EXIT: *ret_sz = i;
	return del_found;
}

/*************************************
 * AT Commands handlers
 ************************************/
#if 0
static int at_null_cmd_handler(tstrFifo* pstrFifo)
{
	at_send_err(AT_ERR_INVALID_CMD);
	return 0;
}
#endif

static int at_ver_cmd_handler(tstrFifo* pstrFifo)
{
	uint8 tmp[sizeof("\r\n00000.00000")] = "\r\n00000.00000";
	sint16_to_str((char*) &tmp[2], M2M_FIRMWARE_VERSION_MAJOR_NO);
	sint16_to_str((char*) &tmp[8], M2M_FIRMWARE_VERSION_MINOR_NO);
	nm_uart_send(AT_UART,tmp, sizeof(tmp));
	at_send_ok();
	return 0;
}

static int at_scan_cmd_handler(tstrFifo* pstrFifo)
{
	int ret;
	ret = m2m_wifi_request_scan(M2M_WIFI_CH_ALL);
	if (M2M_SUCCESS != ret)
		at_send_err(ret);
	/* the scan command will be completed on wifi_cb reception */
	return 0;
}
static int at_conn_cmd_handler(tstrFifo* pstrFifo)
{
	int ret;
	ret = m2m_wifi_connect(DEFAULT_SSID, sizeof(DEFAULT_SSID),
			DEFAULT_AUTH, DEFAULT_KEY, M2M_WIFI_CH_ALL);
	if (M2M_SUCCESS != ret)
		at_send_err(ret);
	/* the scan command will be completed on wifi_cb reception */
	return 0;
}
const tstrAtCmd gastrAtCmdLst[] = {
		//{ "", at_null_cmd_handler },
		{ "VER", at_ver_cmd_handler },
		{ "SCAN", at_scan_cmd_handler },
		{ "CONN", at_conn_cmd_handler },
};

static tpf_at_cmd_handler get_cmd_handle(char* cmd)
{
	tpf_at_cmd_handler pf_handle = NULL;
	int total_num_cmds = sizeof(gastrAtCmdLst) / sizeof(tstrAtCmd);
	int i;
	for (i = 0; i < total_num_cmds; i++)
	{
		if (!m2m_memcmp((uint8*) cmd, (uint8*) gastrAtCmdLst[i].cmd,
				m2m_strlen((uint8*) gastrAtCmdLst[i].cmd)))
		{
			pf_handle = gastrAtCmdLst[i].handler;
		}
	}
	return pf_handle;
}

/*************************************
 * M2M Stack callback functions
 ************************************/
void wifi_cb(uint8 u8MsgType, void * pvMsg)
{
	static uint8 idx = 0;
	if (u8MsgType == M2M_WIFI_RESP_CON_STATE_CHANGED)
	{
		tstrM2mWifiStateChanged *pstrWifiState =(tstrM2mWifiStateChanged*) pvMsg;
		M2M_INFO("Wifi State \"%s\"\n", pstrWifiState->u8CurrState? "CONNECTED":"DISCONNECTED");
		if (pstrWifiState->u8CurrState == M2M_WIFI_CONNECTED)
		{


		}
		else if (pstrWifiState->u8CurrState == M2M_WIFI_DISCONNECTED)
		{
			gbWifiConnected = M2M_WIFI_DISCONNECTED;
		}
	}
	else if (u8MsgType == M2M_WIFI_REQ_DHCP_CONF)
	{
		uint8 *pu8IPAddress = (uint8*) pvMsg;
		gbWifiConnected = M2M_WIFI_CONNECTED;

		M2M_INFO("DHCP IP Address \"%u.%u.%u.%u\"\n",
					pu8IPAddress[0],pu8IPAddress[1],pu8IPAddress[2],pu8IPAddress[3]);
#ifdef WAKEUP_UART
		nm_gpio_wakeup_ctrl(AT_UART_WAEKUP,1);
		nm_gpio_wakeup_pol(AT_UART_WAEKUP,FAILLING_EDGE);
#endif
		at_send_ok();

	}
	else if (u8MsgType == M2M_WIFI_RESP_GET_SYS_TIME)
	{
		tstrSystemTime *pstrTime = (tstrSystemTime*)pvMsg;
		M2M_INFO("Time Of Day\n\t%d/%02d/%d %02d:%02d:%02d GMT\n",
		pstrTime->u8Month, pstrTime->u8Day, pstrTime->u16Year,
		pstrTime->u8Hour, pstrTime->u8Minute, pstrTime->u8Second);

	}
	else if (M2M_WIFI_RESP_SCAN_DONE == u8MsgType)
	{
		tstrM2mScanDone *pstrState = (tstrM2mScanDone *) pvMsg;
		if (pstrState->s8ScanState == M2M_SUCCESS)
		{
			char tmp[sizeof("\r\n00000")] = "\r\n00000";
			sint16_to_str(&tmp[2], pstrState->u8NumofCh);
			nm_uart_send(AT_UART,(uint8*) tmp, sizeof(tmp));
			idx = 0;
			if(pstrState->u8NumofCh > 0)
			{
				m2m_wifi_req_scan_result(idx);
				idx++;
			}
			else
			{
				at_send_ok();
			}
		}
		else
		{
			idx = 0;
			at_send_err(pstrState->s8ScanState);
		}
	}
	else if (M2M_WIFI_RESP_SCAN_RESULT == u8MsgType)
	{
		tstrM2mWifiscanResult *pstrScanResult = (tstrM2mWifiscanResult*) pvMsg;

		uint8 u8NumFoundAPs = m2m_wifi_get_num_ap_found();

		char tmp[sizeof "000000"] = "000000";
		nm_uart_send(AT_UART,(uint8*) "\r\n", sizeof("\r\n"));
		nm_uart_send(AT_UART,(uint8*) pstrScanResult->au8SSID,
				m2m_strlen(pstrScanResult->au8SSID));
		nm_uart_send(AT_UART,(uint8*) ",", sizeof(","));
		if (1 == pstrScanResult->u8AuthType) {
			nm_uart_send(AT_UART,(uint8*) "OPEN", sizeof("OPEN"));
		} else if (2 == pstrScanResult->u8AuthType) {
			nm_uart_send(AT_UART,(uint8*) "WPA", sizeof("WPA"));
		} else if (3 == pstrScanResult->u8AuthType) {
			nm_uart_send(AT_UART,(uint8*) "WEP", sizeof("WEP"));
		} else if (4 == pstrScanResult->u8AuthType) {
			nm_uart_send(AT_UART,(uint8*) "ENTERPRISE", sizeof("ENTERPRISE"));
		}
		nm_uart_send(AT_UART,(uint8*) ",", sizeof(","));
		sint16_to_str(tmp, pstrScanResult->s8rssi);
		nm_uart_send(AT_UART,(uint8*) tmp, sizeof(tmp));
		nm_uart_send(AT_UART,(uint8*) ",", sizeof(","));
		m2m_memset((uint8*)tmp, (uint8)'0',sizeof(tmp)-1);
		tmp[sizeof(tmp)-2] = 0;
		sint16_to_str(tmp, pstrScanResult->u8ch);
		nm_uart_send(AT_UART,(uint8*) tmp, sizeof(tmp));

		if(idx < u8NumFoundAPs)
		{
			m2m_wifi_req_scan_result(idx);
			idx++;
		}
		else
		{
			at_send_ok();
		}
	}
	nm_uart_flush(AT_UART);
}

void wifi_mgmt_pkt_rsvd(uint8* pu8Buf, uint16 u16Sz) {
	asm ("mov r0, #0\n");
}
#if 0
void AppServerCb(uint8* pu8HostName, uint32 u32ServerIP) {
	asm ("mov r0, #0\n");
}
#endif
#define AT_ECHO_ENABLE
/*************************************
 * UART functions
 ************************************/
void uart_rx_isr(void)
{
	sint8 ret;
	char c = 'E';
	ret = nm_uart_recv(AT_UART,(void*) &c, 1);
	if(ret != NM_SUCCESS)
	{
		//nm_uart_send(AT_UART,(void*) &c, 1);
	}
	else
	{
#ifdef WAKEUP_UART
		if((nm_gpio_wakeup_sts(AT_UART_WAEKUP)) && done)
		{
			char ca = 'A';
			/*
			 * Work around as the first char used to wakeup the chip can't be received by the hardware
			 *
			 */
			if(c != ca)
			{
#ifdef AT_ECHO_ENABLE
				nm_uart_send(AT_UART,(void*) &ca, 1);
#endif
				fifo_put(&gstrRxFifo, ca);
			}
			done = 0;

		}
#endif
		fifo_put(&gstrRxFifo, c);
#ifdef AT_ECHO_ENABLE
		nm_uart_send(AT_UART,(void*) &c, 1);
#endif
	}
}

#define HEX2ASCII(x) (((x)>=10)? (((x)-10)+'A') : ((x)+'0'))
static void set_val_to_str(uint8 * name, uint32 val)
{
	/* Name must be in the format WINC1500_00:00 */
	uint16 len;

	len = m2m_strlen(name);
	if(len >= 5) {
		name[len-2] = HEX2ASCII((val >> 0) & 0x0f);
		name[len-3] = HEX2ASCII((val >> 4) & 0x0f);
		name[len-4] = HEX2ASCII((val >> 8) & 0x0f);
		name[len-5] = HEX2ASCII((val >> 12) & 0x0f);
	}
}
/*************************************
 * Application main
 ************************************/
void app_main_context(void* pv)
{
	uint8 tmp[AT_MAX_CMD_SZ];
	at_send_ok();
	while (1)
	{

		/* get "AT+" */
		fifo_get(&gstrRxFifo, tmp, 3);
		if (m2m_memcmp(tmp, (uint8*) "AT+", 3))
		{
			at_send_err(AT_ERR_INVALID_CMD);
		}
		else
		{
			int found;
			uint16 ret_sz;
			found = fifo_token_get(&gstrRxFifo, tmp, sizeof(tmp), &ret_sz, (char*) ("+" "\r"), 2);
			if (!found)
			{
				M2M_ERR("CMD \"%s\" NOT Supported\n",(char*)tmp);
				at_send_err(AT_ERR_INVALID_CMD);
			}
			else
			{
				tpf_at_cmd_handler pf_handle = get_cmd_handle((char*) tmp);

				if (pf_handle == NULL)
				{
					at_send_err(AT_ERR_INVALID_CMD);
				}
				else
				{
					pf_handle(&gstrRxFifo);
				}
			}
		}
		nm_uart_flush(AT_UART);
#ifdef WAKEUP_UART
		nm_gpio_wakeup_clr_irq(AT_UART_WAEKUP);
		done = 1;
#endif
	}
}

void app_m2m_cb_poll(void *pv) {
	for (;;) {
		app_os_sem_down(&gstrAppSem);
		m2m_wifi_handle_events(NULL);
	}
}
#define HEX2ASCII(x) (((x)>=10)? (((x)-10)+'A') : ((x)+'0'))
static void set_dev_name_to_mac(uint8 * name, uint8 * mac_addr)
{
	/* Name must be in the format WINC1500_00:00 */
	uint16 len;

	len = m2m_strlen(name);
	if(len >= 5) {
		name[len-1] = HEX2ASCII((mac_addr[5] >> 0) & 0x0f);
		name[len-2] = HEX2ASCII((mac_addr[5] >> 4) & 0x0f);
		name[len-4] = HEX2ASCII((mac_addr[4] >> 0) & 0x0f);
		name[len-5] = HEX2ASCII((mac_addr[4] >> 4) & 0x0f);
	}
}

int app_psm_sleep(unsigned int sleep)
{
#ifdef WAKEUP_RTS
	static uint32 mux = 0;

	if(sleep)
	{
		/**
		 * Before sleep, switch rts to high to prevent the host from sending any thing
		 */
		mux = nm_gpio_get_mux(UART2_RTS_GPIO3);
		nm_gpio_output_ctrl(UART2_RTS_GPIO3,1);
	}
	else
	{
		/**
		 * after wakeup, switch rts to low
		 */
		nm_gpio_set_mux(UART2_RTS_GPIO3,mux);
	}
#endif
	return 0;
}

sint8 app_start(void)
{
	uint8 mac_addr[6];
	uint8 u8IsMacAddrValid;
	sint8 ret = M2M_SUCCESS;
	tstrUartConfig strconfig;
	tstrWifiInitParam param;
	M2M_INFO("APP Start\n");
	app_os_sem_init(&gstrAppSem, "APP", 0);
	m2m_memset((uint8*)&param, 0, sizeof(param));
	param.pfAppWifiCb = wifi_cb;

	ret = m2m_wifi_init(&param);
	if(ret != M2M_SUCCESS)
	{
		M2M_ERR("Driver init fail\n");
		goto ERR;
	}
	m2m_wifi_get_otp_mac_address(mac_addr, &u8IsMacAddrValid);
	if(!u8IsMacAddrValid) {
		M2M_INFO("Default MAC\n");
		m2m_wifi_get_mac_address(mac_addr);
	} else {
		M2M_INFO("OTP MAC\n");
	}
	M2M_INFO("MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
		mac_addr[0],mac_addr[1],mac_addr[2],
		mac_addr[3],mac_addr[4],mac_addr[5]);
	set_dev_name_to_mac((uint8*)gacDeviceName, mac_addr);
	//set_dev_name_to_mac((uint8*)gstrM2MAPConfig.au8SSID, mac_addr);
	m2m_wifi_set_device_name((uint8*)gacDeviceName, (uint8)m2m_strlen((uint8*)gacDeviceName));
	fifo_init(&gstrRxFifo);
	{
#ifndef NMC1003A0
		strconfig.u32BaudRate = 115200;
		strconfig.u8EnFlowctrl = 0;
		strconfig.u8TXGpioPin = UART1_TX_GPIO7;
		strconfig.u8RxGpioPin = UART1_RX_GPIO8;
#else
		strconfig.u32BaudRate = 115200;
		strconfig.u8EnFlowctrl = 1;
		strconfig.u8TXGpioPin = UART2_TX_GPIO6;
		strconfig.u8RxGpioPin = UART2_RX_GPIO4;
		strconfig.u8RtsGpioPin = UART2_RTS_GPIO3;
		strconfig.u8CtsGpioPin = UART2_CTS_GPIO5;
#endif
		nm_uart_init(AT_UART, &strconfig); // default baud rate is 115200
	}
#ifdef NMC1003A0
	m2m_wifi_set_power_profile(PWR_DEFAULT);
#endif
#ifndef NMC1003A0
	m2m_wifi_set_sleep_mode(M2M_PS_H_AUTOMATIC,1);
#else
	m2m_wifi_set_sleep_mode(M2M_PS_DEEP_AUTOMATIC,1);
#endif

	nm_uart_register_rx_isr(AT_UART,uart_rx_isr);
	app_os_sch_task_create(&gstrTaskApp, app_main_context, "APP", gau8StackApp,
			sizeof(gau8StackApp), 100);
	app_os_sch_task_create(&gstrTaskM2m, app_m2m_cb_poll, "M2M_POLL",
			gau8StackM2m, sizeof(gau8StackM2m), 100);
ERR:
	return ret;
}
