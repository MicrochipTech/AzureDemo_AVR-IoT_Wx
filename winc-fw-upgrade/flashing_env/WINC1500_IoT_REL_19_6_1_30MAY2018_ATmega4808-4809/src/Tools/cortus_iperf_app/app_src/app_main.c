/*!
@file		app_main.c

@brief		This module contains user Application related functions

@author		M. Abdelmawla
@date		24 MAY 2013
@version	1.0
*/

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
INCLUDES
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/


#include "driver/source/m2m_hif.h"

#include "m2m_test_config.h"
#include "driver/include/m2m_wifi.h"
#include "socket/include/socket.h"
#include "iperf.h"

#include "nmi_uart.h"
#include "nmi_gpio.h"
#include "nmi_spi.h"
#include "nmi_btn.h"


/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
MACROS
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

#define APP_SEM_POST(pSem)					app_os_sem_up(pSem)

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
PRIVATE DATA TYPES
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/


typedef enum
{
	ACT_REQ_CONNECT					= 1,
	ACT_REQ_CONNECT_LIST			= 2,
	ACT_REQ_DISCONNECT				= 3,
	ACT_REQ_START_1ST_TIME_CONT		= 4,
	ACT_REQ_WPS_CONNECT				= 5,
	ACT_REQ_WPS_DISABLE				= 6,
	ACT_REQ_SCAN					= 7,
	ACT_REQ_RSSI					= 8,
	ACT_REQ_SCAN_RESULT				= 9,
	ACT_REQ_WIFI_GROWL_INIT			= 10,
	ACT_REQ_START_PROWL_APP			= 11,
	ACT_REQ_START_NMA_APP			= 12,
	ACT_REQ_START_BOTH_APP			= 13,
	ACT_REQ_WIFI_GROWL_DEINIT		= 14,
	ACT_REQ_TCP_INIT				= 15,
	ACT_REQ_TCP_DEINIT				= 16,
	ACT_REQ_TCP_SEND				= 17,
	ACT_REQ_P2P_CONNECT				= 18,
	ACT_REQ_P2P_DISCONNECT			= 19,
	ACT_REQ_ENABLE_AP				= 20,
	ACT_REQ_DISABLE_AP				= 21,
	ACT_REQ_UDP_TEST				= 22,
	ACT_REQ_SERVER_INIT				= 23,
	ACT_REQ_CLIENT_LED_ON			= 24,
	ACT_REQ_CLIENT_LED_OFF			= 25,
	ACT_REQ_CLIENT_WAKE				= 26,
	ACT_REQ_FIRMWARE_INFO           = 27,
	ACT_REQ_MEM_DUMP				= 28,
	ACT_REQ_CONNECT_PROV			= 29,
	ACT_REQ_IPERF_START				= 30,
	ACT_REQ_IPERF_SEND				= 31,
	ACT_REQ_EXIT					= 0,
	ACT_REQ_NONE					= 127
} tenuActReq;

typedef enum{
	MODE_TCP_CLIENT,
	MODE_TCP_SERVER,
	MODE_UDP_CLIENT,
	MODE_UDP_SERVER
}tenuNMI_IperfMode;

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
GLOBAL VARIABLES
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

uint8 			gau8StackApp[1 * 1024];
tstrOsTask 		gstrTaskApp;
tstrOsSemaphore gstrAppSem;

static uint8 gbWifiConnected = 0;
static tenuActReq genuActReq = ACT_REQ_NONE;
uint8 	gbIperfTestActive = 0;
SOCKET	UdpTxSocket	= -1;
SOCKET	TcpRxSocket	= -1;
SOCKET	TcpTxSocket	= -1;
tenuNMI_IperfMode genumMode = MODE_UDP_SERVER;
/*********************************************************************
Function
wifi_cb

Description


Return
None.

Author


Version
1.0

Date
24 June 2013
*********************************************************************/
static void wifi_cb(uint8 u8MsgType, void * pvMsg)
{
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
		genuActReq = ACT_REQ_IPERF_START;
		APP_SEM_POST(&gstrAppSem);
			
	}
	else
	{
		M2M_ERR("req not defined %x\n", u8MsgType);
	}
}


/*======*======*======*======*
main APIs
*======*======*======*======*/
void App_ProcessActRequest(tenuActReq enuActReq)
{

	if (enuActReq == ACT_REQ_CONNECT)
	{
		M2M_REQ("ACT_REQ_CONNECT\n");
		m2m_wifi_connect(DEFAULT_SSID, sizeof(DEFAULT_SSID),
			DEFAULT_AUTH, DEFAULT_KEY, M2M_WIFI_CH_ALL);
		genuActReq = ACT_REQ_NONE;
	}
	else if(enuActReq == ACT_REQ_IPERF_START)
	{
		if(!gbIperfTestActive)
		{
			// On button press, start the Iperf client
			gbIperfTestActive = 1;
			if(genumMode == MODE_UDP_CLIENT)
			{
				UdpTxSocket = IperfUdpClientStart();
				genuActReq = ACT_REQ_IPERF_SEND;
				APP_SEM_POST(&gstrAppSem);
			}
			else if(genumMode == MODE_TCP_CLIENT)
			{
				TcpTxSocket = IperfTcpClientStart();
				genuActReq = ACT_REQ_IPERF_SEND;
				APP_SEM_POST(&gstrAppSem);
			}
			else if(genumMode == MODE_TCP_SERVER)
			{
				IperfTcpServerStart();
				genuActReq = ACT_REQ_NONE;
			}
			else if(genumMode == MODE_UDP_SERVER)
			{
				IperfUdpServerStart();
				genuActReq = ACT_REQ_NONE;
			}
		}
	}
	else if(enuActReq == ACT_REQ_IPERF_SEND)
	{
		if(gbIperfTestActive)
		{
			if(genumMode == MODE_UDP_CLIENT)
			{
				UDP_IperfEventHandler(UdpTxSocket, SOCKET_MSG_SENDTO,NULL);
			}
			if(genumMode == MODE_TCP_CLIENT && sendTCPpacket == 1)
			{
				SocketSendTestPacket(TcpTxSocket);
			}

			genuActReq = ACT_REQ_IPERF_SEND;
			APP_SEM_POST(&gstrAppSem);
		}else{
			genuActReq = ACT_REQ_NONE;
		}

	}
	else
	{
		M2M_INFO("GRP ? \n",genuActReq);
		genuActReq = ACT_REQ_NONE;
	}



#ifdef _STATIC_PS_
#error "Can't not be enabled in cortus app"
#endif
}
void app_main_context(void* pv)
{

	genuActReq = ACT_REQ_CONNECT;
	M2M_INFO("WIFI APP\n");
	APP_SEM_POST(&gstrAppSem);
	for (;;)
	{
		app_os_sem_down(&gstrAppSem);
		if (genuActReq != ACT_REQ_NONE)
		{
			M2M_DBG("genuActReq %d\n",genuActReq);
			App_ProcessActRequest(genuActReq);

		} else {
			m2m_wifi_handle_events(NULL);
		}

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
sint8 app_start(void)
{
	uint8 mac_addr[6];
	uint8 u8IsMacAddrValid;
	tstrWifiInitParam param;
	sint8 ret = M2M_SUCCESS;
	genuActReq = ACT_REQ_NONE;

	m2m_memset((uint8*) &gstrAppSem, 0, sizeof(gstrAppSem));

	M2M_PRINT("APP Start\n");

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
	//set_dev_name_to_mac((uint8*)gacDeviceName, mac_addr);
	//set_dev_name_to_mac((uint8*)gstrM2MAPConfig.au8SSID, mac_addr);
	//m2m_wifi_set_device_name((uint8*)gacDeviceName, (uint8)m2m_strlen((uint8*)gacDeviceName));

	m2m_wifi_set_power_profile(PWR_HIGH);
	m2m_wifi_enable_sntp(0);

#ifdef _DYNAMIC_PS_
	{
		tstrM2mLsnInt strM2mLsnInt;
		m2m_wifi_set_sleep_mode(M2M_PS_DEEP_AUTOMATIC, 1);
		strM2mLsnInt.u16LsnInt = M2M_LISTEN_INTERVAL;
		m2m_wifi_set_lsn_int(&strM2mLsnInt);
	}
#endif

	app_os_sem_init(&gstrAppSem, "APP", 0);
	app_os_sch_task_create(&gstrTaskApp, app_main_context, "APP", gau8StackApp,sizeof(gau8StackApp), 120);
ERR:
	return ret;
}
