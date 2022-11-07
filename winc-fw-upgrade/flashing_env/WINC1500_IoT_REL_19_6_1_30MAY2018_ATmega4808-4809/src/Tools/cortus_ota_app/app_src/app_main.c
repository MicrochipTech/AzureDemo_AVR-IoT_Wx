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
#include "growl/include/growl.h"
#include "driver/include/m2m_ota.h"
#include "led_btn_demo.h"
#include "spi_flash_map.h"
#include "driver/source/nmasic.h"


#include "nmi_uart.h"
#include "nmi_gpio.h"
#include "nmi_spi.h"
#include "nmi_btn.h"


#define LED0 		GPIO_0_HOST_WAKEUP
#define LED1 		GPIO_1_RTC_CLK

#define ON 			(1)
#define OFF 		(0)
#define BLINK 		(2)



#define LED_GRWOL	 (LED0)
#define LED_WIFI	 (LED1)
#define LED_P2P		 (LED1)
#define LED_AP		 (LED1)
#define LED_DEMO1	 (LED0)
#define LED_DEMO2	 (LED0)
#define LED_DEMO3	 (LED0)
#define LED_ERR		 (LED1)

#define SEL(x,m1,m2,m3)  ((x>1)?((x>2)?(m3):(m2)):(m1))

const uint8 BITS_t[] =
{ 1, 2, 4, 8, 16, 32, 64, 128 };


/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
MACROS
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

#define WEP_KEY_MAX_INDEX					((uint8)4)
#define APP_TIMER_PERIOD					(300)	/*in ms*/
#define APP_SEM_POST(pSem)					app_os_sem_up(pSem)
#define APP_M2M_REQ_GROUP_IP				2

#define APP_P2P		1		/*p2p app*/
#define APP_AP      2		/*AP app*/
#define APP_WIFI    3		/*WI-FI app*/
#define APP_INIT	4		/*allow the user to choose from three apps using btn1,btn2 and timeout*/

#define APP_OTA_URL			"https://s3-us-west-1.amazonaws.com/mo.ismail/m2m_ota_3a0.bin"
#define APP_OTA_URL_CRT 	"https://s3-us-west-1.amazonaws.com/mo.ismail/cortus_app_ota.bin"

#define START_UP_APP APP_WIFI

#define APP_PROV_METHOD_HTTP        (0)
#define APP_PROV_METHOD_WPS         (1)

#define APP_PROV_METHOD             APP_PROV_METHOD_HTTP

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
	ACT_REQ_EXIT					= 0,
	ACT_REQ_NONE					= 127
} tenuActReq;

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
GLOBAL VARIABLES
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

uint8 	gau8StackApp[1 * 1024];
tstrOsTask gstrTaskApp;
tstrOsSemaphore gstrAppSem;


uint16              u16HttpRecvBufSize = 1500;//215040 ;
uint8   pu8HttpRecvBuf  [1500] ;

static sint8 gacDeviceName[] = M2M_DEVICE_NAME;
static uint8 gbWifiConnected = 0;
//static uint8 gbWifiConnectInprogress = 0;
static uint8 gu8Sleep = 0;
static uint8 gu8ConnectList = 0;
static uint8 u8index = 0;
static tenuActReq genuActReq = ACT_REQ_NONE;
static uint8 gbWpsActive		= 0;
static uint8 u8Socket_Opened = 0;
static uint8 gu8App = 0;
static uint8 gu8Prov;
static tstrOsTimer gstrTimerLed;

#ifdef UDP_TEST
uint8 gbUdpTestActive = 0;
#endif

#ifdef LIST_CONNECT
static tstrM2mWifiWepParams  gstrWepParams = WEP_CONN_PARAM;
static tstr1xAuthCredentials gstrCred1x    = AUTH_CREDENTIALS;
static tstrM2mAp gastrPreferredAPList[] = AP_LIST;
#endif

static tstrM2MAPConfig gstrM2MAPConfig = {
NMI_M2M_AP, NMI_M2M_AP_CHANNEL, M2M_WIFI_WEP_KEY_INDEX_1, sizeof(NMI_M2M_AP_WEP_KEY) - 1, NMI_M2M_AP_WEP_KEY, (uint8) NMI_M2M_AP_SEC,NMI_M2M_AP_SSID_MODE,HTTP_PROV_SERVER_IP_ADDRESS};
void flash_test(void)
{
	sint8  ret = 0;
	uint8 magic[40] = {0};
	uint32 offset = M2M_OTA_IMAGE2_OFFSET;
	/*
	 *
	 * the allowed area for user flash is from M2M_OTA_IMAGE2_OFFSET to M2M_APP_4M_MEM_FLASH_OFFSET which is around
	 */
	M2M_PRINT("app flash size %d\n",M2M_APP_4M_MEM_FLASH_OFFSET - M2M_OTA_IMAGE2_OFFSET);
	M2M_PRINT("app flash start address %d\n",M2M_OTA_IMAGE2_OFFSET);
	M2M_PRINT("app_spi_flash_get_size_inbyte %d\n",app_spi_flash_get_size_inbyte());
	M2M_PRINT("app_spi_flash_rdid %x\n",app_spi_flash_rdid());

	M2M_PRINT("*******************\n");
	ret = app_spi_flash_read(&magic[0],0,sizeof(magic));
	M2M_PRINT("spi_flash_read 0 %d\n",ret);
	M2M_PRINT(" %x%x%x%x\n",magic[0],magic[1],magic[2],magic[3]);
	ret = app_spi_flash_read(&magic[0],offset,sizeof(magic));
	M2M_PRINT("spi_flash_read %d %d\n",offset,ret);
	M2M_PRINT(" %x%x%x%x\n",magic[0],magic[1],magic[2],magic[3]);
	ret = app_spi_flash_erase(offset,FLASH_SECTOR_SZ);
	M2M_PRINT("spi_flash_erase %d %d\n",offset,ret);
	/*note that min erase size is 4K*/
	ret = app_spi_flash_read(&magic[0],offset,sizeof(magic));
	M2M_PRINT("spi_flash_read %d %d %d\n",offset,sizeof(magic),ret);
	M2M_PRINT(" %x%x%x%x\n",magic[0],magic[1],magic[2],magic[3]);
	magic[0]='M';
	magic[1]='I';
	magic[2]='I';
	magic[3]='O';
	ret = app_spi_flash_write(&magic[0],offset,sizeof(magic));
	M2M_PRINT("spi_flash_write %d %d %d\n",offset,sizeof(magic),ret);
	M2M_PRINT(" %x%x%x%x\n",magic[0],magic[1],magic[2],magic[3]);
	magic[0]='x';
	magic[1]='x';
	magic[2]='M';
	magic[3]='O';
	ret = app_spi_flash_read(&magic[0],offset,sizeof(magic));
	M2M_PRINT("spi_flash_read %d %d %d\n",offset,sizeof(magic),ret);
	M2M_PRINT(" %x%x%x%x\n",magic[0],magic[1],magic[2],magic[3]);
	ret = app_spi_flash_read(&magic[0],0,sizeof(magic));
	M2M_PRINT("spi_flash_read 0 %d\n",ret);
	M2M_PRINT(" %x%x%x%x\n",magic[0],magic[1],magic[2],magic[3]);
	M2M_PRINT("*******************\n");

}

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
PRIVATE APIs
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
/*======*======*======*======*
Host APIs
*======*======*======*======*/

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

void OtaNotifCb(tstrOtaUpdateInfo *pv)
{

}

void OtaUpdateCb(uint8 u8OtaUpdateStatusType ,uint8 u8OtaUpdateStatus)
 {
	sint8 ret;
	tstrM2mRev strtmp;
	ret = m2m_ota_get_firmware_version(&strtmp);
	if((ret == M2M_SUCCESS)||(ret == M2M_ERR_FW_VER_MISMATCH))
	{
		M2M_INFO("OTA Firmware ver   : %u.%u.%u Svnrev %u\n", strtmp.u8FirmwareMajor, strtmp.u8FirmwareMinor, strtmp.u8FirmwarePatch,strtmp.u16FirmwareSvnNum);
		M2M_INFO("OTA Min driver ver : %u.%u.%u\n", strtmp.u8DriverMajor, strtmp.u8DriverMinor, strtmp.u8DriverPatch);
		M2M_INFO("OTA Curr driver ver: %u.%u.%u Svnrev %u\n", M2M_DRIVER_VERSION_MAJOR_NO, M2M_DRIVER_VERSION_MINOR_NO, M2M_DRIVER_VERSION_PATCH_NO,M2M_DRIVER_SVN_VERSION);
		M2M_INFO("OTA Firmware Build %s Time %s\n",strtmp.BuildDate,strtmp.BuildTime);
	}
	if(u8OtaUpdateStatusType == DL_STATUS)
		{
		if(u8OtaUpdateStatus == OTA_STATUS_SUCCESS)
			{
				M2M_INFO(" Switch Cortus Image\n");
				m2m_ota_switch_crt();		
			}
		 }
	else if(u8OtaUpdateStatusType == SW_STATUS) 
		{
		if(u8OtaUpdateStatus == OTA_STATUS_SUCCESS)
			{
				M2M_INFO("Now reset the system...");
			}
		}
}

static void wifi_cb(uint8 u8MsgType, void * pvMsg)
{
	if (u8MsgType == M2M_WIFI_RESP_CON_STATE_CHANGED)
	{
		tstrM2mWifiStateChanged *pstrWifiState =(tstrM2mWifiStateChanged*) pvMsg;
		M2M_INFO("Wifi State \"%s\"\n", pstrWifiState->u8CurrState? "CONNECTED":"DISCONNECTED");
		if (pstrWifiState->u8CurrState == M2M_WIFI_CONNECTED)
		{
#ifdef CONF_STATIC_IP_ADDRESS
			tstrM2MIPConfig strStaticConf;
			strStaticConf.u32StaticIP = nmi_inet_addr(STATIC_IP_ADDRESS);
			strStaticConf.u32DNS = nmi_inet_addr(DNS_ADDRESS);
			strStaticConf.u32Gateway = nmi_inet_addr(DEFAULT_GATEWAY_ADDRESS);
			strStaticConf.u32SubnetMask = nmi_inet_addr(SUBNET_MASK);
			m2m_wifi_set_static_ip(&strStaticConf);
			gu8Sleep = PS_REQ_SLEEP;
			gbWifiConnected = M2M_WIFI_CONNECTED;
			led_ctl(LED_WIFI, ON);
			M2M_INFO("STATIC IP Address :: %s ::\n",STATIC_IP_ADDRESS);
#else
			gu8Sleep = PS_WAKE;
			if (gu8App == APP_AP)
			{
			}
			else if (gu8App == APP_P2P)
			{ 
			}
			else if (gu8App == APP_WIFI)
			{
			}
			

			m2m_ota_start_update_crt((uint8 *)APP_OTA_URL_CRT);	
#endif
		}
		else if (pstrWifiState->u8CurrState == M2M_WIFI_DISCONNECTED)
		{
			gbWifiConnected = M2M_WIFI_DISCONNECTED;
			gu8Sleep = PS_WAKE;
			u8Socket_Opened = 0;
			if (gu8App == APP_P2P)
			{

			}
			else if (gu8App == APP_AP)
			{

			}
			else if (gu8App == APP_WIFI)
			{


				if ((gbWpsActive == WPS_DISABLED)&&(gu8Prov == PROV_DISABLED))
				{
					m2m_wifi_request_scan(M2M_WIFI_CH_ALL);
					gu8ConnectList = 1;
				}
			}
		}
	}
	else if (u8MsgType == M2M_WIFI_RESP_GET_SYS_TIME)
	{
		tstrSystemTime *pstrTime = (tstrSystemTime*)pvMsg;
		M2M_INFO("Time Of Day\n\t%d/%02d/%d %02d:%02d:%02d GMT\n",
		pstrTime->u8Month, pstrTime->u8Day, pstrTime->u16Year,
		pstrTime->u8Hour, pstrTime->u8Minute, pstrTime->u8Second);

	}
	else if (u8MsgType == M2M_WIFI_REQ_DHCP_CONF)
	{
		gbWifiConnected = M2M_WIFI_CONNECTED;
		tstrM2MIPConfig* pstrM2MIpConfig = (tstrM2MIPConfig*) pvMsg;
		uint8 *pu8IPAddress = (uint8*) &pstrM2MIpConfig->u32StaticIP;
		if (gu8App == APP_AP)
		{


		}
		else
		{
			M2M_INFO("DHCP IP Address \"%u.%u.%u.%u\"\n",pu8IPAddress[0],pu8IPAddress[1],pu8IPAddress[2],pu8IPAddress[3]);
			if (gu8App == APP_P2P)
			{

			}
			else if (gu8App == APP_WIFI)
			{

				if(gu8Prov != PROV_ENABLED) {
					genuActReq = ACT_REQ_START_BOTH_APP;
					APP_SEM_POST(&gstrAppSem);
				}
			}
			gu8Sleep = PS_REQ_SLEEP;
		}
	}
	else if(u8MsgType == M2M_WIFI_REQ_WPS)
	{
		tstrM2MWPSInfo	*pstrWPS = (tstrM2MWPSInfo*)pvMsg;
		if (gu8App == APP_WIFI)
		{
			//led_ctl(LED_ERR, OFF);

		}
		gu8Sleep = PS_WAKE;
		gbWpsActive = WPS_DISABLED;
		if(pstrWPS->u8AuthType == 0)
		{
			M2M_ERR("WPS Is not enabled OR Timedout\n");
			/* WPS is not enabled by firmware OR WPS monitor timeout. So, resume connect. */
			m2m_wifi_request_scan(M2M_WIFI_CH_ALL);
			gu8ConnectList = 1;
		}
		else
		{
			m2m_wifi_connect((char*)pstrWPS->au8SSID, (uint8)m2m_strlen(pstrWPS->au8SSID),
				pstrWPS->u8AuthType, pstrWPS->au8PSK, pstrWPS->u8Ch);
		}
	}
	else if (u8MsgType == M2M_WIFI_RESP_IP_CONFLICT)
	{
		if (gu8App == APP_WIFI)
		{

			gu8Sleep = PS_WAKE;
		}
	}
	else if (u8MsgType == M2M_WIFI_RESP_CLIENT_INFO)
	{
		uint8 *pstrInfo = (uint8*) pvMsg;
		gu8Sleep = PS_WAKE;
		if (*pstrInfo == M2M_CLIENT_RESP_MOVEMENT)
		{

		}
		else if (*pstrInfo == M2M_CLIENT_RESP_BTN1_PRESS)
		{

		}
		else if (*pstrInfo == M2M_CLIENT_RESP_BTN2_PRESS)
		{

		}
	}
	else if(u8MsgType == M2M_WIFI_RESP_DEFAULT_CONNECT)
	{
		tstrM2MDefaultConnResp	*pstrResp = (tstrM2MDefaultConnResp*)pvMsg;
		if(pstrResp->s8ErrorCode == M2M_DEFAULT_CONN_EMPTY_LIST)
		{

#if (APP_PROV_METHOD == APP_PROV_METHOD_HTTP)
			if (gu8Prov == PROV_DISABLED) {

			}
#elif (APP_PROV_METHOD == APP_PROV_METHOD_WPS)
			if(gu8WPS == WPS_DISABLED) {
				app_wifi_start_stop_prov();
			}
#endif
		}
		else if(pstrResp->s8ErrorCode == M2M_DEFAULT_CONN_SCAN_MISMATCH)
		{

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
	else if(u8MsgType == M2M_WIFI_RESP_PROVISION_INFO)
	{
		tstrM2MProvisionInfo	*pstrProvInfo = (tstrM2MProvisionInfo*)pvMsg;
		if (gu8App == APP_WIFI)
		{


		}
		gu8Sleep = PS_WAKE;
		gu8Prov = PROV_DISABLED;
		if(pstrProvInfo->u8Status == M2M_SUCCESS)
		{
			M2M_INFO("PROV SSID : %s\n",pstrProvInfo->au8SSID);
			if(pstrProvInfo->u8SecType != M2M_WIFI_SEC_OPEN) {
				M2M_INFO("PROV PSK  : %s\n",pstrProvInfo->au8Password);
			}

			m2m_wifi_connect((char*)pstrProvInfo->au8SSID, (uint8)m2m_strlen((uint8*)pstrProvInfo->au8SSID), pstrProvInfo->u8SecType,
					pstrProvInfo->au8Password, M2M_WIFI_CH_ALL);
		}
		else
		{
			// TODO: Assign LEDs foe Error or Success
			M2M_ERR("Provisioning Failed\n");

		}
	}
	else if (u8MsgType == M2M_WIFI_RESP_SCAN_DONE)
	{
		tstrM2mScanDone *pstrInfo = (tstrM2mScanDone*) pvMsg;
		gu8Sleep = PS_REQ_SLEEP;
		M2M_INFO("Num of AP found %d\n", pstrInfo->u8NumofCh);
		if(pstrInfo->s8ScanState == M2M_SUCCESS)
		{
			if(gu8ConnectList)
			{
				u8index = 0;
				gu8Sleep = PS_WAKE;
				if (pstrInfo->u8NumofCh >= 1)
				{
					m2m_wifi_req_scan_result(u8index);
					u8index++;
				}
				else
				{
					M2M_INFO("No AP Found Rescan\n");
					m2m_wifi_request_scan(M2M_WIFI_CH_ALL);
				}
			}
			else
			{
				/**/
			}
		}
		else
		{
			M2M_ERR("Scan fail %d\n",pstrInfo->s8ScanState);
		}
	}
	else if (u8MsgType == M2M_WIFI_RESP_CURRENT_RSSI)
	{
		sint8 *rssi = (sint8*) pvMsg;
		gu8Sleep = PS_REQ_SLEEP;
		M2M_INFO("ch rssi %d\n", *rssi);
	}
	else if (u8MsgType == M2M_WIFI_RESP_SCAN_RESULT)
	{
		tstrM2mWifiscanResult		*pstrScanResult =(tstrM2mWifiscanResult*)pvMsg;
		uint8						u8NumFoundAPs = m2m_wifi_get_num_ap_found();
		M2M_INFO(">>%02d RI %d SEC %s CH %02d SSID %s\n",pstrScanResult->u8index,pstrScanResult->s8rssi,
			M2M_SEL(pstrScanResult->u8AuthType,"OPEN","WPA ","WEP "),pstrScanResult->u8ch,pstrScanResult->au8SSID);
		if(gu8ConnectList)
		{
			uint8 j = 0;
			gu8Sleep = PS_WAKE;
			for (j = 0; j < M2M_AP_LIST_SZ; j++)
			{
				if(m2m_memcmp((uint8*)pstrScanResult->au8SSID,
					(uint8*)gastrPreferredAPList[j].au8Ssid, 
					m2m_strlen((uint8*)gastrPreferredAPList[j].au8Ssid)) == 0)
				{
					/* A scan result matches an entry in the preferred AP List.
					Initiate a connection request.
					*/
					M2M_INFO("match\n");
					u8index			= 0;
					gu8ConnectList	= 0;

					m2m_wifi_connect((char*)pstrScanResult->au8SSID,
						(uint8)m2m_strlen((uint8*)pstrScanResult->au8SSID),
						gastrPreferredAPList[j].u8AuthType,
						(void*)gastrPreferredAPList[j].pu8AuthCred,
						(pstrScanResult->u8ch));
					return;
				}
			}
		}
		if(u8index < u8NumFoundAPs)
		{
			gu8Sleep = PS_WAKE;
			m2m_wifi_req_scan_result(u8index);
			u8index++;
		}
		else
		{
			if (gu8ConnectList)
			{
				gu8Sleep = PS_WAKE;
				M2M_INFO("No AP Match Rescan\n");
				m2m_wifi_request_scan(M2M_WIFI_CH_ALL);

			}
			else
			{
				gu8Sleep = PS_REQ_SLEEP;
			}
			u8index = 0;
		}
	}
	else
	{
		M2M_ERR("req not defined %x\n", u8MsgType);
	}
}
/*======*======*======*======*
main APIs
*======*======*======*======*/

void app_main_context(void* pv)
{
	gu8Sleep = PS_WAKE;
	M2M_REQ("ACT_REQ_CONNECT\n");
	m2m_wifi_connect(DEFAULT_SSID, sizeof(DEFAULT_SSID),
		DEFAULT_AUTH, DEFAULT_KEY, M2M_WIFI_CH_ALL);
	for (;;)
	{
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


sint8 app_start(void)
{
	tstrM2mRev strtmp;
	uint8 mac_addr[6];
	uint8 u8IsMacAddrValid;
	tstrWifiInitParam param;
	sint8 ret = M2M_SUCCESS;
	tstrM2mLsnInt strM2mLsnInt;
	genuActReq = ACT_REQ_NONE;
	gu8App = START_UP_APP;
	m2m_memset((uint8*) &gstrAppSem, 0, sizeof(gstrAppSem));
	m2m_memset((uint8*) &gstrTimerLed, 0, sizeof(gstrTimerLed));

	M2M_PRINT("APP Started.\n");

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
	set_dev_name_to_mac((uint8*)gstrM2MAPConfig.au8SSID, mac_addr);
	m2m_wifi_set_device_name((uint8*)gacDeviceName, (uint8)m2m_strlen((uint8*)gacDeviceName));
	m2m_ota_init(OtaUpdateCb, OtaNotifCb);
#ifdef _DYNAMIC_PS_
	m2m_wifi_set_sleep_mode(M2M_PS_DEEP_AUTOMATIC, 1);
	strM2mLsnInt.u16LsnInt = M2M_LISTEN_INTERVAL;
	m2m_wifi_set_lsn_int(&strM2mLsnInt);
#endif
	m2m_wifi_set_device_name((uint8*)gacDeviceName, (uint8)m2m_strlen((uint8*)gacDeviceName));

#ifdef GROWL
	//NMI_GrowlInit((uint8*) PROWL_API_KEY, (uint8*) NMA_API_KEY);
#endif

	ret = m2m_wifi_get_firmware_version(&strtmp);
	//M2M_INFO("Firmware ver   : %u.%u.%u\n", strtmp.u8FirmwareMajor, strtmp.u8FirmwareMinor, strtmp.u8FirmwarePatch);
	//M2M_INFO("Min driver ver : %u.%u.%u\n", strtmp.u8DriverMajor, strtmp.u8DriverMinor, strtmp.u8DriverPatch);
	M2M_INFO("Firmware Build %s Time %s\n",strtmp.BuildDate,strtmp.BuildTime);

	ret = m2m_ota_get_firmware_version(&strtmp);

	if((ret == M2M_SUCCESS)||(M2M_ERR_FW_VER_MISMATCH == ret))
	{
		M2M_INFO("OTA Firmware ver   : %u.%u.%u Svnrev %u\n", strtmp.u8FirmwareMajor, strtmp.u8FirmwareMinor, strtmp.u8FirmwarePatch,strtmp.u16FirmwareSvnNum);
		M2M_INFO("OTA Min driver ver : %u.%u.%u\n", strtmp.u8DriverMajor, strtmp.u8DriverMinor, strtmp.u8DriverPatch);
		M2M_INFO("OTA Firmware Build %s Time %s\n",strtmp.BuildDate,strtmp.BuildTime);
	}

	app_os_sem_init(&gstrAppSem, "APP", 0);
	app_os_sch_task_create(&gstrTaskApp, app_main_context, "APP", gau8StackApp,
		sizeof(gau8StackApp), 100);
ERR:
	return ret;
}
