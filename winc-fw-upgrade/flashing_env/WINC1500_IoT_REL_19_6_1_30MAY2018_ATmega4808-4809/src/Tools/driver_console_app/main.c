/**
*  @file		main.c				
*  @brief		This module contains M2M driver test application
*  @author		M. Abdelmawla
*  @date		10 JULY 2012
*  @version		1.0	
*/


/* Windows specific for threads */
#include "windows.h"
#include "conio.h"
#include "process.h"
#include "driver/include/m2m_ota.h"
#include "m2m_test_config.h"
#include "driver/source/nmbus.h"
#include "bus_wrapper/include/nm_bus_wrapper.h"
#include "driver/include/m2m_wifi.h"
#include "driver/include/m2m_periph.h"
#include "growl/include/growl.h"
#include "driver/source/m2m_hif.h"
#include "Tools/ethernet_mode/ethernet_mode.h"
#include "led_btn_demo/led_btn_demo.h"
#include "http_client/http_client.h"
#include "spi_flash/include/spi_flash.h"

void m2m_MemoryDump(void);
void http_test_task(uint8 u8ForceTestStart);
void http_client_test_init(void);
extern uint32 nmi_inet_addr(char *pcIpAddr);


static volatile uint8 gu8Connected = 0;
static volatile uint8 gu8Growl = 0;
static volatile uint8 u8NoCb = 0;
static volatile uint8 gu8Sleep = 0;
static volatile uint32 gu32Count =  0;
static volatile uint8 gu8ConnectList =  0;
static volatile uint8 index =  0;

#ifdef UDP_TEST
uint8 gbUdpTestActive = 0;
#endif

#ifdef AUTOMATED_TEST
static volatile uint8 gu8nTests = 0;
#endif

typedef enum
{
	ACT_REQ_CONNECT					= 1,
	ACT_REQ_CONNECT_LIST			= 2,
	ACT_REQ_DISCONNECT				= 3,
	ACT_REQ_START_PROVISIONING		= 4,
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
	ACT_REQ_ENABLE_MONITOR_MODE		= 29,		
	ACT_REQ_DISABLE_MONITOR_MODE	= 30,
	ACT_REQ_GPIO_TEST				= 31,
	ACT_REQ_OTA_ROLL				= 32,
	ACT_REQ_OTA_SWITCH              = 33,
	ACT_REQ_OTA_START_UPDATE		= 34,
	ACT_REQ_GET_CONN_INFO           = 35,
	ACT_REQ_STOP_PROV_MODE          = 36,
	ACT_REQ_MAC_ADDRESS				= 37,
	ACT_REQ_ETH_MODE_INIT			= 38,
	ACT_REQ_SEND_PING_REQUEST		= 39,
	ACT_REQ_HTTP_CLIENT_TEST		= 40,
	ACT_REQ_GET_RAND				= 41,
	ACT_REQ_EXIT					= 0,
	ACT_REQ_NONE					= 127
} tenuActReq;

static uint8 gu8TestBuf[TEST_BUS_BUF_SZ];
static uint8 gu8ShouldExit;
static tenuActReq genuActReq;
static uint8 gau8MacAddr[] = MAC_ADDRESS;
static sint8 gacDeviceName[] = M2M_DEVICE_NAME;
static uint8 gu8ApEnabled = 0;
static uint8 gu8P2pEnabled = 0;
static uint8 gu8Server = 0;
static tstrM2mWifiWepParams  gstrWepParams = WEP_CONN_PARAM;

static tstr1xAuthCredentials gstrCred1x    = AUTH_CREDENTIALS;

static tstrM2MAPConfig gstrM2MAPConfig = WINC_HOTSPOT_CONF;

static char gacHttpProvDomainName[] = HTTPS_PROV_SERVER_DN;

static tstrM2mAp gastrPreferredAPList[] = AP_LIST;

static uint32	gu32ConnectioCount = 0;

//#define __CONNECTION_TEST__


#ifdef __CONNECTION_TEST__
void CALLBACK TimerProc(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	gu32ConnectioCount ++;
	printf("TIMER %u\n",gu32ConnectioCount);
	genuActReq = ACT_REQ_CONNECT_LIST;
}
#endif

void StartConnectionTimer(void)
{
#ifdef __CONNECTION_TEST__
	UINT		u32Period = 2000,u32Resolution;
	TIMECAPS	tc;

	/* Set resolution to the minimum supported by the system */
	if(timeGetDevCaps(&tc, sizeof(TIMECAPS)) == TIMERR_NOERROR)
	{
		u32Resolution = min(max(tc.wPeriodMin, 0), tc.wPeriodMax);
		if(timeBeginPeriod(u32Resolution) == TIMERR_NOERROR)
		{
			timeSetEvent(u32Period, u32Resolution, TimerProc, 0, TIME_ONESHOT);
		}
	}
#endif
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
sint8 check_bus(void)
{
	uint16 i = 0;

	nm_bus_iface_init(&i);

	/* Check chip ID (read reg test)*/
	if((nm_read_reg(0x1000) & 0xfffff000) !=  0x100000)
	{
		M2M_ERR("invalid chip ID = 0x%08lx\n", nm_read_reg(0x1000));
		M2M_ERR("read reg test failed\n");
		return M2M_ERR_BUS_FAIL;
	}

	/* Test write reg */
	nm_write_reg(SHARED_PKT_MEM_BASE, 0x12345678);
	if(nm_read_reg(SHARED_PKT_MEM_BASE) != 0x12345678)
	{
		M2M_ERR("write reg test failed\n");
		return M2M_ERR_BUS_FAIL;
	}

	/* block read/write test */
	for(i = 0; i < TEST_BUS_BUF_SZ; i++)
	{
		gu8TestBuf[i] = (uint8)i;
	}
	nm_write_block(SHARED_PKT_MEM_BASE, gu8TestBuf, TEST_BUS_BUF_SZ);
	for(i = 0; i < TEST_BUS_BUF_SZ; i++)
	{
		gu8TestBuf[i] =0;
	}
	nm_read_block(SHARED_PKT_MEM_BASE, gu8TestBuf, TEST_BUS_BUF_SZ);
	for(i = 0; i < TEST_BUS_BUF_SZ; i++)
	{
		if(gu8TestBuf[i] != (uint8)i)
		{
			M2M_ERR("read/write block test failed at %u (expected, received) = (0x%02x, 0x%02x)\n", i, (uint8)i, gu8TestBuf[i]);
			return M2M_ERR_BUS_FAIL;
		}
	}

	M2M_DBG("Bus test passed successfully\n");

	return M2M_SUCCESS;
}
#ifdef LED_BTN_DEMO
static void led_btn_demo_app_cb(uint8 cmd)
{
	if(cmd == 'A')
	{
		M2M_INFO("LED 1 ON\n");
	}
	if(cmd == 'B')
	{
		M2M_INFO("LED 1 OFF\n");
	}
	if(cmd == 'C')
	{
		M2M_INFO("LED 2 ON\n");
	}
	if(cmd == 'D')
	{
		M2M_INFO("LED 2 OFF\n");
	}
	if(cmd == 'E')
	{
		M2M_INFO("LED 3 ON\n");
	}
	if(cmd == 'F')
	{
		M2M_INFO("LED 3 OFF\n");
	}
}
#endif

static void wifi_cb(uint8 u8MsgType,void * pvMsg)
{
	if(u8MsgType == M2M_WIFI_RESP_CON_STATE_CHANGED)
	{
		tstrM2mWifiStateChanged *pstrWifiState = (tstrM2mWifiStateChanged*)pvMsg;
		M2M_INFO("Wifi State :: %s :: ErrCode %d\n", pstrWifiState->u8CurrState? "CONNECTED":"DISCONNECTED",pstrWifiState->u8ErrCode);
		if(pstrWifiState->u8CurrState == M2M_WIFI_CONNECTED)
		{
#ifdef CONF_STATIC_IP_ADDRESS
			if((gu8ApEnabled == 0)&&(gu8P2pEnabled == 0))
			{
				tstrM2MIPConfig	strStaticConf;
				strStaticConf.u32StaticIP = nmi_inet_addr(STATIC_IP_ADDRESS);
				strStaticConf.u32DNS = nmi_inet_addr(DNS_ADDRESS);
				strStaticConf.u32Gateway = nmi_inet_addr(DEFAULT_GATEWAY_ADDRESS);
				strStaticConf.u32SubnetMask = nmi_inet_addr(SUBNET_MASK);
				m2m_wifi_set_static_ip(&strStaticConf);
				gu8Sleep = PS_REQ_SLEEP;
				gu8Connected = M2M_WIFI_CONNECTED;
				M2M_INFO("STATIC IP Address :: %s ::\n",STATIC_IP_ADDRESS);
				genuActReq = ACT_REQ_HTTP_CLIENT_TEST;
			}
#else
			gu8Sleep = PS_WAKE;
#endif
		}
		else if(pstrWifiState->u8CurrState == M2M_WIFI_DISCONNECTED)
		{
			gu8Connected = M2M_WIFI_DISCONNECTED;
			gu8Sleep = PS_WAKE;
		}
	}
	else if(u8MsgType == M2M_WIFI_REQ_DHCP_CONF)
	{
	    tstrM2MIPConfig* pstrM2MIpConfig = (tstrM2MIPConfig*) pvMsg;
		uint8 *pu8IPAddress = (uint8*) &pstrM2MIpConfig->u32StaticIP;
		uint8 *pu8DNS = (uint8*) &pstrM2MIpConfig->u32DNS;
		uint8 *pu8Gateway= (uint8*) &pstrM2MIpConfig->u32Gateway;
		uint8 *pu8SubnetMask = (uint8*) &pstrM2MIpConfig->u32SubnetMask;
		static uint8 u8Toggle = 0;

		if(gu8ApEnabled == 1)
		{
			
#ifdef LED_BTN_DEMO
			LedBtnDemo_AppStart(led_btn_demo_app_cb);
#endif
		}

		M2M_INFO("IP Address  \"%u.%u.%u.%u\"\n",pu8IPAddress[0],pu8IPAddress[1],pu8IPAddress[2],pu8IPAddress[3]);
		M2M_INFO("DNS         \"%u.%u.%u.%u\"\n",pu8DNS[0],pu8DNS[1],pu8DNS[2],pu8DNS[3]);
		M2M_INFO("GateWay     \"%u.%u.%u.%u\"\n",pu8Gateway[0],pu8Gateway[1],pu8Gateway[2],pu8Gateway[3]);
		M2M_INFO("Subnet Mask \"%u.%u.%u.%u\"\n",pu8SubnetMask[0],pu8SubnetMask[1],pu8SubnetMask[2],pu8SubnetMask[3]);
		M2M_INFO("Lease time  \"%x\"\n",pstrM2MIpConfig->u32DhcpLeaseTime);
		gu8Sleep = PS_REQ_SLEEP;
		gu8Connected = M2M_WIFI_CONNECTED;
		StartConnectionTimer();
	}
	else if(u8MsgType == M2M_WIFI_RESP_DEFAULT_CONNECT)
	{
		tstrM2MDefaultConnResp	*pstrResp = (tstrM2MDefaultConnResp*)pvMsg;
		M2M_INFO("M2M_WIFI_RESP_DEFAULT_CONNECT %d\n",pstrResp->s8ErrorCode);
		if(pstrResp->s8ErrorCode == M2M_DEFAULT_CONN_EMPTY_LIST)
		{
			m2m_wifi_start_provision_mode((tstrM2MAPConfig*)&gstrM2MAPConfig, (char*)gacHttpProvDomainName, HTTP_REDIRECT_FLAG);
		}
		else if(pstrResp->s8ErrorCode == M2M_DEFAULT_CONN_SCAN_MISMATCH)
		{
			// TODO: Add Code here
			m2m_wifi_request_scan(M2M_WIFI_CH_ALL);
		}
	}
	else if (u8MsgType == M2M_WIFI_RESP_GET_SYS_TIME)
	{
		tstrSystemTime *pstrTime = (tstrSystemTime*)pvMsg;
		M2M_INFO("Time Of Day\n\t%d/%02d/%d %02d:%02d:%02d GMT\n",
		pstrTime->u8Month, pstrTime->u8Day, pstrTime->u16Year,
		pstrTime->u8Hour, pstrTime->u8Minute, pstrTime->u8Second);

	}
	else if(u8MsgType == M2M_WIFI_RESP_PROVISION_INFO)
	{
		tstrM2MProvisionInfo	*pstrProvInfo = (tstrM2MProvisionInfo*)pvMsg;
		if(pstrProvInfo->u8Status == M2M_SUCCESS)
		{
			m2m_wifi_connect((char*)pstrProvInfo->au8SSID, (uint8)strlen(pstrProvInfo->au8SSID), pstrProvInfo->u8SecType, 
					pstrProvInfo->au8Password, M2M_WIFI_CH_ALL);

			M2M_INFO("PROV SSID : %s\n",pstrProvInfo->au8SSID);
			M2M_INFO("PROV PSK  : %s\n",pstrProvInfo->au8Password);
		}
		else
		{
			// TODO: Assign LEDs foe Error or Success
			M2M_ERR("Provisioning Failed\n");
		}
	}
	else if(u8MsgType == M2M_WIFI_RESP_CONN_INFO)
	{
		tstrM2MConnInfo		*pstrConnInfo = (tstrM2MConnInfo*)pvMsg;
		M2M_INFO("*************************************\n");
		M2M_INFO("           NETWORK INFO              \n");
		M2M_INFO("*************************************\n");
		M2M_INFO("SSID    : %s\n",pstrConnInfo->acSSID);
		M2M_INFO("RSSI    : %d\n",pstrConnInfo->s8RSSI);
		M2M_INFO("SEC     : %d\n",pstrConnInfo->u8SecType);
		M2M_INFO("IP ADDR : %u.%u.%u.%u\n",pstrConnInfo->au8IPAddr[0],pstrConnInfo->au8IPAddr[1],
			pstrConnInfo->au8IPAddr[2],pstrConnInfo->au8IPAddr[3]);
	}
	else if(u8MsgType == M2M_WIFI_REQ_WPS)
	{
		tstrM2MWPSInfo	*pstrWPS = (tstrM2MWPSInfo*)pvMsg;
		gu8Sleep = PS_REQ_SLEEP;
		if(pstrWPS->u8AuthType != 0)
		{
			M2M_INFO("WPS SSID           : %s\n",pstrWPS->au8SSID);
			M2M_INFO("WPS PSK            : %s\n",pstrWPS->au8PSK);
			M2M_INFO("WPS SSID Auth Type : %s\n",pstrWPS->u8AuthType == M2M_WIFI_SEC_OPEN ? "OPEN" : "WPA/WPA2");
			M2M_INFO("WPS Channel        : %d\n",pstrWPS->u8Ch);
			m2m_wifi_connect((char*)pstrWPS->au8SSID, (uint8)m2m_strlen(pstrWPS->au8SSID),
				pstrWPS->u8AuthType, pstrWPS->au8PSK, pstrWPS->u8Ch);
		}
		else
		{
			M2M_ERR("WPS Is not enabled OR Timedout\n");
			genuActReq = ACT_REQ_CONNECT_LIST;
		}
	}
	else if(u8MsgType == M2M_WIFI_RESP_IP_CONFLICT)
	{
		gu8Sleep = PS_WAKE;
	}
	else if(u8MsgType == M2M_WIFI_RESP_GET_PRNG)
	{
		int i = 0;
		tstrPrng * pstrPrng = (tstrPrng*)pvMsg;
		M2M_INFO("Dump %d %x\n",pstrPrng->u16PrngSize,(uint32)pstrPrng->pu8RngBuff);
		for (i = 0; i<pstrPrng->u16PrngSize;i++)
		{
			if(!(i%16))printf("\n");
			printf("%02x ",pstrPrng->pu8RngBuff[i]);
		}
	}
	else if(u8MsgType == M2M_WIFI_RESP_SCAN_DONE)
	{
		tstrM2mScanDone	*pstrInfo = (tstrM2mScanDone*)pvMsg;
		gu8Sleep = PS_REQ_SLEEP;
		M2M_INFO("Num of AP found %d\n",pstrInfo->u8NumofCh);
		if(pstrInfo->s8ScanState == M2M_SUCCESS)
		{
			if(gu8ConnectList)
			{	
				index = 0;
				gu8Sleep = PS_WAKE;
				if(pstrInfo->u8NumofCh>=1)
				{
					m2m_wifi_req_scan_result(index);
					index++;
				}
				else
				{
					M2M_INFO("No AP Found Rescan\n");
					m2m_wifi_request_scan(M2M_WIFI_CH_ALL);
				}
			}
		}
		else
		{
			M2M_ERR("Scan fail %d\n",pstrInfo->s8ScanState);
		}
	}
	else if(u8MsgType == M2M_WIFI_RESP_CURRENT_RSSI)
	{
		sint8	*rssi = (sint8*)pvMsg;
		gu8Sleep = PS_REQ_SLEEP;
		M2M_INFO("ch rssi %d\n",*rssi);
	}
	else if(u8MsgType == M2M_WIFI_RESP_CLIENT_INFO)
	{
		uint8	*pstrInfo = (uint8*)pvMsg;
		gu8Sleep = PS_WAKE;
		M2M_INFO("Client state %d\n",*pstrInfo);
		if(*pstrInfo == M2M_CLIENT_RESP_MOVEMENT)
		{
			M2M_INFO("Client Movement Detection\n");
			if(gu8Connected == M2M_WIFI_CONNECTED)
			{
#ifdef GROWL
				NMI_GrowlSendNotification(PROWL_CLIENT,(uint8*)"NMIAppSRV",(uint8*)"TI App",(uint8*)"Movement Detection",PROWL_CONNECTION_TYPE);
				NMI_GrowlSendNotification(NMA_CLIENT,(uint8*)"NMIAppSRV",(uint8*)"TI App",(uint8*)"Movement Detection",NMA_CONNECTION_TYPE);
#endif
			}
		}
		else if(*pstrInfo == M2M_CLIENT_RESP_BTN1_PRESS)
		{
			M2M_INFO("Client Btn 1 Press\n");
			if(gu8Connected == M2M_WIFI_CONNECTED)
			{
#ifdef GROWL
				NMI_GrowlSendNotification(PROWL_CLIENT,(uint8*)"NMIAppSRV",(uint8*)"TI App",(uint8*)"Btn 1 Press",PROWL_CONNECTION_TYPE);
				NMI_GrowlSendNotification(NMA_CLIENT,(uint8*)"NMIAppSRV",(uint8*)"TI App",(uint8*)"Btn 1 Press",NMA_CONNECTION_TYPE);
#endif
			}
		}
		else if(*pstrInfo == M2M_CLIENT_RESP_BTN2_PRESS)
		{
			M2M_INFO("Client Btn 2 Press\n");
			if(gu8Connected == M2M_WIFI_CONNECTED)
			{
#ifdef GROWL
				NMI_GrowlSendNotification(PROWL_CLIENT,(uint8*)"NMIAppSRV",(uint8*)"TI App",(uint8*)"Btn 2 Press",PROWL_CONNECTION_TYPE);
				NMI_GrowlSendNotification(NMA_CLIENT,(uint8*)"NMIAppSRV",(uint8*)"TI App",(uint8*)"Btn 2 Press",NMA_CONNECTION_TYPE);
#endif
			}
		}
	}
	else if(u8MsgType == M2M_WIFI_RESP_SCAN_RESULT)
	{
		tstrM2mWifiscanResult		*pstrScanResult =(tstrM2mWifiscanResult*)pvMsg;
		uint8						u8NumFoundAPs = m2m_wifi_get_num_ap_found();
		M2M_INFO(">>%02d RI %d SEC %s CH %02d BSSID %02X:%02X:%02X:%02X:%02X:%02X SSID %s\n",
			pstrScanResult->u8index,pstrScanResult->s8rssi,
			M2M_SEL(pstrScanResult->u8AuthType,"OPEN","WPA ","WEP "),pstrScanResult->u8ch,
			pstrScanResult->au8BSSID[0], pstrScanResult->au8BSSID[1], pstrScanResult->au8BSSID[2],
			pstrScanResult->au8BSSID[3], pstrScanResult->au8BSSID[4], pstrScanResult->au8BSSID[5],
			pstrScanResult->au8SSID);
		if(gu8ConnectList)
		{
			uint8	j = 0;
			uint16	u16SSidLen;
#ifdef LIST_CONNECT
			for(j = 0; j < M2M_AP_LIST_SZ; j++)
			{
				u16SSidLen = m2m_strlen((uint8*)gastrPreferredAPList[j].au8Ssid);
				if
				(
					(u16SSidLen == m2m_strlen(pstrScanResult->au8SSID)) &&
					(!m2m_memcmp(pstrScanResult->au8SSID, (uint8*)gastrPreferredAPList[j].au8Ssid,u16SSidLen))
				)
				{
					/* A scan result matches an entry in the preferred AP List.
					Initiate a connection request.
					*/
					M2M_INFO("match\n");
					index			= 0;
					gu8ConnectList	= 0;

					m2m_wifi_connect((char*)pstrScanResult->au8SSID,
						(uint8)m2m_strlen((uint8*)pstrScanResult->au8SSID),
						gastrPreferredAPList[j].u8AuthType,
						(void*)gastrPreferredAPList[j].pu8AuthCred,
						(pstrScanResult->u8ch));
					return;
				}
			}
#endif
		}
		if(index < u8NumFoundAPs)
		{
			gu8Sleep = PS_WAKE;
			m2m_wifi_req_scan_result(index);
			index++;
		}
		else
		{
			if(gu8ConnectList)
			{
				gu8Sleep = PS_WAKE;
				M2M_INFO("No AP Match Rescan\n");
				m2m_wifi_request_scan(M2M_WIFI_CH_ALL);

			}
			else
			{
				gu8Sleep = PS_REQ_SLEEP;
			}
			index = 0;
		}

	}
}


void m2m_gpio_test(void)
{
#if 0
	int i;
	m2m_periph_gpio_set_dir(M2M_PERIPH_GPIO16, 1);
	m2m_periph_gpio_set_dir(M2M_PERIPH_GPIO18, 1);
	for(i=0; i<5; i++) {
		m2m_periph_gpio_set_val(M2M_PERIPH_GPIO16, 1);
		m2m_periph_gpio_set_val(M2M_PERIPH_GPIO18, 0);
		Sleep(1000);
		m2m_periph_gpio_set_val(M2M_PERIPH_GPIO16, 0);
		m2m_periph_gpio_set_val(M2M_PERIPH_GPIO18, 1);
		Sleep(1000);
	}
#endif
}

void GrowlCb(uint8 u8Code, uint8 u8ClientID)
{
	M2M_INFO("%s Return Code :: %u ::\n",(u8ClientID == PROWL_CLIENT) ? "PROWL" : "NMA",u8Code);
#ifdef AUTOMATED_TEST
	if(gu8nTests != 0)
		gu8nTests --;
	if(gu8nTests != 0)
	{
		M2M_INFO("Count %d\n", gu8nTests);
		Sleep(50);
		gu8Sleep = PS_WAKE;
		if(u8ClientID == PROWL_CLIENT)
			NMI_GrowlSendNotification(PROWL_CLIENT,(uint8*)"NMIApp",(uint8*)"NMI_Event",(uint8*)"Win PC",PROWL_CONNECTION_TYPE);
		else
			NMI_GrowlSendNotification(NMA_CLIENT,(uint8*)"NMIApp",(uint8*)"NMI_Event",(uint8*)"Win PC",NMA_CONNECTION_TYPE);		
	}
	else
	{
		gu8Growl = 1;
		gu8Sleep = PS_REQ_SLEEP;
	}
#else
	u8NoCb --;
	if(u8NoCb == 0)
	{
		u8NoCb = 0;
		gu8Growl = 1;
		gu8Sleep = PS_REQ_SLEEP;
	}
#endif
}

void Timer(void* p)
{
	while(1)
	{
		if(gu8Sleep == PS_SLEEP)
		{
			gu32Count++;
			if(gu32Count > (PS_SLEEP_TIME_MS/10))
			{
				gu32Count = 0;
				gu8Sleep = PS_REQ_CHECK;
			}
		}
		else
		{
			gu32Count = 0;
		}
		nm_bsp_sleep(10);
		if(genuActReq == ACT_REQ_EXIT)
		{
			goto EXIT;
		}
	}
EXIT:
	_endthread();
}

/*!<OTA update callback typedef> */
 void OtaUpdateCb(uint8 u8OtaUpdateStatusType ,uint8 u8OtaUpdateStatus)
 {
	sint8 ret;
	tstrM2mRev strtmp;
	M2M_INFO("%d %d\n",u8OtaUpdateStatusType,u8OtaUpdateStatus);
	ret = m2m_ota_get_firmware_version(&strtmp);

	if((ret == M2M_SUCCESS)||(M2M_ERR_FW_VER_MISMATCH == ret))
	{
		M2M_INFO("Firmware ver   : %u.%u.%u Svnrev %u\n", strtmp.u8FirmwareMajor, strtmp.u8FirmwareMinor, strtmp.u8FirmwarePatch,strtmp.u16FirmwareSvnNum);
		M2M_INFO("Min driver ver : %u.%u.%u\n", strtmp.u8DriverMajor, strtmp.u8DriverMinor, strtmp.u8DriverPatch);
		M2M_INFO("Firmware Build %s Time %s\n",strtmp.BuildDate,strtmp.BuildTime);
		if(M2M_ERR_FW_VER_MISMATCH == ret)
		{
			M2M_ERR("Mismatch Firmawre Version\n");
		}
	}
 }

/*!<OTA notify callback typedef> */
 void OtaNotifCb(tstrOtaUpdateInfo *pv)
 {
	 M2M_INFO("\n");
		 
 }

/* This will be like the main in the MCU */
void execution_thread(void* p)
{
	uint8				bIsHttpClientRunning = 0;				
	tstrWifiInitParam	param;
	sint8				ret = M2M_SUCCESS;
	uint8				mac_addr[6];
	uint8				u8IsMacAddrValid;
	tstrM2mRev strtmp;
	
	gu8Growl = 1;
	gu8Connected = M2M_WIFI_DISCONNECTED;
	nm_bsp_init();
	gu8Sleep = PS_REQ_SLEEP;

	m2m_memset((uint8*)&param, 0, sizeof(param));
	param.pfAppWifiCb = wifi_cb;	
#ifdef ETH_MODE
	param.strEthInitParam.pfAppWifiCb = wifi_cb;
	param.strEthInitParam.pfAppEthCb = ethernet_demo_cb; 
	param.strEthInitParam.au8ethRcvBuf = gau8ethRcvBuf;
	param.strEthInitParam.u16ethRcvBufSize = sizeof(gau8ethRcvBuf);
#endif

	ret = m2m_wifi_init(&param);
	if (M2M_SUCCESS != ret) {
		M2M_ERR("failed to initialize Driver, Error no = %d\n", ret);
		return;
	}
//	m2m_ssl_init(NULL);
//	eccInit();
//	test();
#ifndef BUS_ONLY

#if 0
	/*change only for lab board */
	{
		tstrM2mWifiGainsParams strGains;
		strGains.u8PPAGFor11B = 0x1; /*FOR winc make it 0x3*/
		strGains.u8PPAGFor11GN = 0x3;
		m2m_wifi_set_gains(&strGains);
	}
#endif

	ret = m2m_ota_get_firmware_version(&strtmp);

	if((ret == M2M_SUCCESS)||(ret == M2M_ERR_FW_VER_MISMATCH))
	{
		M2M_INFO("OTA Firmware ver   : %u.%u.%u Svnrev %u\n", strtmp.u8FirmwareMajor, strtmp.u8FirmwareMinor, strtmp.u8FirmwarePatch,strtmp.u16FirmwareSvnNum);
		M2M_INFO("OTA Min driver ver : %u.%u.%u\n", strtmp.u8DriverMajor, strtmp.u8DriverMinor, strtmp.u8DriverPatch);
		M2M_INFO("OTA Firmware Build %s Time %s\n",strtmp.BuildDate,strtmp.BuildTime);
	}


#if 0
	{
		SYSTEMTIME		windowsTime;
		tstrSystemTime	strSysTime;

		GetSystemTime(&windowsTime);
		strSysTime.u16Year	= windowsTime.wYear;
		strSysTime.u8Month	= windowsTime.wMonth;
		strSysTime.u8Day	= windowsTime.wDay;
		strSysTime.u8Hour	= windowsTime.wHour;
		strSysTime.u8Minute	= windowsTime.wMinute;
		strSysTime.u8Second	= windowsTime.wMilliseconds;
		m2m_wifi_enable_sntp(0);
		//m2m_wifi_set_sytem_time();
	}
#endif
#if 0
	{
		tstrM2MScanOption strScanOption;
		strScanOption.s8RssiThresh = -45;
		strScanOption.u8NumOfSlot = 2;
		strScanOption.u8SlotTime = 30;
		strScanOption.u8ProbesPerSlot = 2;
		m2m_wifi_set_scan_options(&strScanOption);
	}
#endif
	//m2m_wifi_enable_dhcp(0);
	m2m_wifi_get_otp_mac_address(mac_addr, &u8IsMacAddrValid);
	if(!u8IsMacAddrValid) {
		M2M_INFO("Default MAC\n");
		m2m_wifi_set_mac_address(gau8MacAddr);
	} else {
		M2M_INFO("OTP MAC\n");
	}
	m2m_wifi_get_mac_address(mac_addr);
	M2M_INFO("MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
		mac_addr[0],mac_addr[1],mac_addr[2],
		mac_addr[3],mac_addr[4],mac_addr[5]);

	set_dev_name_to_mac((uint8*)gacDeviceName, gau8MacAddr);
	//set_dev_name_to_mac((uint8*)gstrM2MAPConfig.au8SSID, gau8MacAddr);
	m2m_wifi_set_device_name((uint8*)gacDeviceName, (uint8)m2m_strlen((uint8*)gacDeviceName));



#ifdef _DYNAMIC_PS_
	
	m2m_wifi_set_sleep_mode(M2M_PS_DEEP_AUTOMATIC, 1);
	
#elif (defined _STATIC_PS_)
	m2m_wifi_set_sleep_mode(M2M_PS_MANUAL, 1);
#else
	m2m_wifi_set_sleep_mode(M2M_NO_PS, 1);
#endif
#endif

	m2m_ota_init(OtaUpdateCb, OtaNotifCb);

	M2M_INFO("Wifi State :: %s ::\n","DISCONNECTED");
//	m2m_wifi_set_scan_region(REG_CH_6|REG_CH_11);

	//m2m_wifi_set_battery_voltage(200);
	//m2m_wifi_set_power_profile(PWR_DEFAULT);
	//m2m_wifi_set_tx_power(TX_PWR_MED);

	for(;;)
	{
		switch(genuActReq)
		{
		case ACT_REQ_GET_CONN_INFO:
			genuActReq = ACT_REQ_NONE;
			m2m_wifi_get_connection_info();
			break;

		case ACT_REQ_STOP_PROV_MODE:
			m2m_wifi_stop_provision_mode();
			genuActReq = ACT_REQ_NONE;
			break;

		case ACT_REQ_OTA_START_UPDATE:
			m2m_ota_start_update(OTA_URL);
			genuActReq = ACT_REQ_NONE;
			break;

		case ACT_REQ_OTA_ROLL:
			m2m_ota_rollback();
			genuActReq = ACT_REQ_NONE;
			break;

		case ACT_REQ_OTA_SWITCH:
			m2m_ota_switch_firmware();
			genuActReq = ACT_REQ_NONE;
			break;

		case ACT_REQ_CONNECT:
			gu8Sleep = PS_WAKE;
			m2m_wifi_default_connect();
			//m2m_wifi_connect(DEFAULT_SSID, sizeof(DEFAULT_SSID),
				//DEFAULT_AUTH, DEFAULT_KEY, M2M_WIFI_CH_ALL);

			genuActReq = ACT_REQ_NONE;
			break;

		case ACT_REQ_START_PROVISIONING:
			{
				m2m_wifi_start_provision_mode(&gstrM2MAPConfig, gacHttpProvDomainName, HTTP_REDIRECT_FLAG);
				genuActReq = ACT_REQ_NONE;
			}
			break;

		case ACT_REQ_WPS_CONNECT:
			//m2m_ota_rollback();
			//m2m_wifi_wps(WPS_PIN_TRIGGER,WPS_PIN_NUMBER);
			m2m_wifi_wps(WPS_PBC_TRIGGER,NULL);

			gu8Sleep = PS_WAKE;

			genuActReq = ACT_REQ_NONE;
			break;

		case ACT_REQ_WPS_DISABLE:
			m2m_wifi_wps_disable();
			gu8Sleep = PS_WAKE;
			genuActReq = ACT_REQ_NONE;
			break;

		case ACT_REQ_START_PROWL_APP:
		case ACT_REQ_START_NMA_APP:
		case ACT_REQ_START_BOTH_APP:

#ifdef GROWL
			gu8Growl = 0;
			u8NoCb = 1;
			gu8Sleep = PS_WAKE;
#ifdef AUTOMATED_TEST
			if(gu8nTests == 0)
			{
				gu8nTests = GROWL_N_TESTS;
			}
#endif
			if(genuActReq == ACT_REQ_START_BOTH_APP)
			{
				u8NoCb = 2;
				NMI_GrowlSendNotification(PROWL_CLIENT,(uint8*)"NMIApp",(uint8*)"NMI_Event",(uint8*)"Win PC",PROWL_CONNECTION_TYPE);
				NMI_GrowlSendNotification(NMA_CLIENT,(uint8*)"NMIApp",(uint8*)"NMI_Event",(uint8*)"Win PC",NMA_CONNECTION_TYPE);
			}
			else
			{
				uint8	u8ClientID = PROWL_CLIENT;
				uint8	u8UseSSL = PROWL_CONNECTION_TYPE;
				if(genuActReq == ACT_REQ_START_NMA_APP)
				{
					u8ClientID	= NMA_CLIENT;
					u8UseSSL	= NMA_CONNECTION_TYPE;
				}
				NMI_GrowlSendNotification(u8ClientID,(uint8*)"NMIApp",(uint8*)"NMI_Event",(uint8*)"Win PC",u8UseSSL);
			}
#else
			M2M_ERR("GROWL Not Defined\n");
#endif
			genuActReq = ACT_REQ_NONE;
			break;

		case ACT_REQ_SCAN:
			{
#if 0
#define SSID1 "DEMO_AP"
#define SSID2 "12345678901234567890123456789012"
#define SSID3 "DEMO_AP1"
#define SSID4 "TEST_AP"

				int i = 0;
				uint8 SSID[150];
				gu8Sleep = PS_WAKE;
				SSID[i++] = 4;
				SSID[i++] = strlen(SSID1);
				memcpy(&SSID[i],SSID1,strlen(SSID1));
				i+= strlen(SSID1);
				SSID[i++] = strlen(SSID2);
				memcpy(&SSID[i],SSID2,strlen(SSID2));
				i+= strlen(SSID2);
				SSID[i++] = strlen(SSID3);
				memcpy(&SSID[i],SSID3,strlen(SSID3));
				i+= strlen(SSID3);
				SSID[i++] = strlen(SSID4);
				memcpy(&SSID[i],SSID4,strlen(SSID4));
				i+= strlen(SSID4);

				m2m_wifi_request_scan_ssid_list(M2M_WIFI_CH_ALL,SSID);
#else
				m2m_wifi_request_scan(M2M_WIFI_CH_ALL);
#endif
				genuActReq = ACT_REQ_NONE;
			}
			break;

		case ACT_REQ_RSSI:
			gu8Sleep = PS_WAKE;
			m2m_wifi_req_curr_rssi();
			genuActReq = ACT_REQ_NONE;
			break;
		case ACT_REQ_ETH_MODE_INIT:
			gu8Sleep = PS_WAKE;
#ifdef ETH_MODE
			ethernet_demo_init();
#endif
			genuActReq = ACT_REQ_NONE;
			break;
			
		case ACT_REQ_SEND_PING_REQUEST:
#ifdef ETH_MODE
			if(*gau8RemoteMacAddr)
				send_ping_req(PING_PACKETS_COUNT);
			else
				send_arp_req();
#else
			M2M_ERR(" ETH_MODE Not Defined!\n");
#endif
			genuActReq = ACT_REQ_NONE;
			gu8Sleep = PS_WAKE;
			break;		
		case ACT_REQ_MAC_ADDRESS:
			gu8Sleep = PS_WAKE;
			{
				m2m_wifi_get_mac_address(mac_addr);
				printf("MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
					mac_addr[0]&0x000000ff,mac_addr[1]&0x000000ff,mac_addr[2]&0x000000ff,
					mac_addr[3]&0x000000ff,mac_addr[4]&0x000000ff,mac_addr[5]&0x000000ff);
			}
			genuActReq = ACT_REQ_NONE;
			break;

		case ACT_REQ_CLIENT_WAKE:
			gu8Sleep = PS_WAKE;
			m2m_wifi_req_client_ctrl(M2M_CLIENT_CMD_WAKE_FIRMWARE);
			genuActReq = ACT_REQ_NONE;
			break;

		case ACT_REQ_CLIENT_LED_ON:
			gu8Sleep = PS_WAKE;
			m2m_wifi_req_client_ctrl(M2M_CLIENT_CMD_LED_ON);
			genuActReq = ACT_REQ_NONE;
			break;

		case ACT_REQ_CLIENT_LED_OFF:
			gu8Sleep = PS_WAKE;
			m2m_wifi_req_client_ctrl(M2M_CLIENT_CMD_LED_OFF);
			genuActReq = ACT_REQ_NONE;
			break;

		case ACT_REQ_SERVER_INIT:
			gu8Sleep = PS_WAKE;
			gu8Server = 1;
			m2m_wifi_req_server_init(M2M_SERVER_CHANNEL);
			genuActReq = ACT_REQ_NONE;
			break;

		case ACT_REQ_DISCONNECT:
			gu8Sleep = PS_WAKE;
			m2m_wifi_disconnect();
			genuActReq = ACT_REQ_NONE;
			break;

		case ACT_REQ_SCAN_RESULT:
			{
				uint8 chnum = m2m_wifi_get_num_ap_found();
				gu8Sleep = PS_WAKE;
				index = 0;
				if(chnum >= 1)
				{
					m2m_wifi_req_scan_result(index);
					index++;
				}
				else
				{
					M2M_INFO("No AP Found\n");
				}
				genuActReq = ACT_REQ_NONE;
			}
			break;

		case ACT_REQ_CONNECT_LIST:
			gu8ConnectList = 1;
			gu8Sleep = PS_WAKE;
			m2m_wifi_request_scan(M2M_WIFI_CH_ALL);
			genuActReq = ACT_REQ_NONE;
			break;

		case ACT_REQ_UDP_TEST:
#ifdef UDP_TEST
			gu8Sleep = PS_WAKE;
			gu8Growl = 0;
			SocketTest();
#else

			M2M_ERR(" UDP_TEST Not defined\n\r");
#endif
			genuActReq = ACT_REQ_NONE;
			break;
#if 0
		case ACT_REQ_FIRMWARE_INFO:
			{
				tstrM2mRev strtmp;
				if(M2M_ERR_FW_VER_MISMATCH != nm_get_firmware_info(&strtmp))
				{
					M2M_INFO("FIRMWARE_INFO: \nREV_MAJOR = %d, REV_MINOR= %d, HW_REV= %X\n",strtmp.u16Major, strtmp.u16Minor, strtmp.u32Chipid);
					genuActReq = ACT_REQ_NONE;
				}
				else
				{
					M2M_ERR("Error while reading reg\n");
				}
			}
			break;
#endif
		case ACT_REQ_P2P_CONNECT:
			m2m_wifi_p2p(M2M_WIFI_CH_1);
			gu8P2pEnabled = 1;
			gu8Sleep = PS_WAKE;
			genuActReq = ACT_REQ_NONE;
			break;

		case ACT_REQ_TCP_INIT:
			gu8Sleep = PS_WAKE;
			gu8Growl = 0;
#ifdef LED_BTN_DEMO
			LedBtnDemo_AppStart(led_btn_demo_app_cb);	
#endif
			genuActReq = ACT_REQ_NONE;
			break;

		case ACT_REQ_TCP_DEINIT:
			gu8Sleep = PS_WAKE;
			gu8Growl = 0;
#ifdef LED_BTN_DEMO
			LedBtnDemo_AppStop();	
#endif
			genuActReq = ACT_REQ_NONE;
			break;

		case ACT_REQ_WIFI_GROWL_INIT:
			gu8Sleep = PS_WAKE;
			gu8Growl = 0;
#ifdef GROWL
			NMI_GrowlInit((uint8*)PROWL_API_KEY,(uint8*)NMA_API_KEY);
#endif
			genuActReq = ACT_REQ_NONE;
			break;

		case ACT_REQ_WIFI_GROWL_DEINIT:
			gu8Sleep = PS_WAKE;
			gu8Growl = 0;
#ifdef GROWL
			NMI_GrowldeInit();
#endif
			genuActReq = ACT_REQ_NONE;
			break;

		case ACT_REQ_P2P_DISCONNECT:
			m2m_wifi_p2p_disconnect();
			gu8P2pEnabled = 0;
			gu8Sleep = PS_WAKE;
			genuActReq = ACT_REQ_NONE;
			break;

		case ACT_REQ_TCP_SEND:
#ifdef LED_BTN_DEMO
			gu8Sleep = PS_WAKE;
			LedBtnDemo_AppSendNotification("Movement Detected");
			genuActReq = ACT_REQ_NONE;
#endif
			break;

		case ACT_REQ_ENABLE_AP:
			{
//#define TEST_IEs
#ifdef TEST_IEs
				char elementData[21];
				static char state = 0;
				/*Each frame must consist of: "Number of total bytes+IE1(ID,Length,Data)+...+IEn(ID,Length,Data)"
				/*Total Number of Bytes*/
				if(0 == state) {	//Add 3 IEs
					state = 1;
					elementData[0]=12;
					/*First IE*/
					elementData[1]=200; elementData[2]=1; elementData[3]='A';
					/*Second IE*/
					elementData[4]=201; elementData[5]=2; elementData[6]='B'; elementData[7]='C';
					/*Third IE*/
					elementData[8]=202; elementData[9]=3; elementData[10]='D'; elementData[11]=0; elementData[12]='F';
				} else if(1 == state) {	//Append 2 IEs to others
					state = 2; 
					elementData[0]=20;
					/*Fourth IE*/
					elementData[13]=203; elementData[14]=1; elementData[15]='G';
					/*Fifth IE*/
					elementData[16]=204; elementData[17]=3; elementData[18]='X'; elementData[19]=5; elementData[20]='Z';
				} else if(2 == state) {	//Delete All IEs
					state = 0; 
					elementData[0]=0;
				}
				m2m_wifi_set_cust_InfoElement(elementData);

				if(gu8ApEnabled == 0) 
#endif			
				{	
				m2m_wifi_enable_ap(&gstrM2MAPConfig);
				}
				gu8Sleep = PS_WAKE;
				genuActReq = ACT_REQ_NONE;
				gu8ApEnabled = 1;
			}
			break;

		case ACT_REQ_DISABLE_AP:
			if(gu8ApEnabled == 1)
			{
				gu8ApEnabled = 0;
				m2m_wifi_disable_ap();
				gu8Sleep = PS_WAKE;
				genuActReq = ACT_REQ_NONE;
			}
			else
			{
				printf("AP is not enabled!\n");
				genuActReq = ACT_REQ_NONE;
			}
			break;

		case ACT_REQ_ENABLE_MONITOR_MODE:
			{
				genuActReq	= ACT_REQ_NONE;
			}
			break;

		case ACT_REQ_DISABLE_MONITOR_MODE:
			genuActReq	= ACT_REQ_NONE;
			break;

		case ACT_REQ_MEM_DUMP:
			gu8Sleep = PS_WAKE;
			hif_send(M2M_REQ_GROUP_IP, 0x77, OTA_URL, (uint16)strlen(OTA_URL) + 1, NULL, 0, 0);
			
			//m2m_MemoryDump();
			//m2m_ota_start_update("dgfdgdf\n");
			//m2m_ota_test();
			genuActReq = ACT_REQ_NONE;
			break;

		case ACT_REQ_HTTP_CLIENT_TEST:
			{
				static uint8	bIsInit = 0;

				gu8Sleep = PS_WAKE;
				if(!bIsInit)
				{
					http_client_test_init();
					bIsInit = 1;
				}
				bIsHttpClientRunning = 1;
				http_test_task(1);
				genuActReq = ACT_REQ_NONE;
			}
			break;

		case ACT_REQ_GPIO_TEST:
			genuActReq = ACT_REQ_NONE;
			m2m_gpio_test();
			break;
		case ACT_REQ_GET_RAND:
			{
				uint8 buff[200];
				m2m_wifi_prng_get_random_bytes(buff,200);
				genuActReq = ACT_REQ_NONE;
			}
			break;

		case ACT_REQ_EXIT:
			gu8Sleep = PS_WAKE;
			m2m_wifi_deinit(NULL);
			genuActReq = ACT_REQ_NONE;
			goto EXIT;
		}
#ifndef BUS_ONLY
		if(gu8Sleep != PS_SLEEP)
		{
			nm_bsp_poll();
			hif_chip_wake();
			m2m_wifi_handle_events(NULL);
			hif_chip_sleep();

			if(gu8Sleep == PS_REQ_CHECK)
			{
				gu8Sleep = PS_REQ_SLEEP;
			}
		}
#endif
		if(bIsHttpClientRunning)
		{
			http_test_task(0);
		}

		if((gu8ApEnabled != 1)&&(gu8P2pEnabled != 1)&&(gu8Server!= 1))
		{
			if((gu8Connected == M2M_WIFI_CONNECTED)&& gu8Growl)
			{
				if(gu8Sleep == PS_REQ_SLEEP)
				{
					uint8 mode = m2m_wifi_get_sleep_mode();
					if((mode == M2M_PS_MANUAL)||(mode == M2M_PS_DEEP_AUTOMATIC))
					{
						//m2m_wifi_request_sleep(PS_SLEEP_TIME_MS/10);
						gu8Sleep = PS_SLEEP;
					}
				}
			}
		}
		nm_bsp_sleep(50);

	}

EXIT:
	_endthread();
}

void print_main_menu(void)
{
	printf("SVN REV %u SVN BR %s \n\r",SVN_REVISION,SVN_REL_URL);
	printf("Built at %s\t%s\n", __DATE__, __TIME__);
	printf("****************************************\n");
	printf("* 1. Default Connect                   *\n");
	printf("* 2. List Connect                      *\n");
	printf("* 3. Disconncet                        *\n");
	printf("* 4. Start Provisioning Mode           *\n");
	printf("* 5. WPS Trigger                       *\n");
	printf("* 6. WPS Stop                          *\n");
	printf("* 7. Req User Scan                     *\n");
	printf("* 8. Req current rssi                  *\n");
	printf("* 9. Req Scan result                   *\n");
	printf("* a. Init growl                        *\n");
	printf("* b. Send Prowl Notification           *\n");
	printf("* c. Send NMA Notification             *\n");
	printf("* d. Send NMA and Prowl Notifications  *\n");
	printf("* e. deInit growl                      *\n");
	printf("* f  Init Tcp Server                   *\n");
	printf("* g. deInit Tcp Server                 *\n");
	printf("* h. P2P Trigger                       *\n");
	printf("* i. MAC Address                       *\n");
	printf("* j. P2P mode stop                     *\n");
	printf("* k. AP mode start                     *\n");
	printf("* l. AP mode stop                      *\n");
	printf("* m. P2P or AP send triger             *\n");
	printf("* n. client led on                     *\n");
	printf("* o. client led off                    *\n");
	printf("* p  Init ps Server                    *\n");
	printf("* q. Send Ping Request                 *\n");
	printf("* w  wake client firmware              *\n");
	printf("* u. UDP TEST                          *\n");
	printf("* v. Firmware Version                  *\n");
	printf("* i. MAC Address                       *\n");
	printf("* x. Enable Monitoring Mode            *\n");
	printf("* y. Disable Monitoring Mode           *\n");
	printf("* A. OTA Start Update                  *\n");	
	printf("* B. OTA Switch                        *\n");
	printf("* C. OTA Image Rollback                *\n");
	printf("* E. Init Ethernet Mode                *\n");
	printf("* F. Print Network Info                *\n");
	printf("* G. GPIO test                         *\n");
	printf("* S. Stop Provision Mode               *\n");
	printf("* 0. Exit                              *\n");
	printf("****************************************\n");
}
sint32 main(void)
{
	char c;
	HANDLE hThread;
	HANDLE hTimerth;
	gu8ShouldExit = 0;

	genuActReq = ACT_REQ_NONE; 

	hThread = (HANDLE)_beginthread(execution_thread, 0, 0);
	hTimerth = (HANDLE)_beginthread(Timer, 0, 0);
	print_main_menu();

	for(;;)
	{
		c = getchar();
		switch(c)
		{
		case '1':
			genuActReq = ACT_REQ_CONNECT;
			break;
		case '2':
			genuActReq = ACT_REQ_CONNECT_LIST;
			break;
		case '3':
			genuActReq = ACT_REQ_DISCONNECT;
			break;
		case '4':
			genuActReq = ACT_REQ_START_PROVISIONING;
			break;
		case '5':
			genuActReq = ACT_REQ_WPS_CONNECT;
			break;
		case '6':
			genuActReq = ACT_REQ_WPS_DISABLE;
			break;
		case '7':
			genuActReq = ACT_REQ_SCAN;
			break;
		case '8':
			genuActReq = ACT_REQ_RSSI;
			break;
		case '9':
			genuActReq = ACT_REQ_SCAN_RESULT;
			break;
		case 'a':
			genuActReq = ACT_REQ_WIFI_GROWL_INIT;
			break;
		case 'b':
			genuActReq = ACT_REQ_START_PROWL_APP;
			break;
		case 'c':
			genuActReq = ACT_REQ_START_NMA_APP;
			break;
		case 'd':
			genuActReq = ACT_REQ_START_BOTH_APP;
			break;
		case 'e':
			genuActReq = ACT_REQ_WIFI_GROWL_DEINIT;
			break;
		case 'f':
			genuActReq = ACT_REQ_TCP_INIT;
			break;
		case 'g':
			genuActReq = ACT_REQ_TCP_DEINIT;
			break;
		case 'h':
			genuActReq = ACT_REQ_P2P_CONNECT;
			break;
		case 'i':
			genuActReq = ACT_REQ_MAC_ADDRESS;
			break;
		case 'j':
			genuActReq = ACT_REQ_P2P_DISCONNECT;
			break;
		case 'k':
			genuActReq = ACT_REQ_ENABLE_AP;
			break;
		case 'l':
			genuActReq = ACT_REQ_DISABLE_AP;
			break;
		case 'm':
			genuActReq = ACT_REQ_TCP_SEND;
			break;
		case 'n':
			genuActReq = ACT_REQ_CLIENT_LED_ON;
			break;
		case 'o':
			genuActReq = ACT_REQ_CLIENT_LED_OFF;
			break;
		case 'p':
			genuActReq = ACT_REQ_SERVER_INIT;
			break;
		case 'q':
			genuActReq = ACT_REQ_SEND_PING_REQUEST;
			break;
		case 'u':
			genuActReq = ACT_REQ_UDP_TEST;
			break;
		case 'v':
			genuActReq = ACT_REQ_FIRMWARE_INFO;
			break;
		case 'w':
			genuActReq = ACT_REQ_CLIENT_WAKE;
			break;
		case 'x':
			genuActReq = ACT_REQ_ENABLE_MONITOR_MODE;
			break;
		case 'y':
			genuActReq = ACT_REQ_DISABLE_MONITOR_MODE;
			break;
		case 'z':
			genuActReq = ACT_REQ_MEM_DUMP;
			break;
		case 'A':
			genuActReq = ACT_REQ_OTA_START_UPDATE;
			break;
		case 'B':
			genuActReq = ACT_REQ_OTA_SWITCH;
			break;
		case 'C':
			genuActReq = ACT_REQ_OTA_ROLL;
			break;
		case 'E':
			genuActReq = ACT_REQ_ETH_MODE_INIT;
			break;
		case 'F':
			genuActReq = ACT_REQ_GET_CONN_INFO;
			break;
		case 'G':
			genuActReq = ACT_REQ_GPIO_TEST;
			break;
		case 'S':
			genuActReq = ACT_REQ_STOP_PROV_MODE;
			break;
		case 'H':
			genuActReq = ACT_REQ_HTTP_CLIENT_TEST;
			break;
		case 'D':
			genuActReq = ACT_REQ_GET_RAND;
			break;
		case '0':
			genuActReq = ACT_REQ_EXIT;
			goto EXIT;
		}
	}
EXIT:
	WaitForSingleObject( hThread, INFINITE );
	return 0;
}

/*
int _main(void)
{
check_bus();
return 0;
}
*/
