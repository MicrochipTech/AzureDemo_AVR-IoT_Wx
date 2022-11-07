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
#include "driver/include/m2m_wifi.h"
#include "driver/include/m2m_ota.h"
#include "driver/source/nmasic.h"
#include "spi_flash_map.h"


#include "nmi_uart.h"
#include "nmi_gpio.h"
#include "nmi_spi.h"
#include "nmi_btn.h"
#include "nmi_pwm.h"


#define DEFAULT_SSID				"DEMO_AP"
#define DEFAULT_AUTH				M2M_WIFI_SEC_WPA_PSK
#define	DEFAULT_KEY					"12345678"


/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
GLOBAL VARIABLES
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

uint8 	gau8StackApp[1 * 1024];
tstrOsTask gstrTaskApp;
tstrOsSemaphore gstrAppSem;
static uint8 gbWifiConnected = 0;

#define TEST_PWM
#ifdef TEST_PWM
void pwm_test()
{
	int fadeValue = 0;
	pwm_init(1);
	M2M_PRINT("TEST_PWM\n");
	while(1)
	{
	  // fade in from min to max in increments of 20 points:
	  for (fadeValue = 0 ; fadeValue <= 1022; fadeValue += 20) {
		// sets the value (range from 0 to 1022):
		  pwm_set_duty_cycle(fadeValue);
		// wait for 10 milliseconds to see the dimming effect
		  app_os_sch_task_sleep(1);
	  }

	  // fade out from max to min in increments of 20 points:
	  for (fadeValue = 1022 ; fadeValue >= 0; fadeValue -= 20) {
			// sets the value (range from 0 to 1022):
		  pwm_set_duty_cycle(fadeValue);
			// wait for 10 milliseconds to see the dimming effect
		  app_os_sch_task_sleep(1);
	  }
	}
}
#endif
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
PRIVATE APIs
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
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
		M2M_INFO("DHCP IP Address \"%u.%u.%u.%u\"\n",pu8IPAddress[0],pu8IPAddress[1],pu8IPAddress[2],pu8IPAddress[3]);
#ifdef TEST_PWM
		pwm_test();
#endif

	}
}
/*======*======*======*======*
main APIs
*======*======*======*======*/

void app_main_context(void* pv)
{

	M2M_REQ("ACT_REQ_CONNECT\n");
	m2m_wifi_connect(DEFAULT_SSID, sizeof(DEFAULT_SSID),
		DEFAULT_AUTH, DEFAULT_KEY, M2M_WIFI_CH_ALL);

	for (;;)
	{
		app_os_sem_down(&gstrAppSem);
		m2m_wifi_handle_events(NULL);

	}
}



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
sint8 app_start(void)
{
	tstrWifiInitParam param;
	sint8 ret = M2M_SUCCESS;

	m2m_memset((uint8*) &gstrAppSem, 0, sizeof(gstrAppSem));
	m2m_memset((uint8*)&param, 0, sizeof(param));
	param.pfAppWifiCb = wifi_cb;

	ret = m2m_wifi_init(&param);
	if(ret != M2M_SUCCESS)
	{
		M2M_ERR("Driver init fail\n");
		goto ERR;
	}
	
	flash_test();

	app_os_sem_init(&gstrAppSem, "APP", 0);
	app_os_sch_task_create(&gstrTaskApp, app_main_context, "APP", gau8StackApp,
		sizeof(gau8StackApp), 100);
ERR:
	return ret;
}
