/*
    \file   api.c

    \brief  Application Main source file.

    (c) 2018 Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip software and any
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party
    license terms applicable to your use of third party software (including open source software) that
    may accompany Microchip software.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS
    FOR A PARTICULAR PURPOSE.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS
    SOFTWARE.
*/

#include <string.h>
#include "api.h"
#include "core/core.h"
#include "atmel_start_pins.h"
#include "../winc/driver/include/m2m_wifi.h"
#include "network_control/network_control.h"
#include "credentials_storage/credentials_storage.h"
#include "cli/cli.h"
#include "mqtt_client/mqtt_client.h"

#define CLOUD_CLI
#define MAIN_WLAN_SSID              "zambiluta" /**< Destination SSID */
#define MAIN_WLAN_AUTH				M2M_WIFI_SEC_WPA_PSK
#define MAIN_WLAN_PSK               "microchip" /**< Password for Destination SSID */
#define MAX_BACK_OFF_RETRIES 8

static uint8_t back_off_intervals[MAX_BACK_OFF_RETRIES] = {1, 2, 4, 8, 16, 32, 32, 32};
static state_t *current_state;
static timer_struct_t wait_timer, back_off_timer;
static uint16_t back_off_time;

static void error_handler(error_t err);
static absolutetime_t wait_timer_cb(void *payload);
static absolutetime_t back_off_timer_cb(void *payload);
static void promice(state_t *current_state);

uint8_t connect_attempts;

static bool isProvisionRequested() 
{
	if (!SW0_get_level())
	{
		return true;
	}
	else 
	{
		return false;
	}
}

void CLOUD_init(void) 
{
	tstrWifiInitParam param;
	param.pfAppWifiCb = NETWORK_CONTROL_wifiHandler;
	nm_bsp_init();
	m2m_wifi_init(&param);
	
	if(isProvisionRequested()) 
	{
		current_state = &ap_provision_state;
		current_state->status = EXECUTE;
	}
	else
	{
		CREDENTIALS_STORAGE_read(ssid, pass, auth_type);

		if (ssid[0] ==  0xFF)
		{
			for (uint8_t j=0; j<= MAX_WIFI_CREDENTIALS_LENGTH; j++)
			{
				ssid[j] = 0;
				pass[j] = 0;
			}
			strcpy(ssid, MAIN_WLAN_SSID);
			strcpy(pass, MAIN_WLAN_PSK);
			itoa(MAIN_WLAN_AUTH, (char*)auth_type, 10);
		}
		current_state = &wifi_connect_state;
		current_state->status = EXECUTE;	
	}
}

void CLOUD_reconnect(void) 
{
	CREDENTIALS_STORAGE_read(ssid, pass, auth_type);

	if (ssid[0] ==  0xFF)
	{
		for (uint8_t j=0; j<= MAX_WIFI_CREDENTIALS_LENGTH; j++)
		{
			ssid[j] = 0;
			pass[j] = 0;
		}
		strcpy(ssid, MAIN_WLAN_SSID);
		strcpy(pass, MAIN_WLAN_PSK);
		itoa(MAIN_WLAN_AUTH, (char*)auth_type, 10);
	}
	current_state = &wifi_connect_state;
	current_state->status = EXECUTE;	
}

void CLOUD_task(void) 
{
	error_t err;
	
	m2m_wifi_handle_events(NULL);
	
	#ifdef CLOUD_CLI 
	CLI_commandReceive();
	#endif
	
	switch(current_state->status) 
	{
		case EXECUTE:
			err = current_state->state_func();
			if(err != NO_ERROR) 
			{
				error_handler(err);
			} 
			else 
			{
				promice(current_state);	
			}
			break;
		
		case PENDING:
			scheduler_timeout_call_next_callback();
			break;
		
		case COMPLETED:
			current_state = current_state->nextState;
			current_state->status = EXECUTE;
			scheduler_timeout_delete(&wait_timer);
			break;
		
		case FAILED:
			scheduler_timeout_delete(&wait_timer);
			error_handler(current_state->onFailError);
			break;
		
		default:
			break;
	}
}

void CLOUD_send(uint8_t *data, uint8_t datalen) 
{
	MQTT_CLIENT_publish(data, datalen);
}

static void error_handler(error_t err) 
{	
	switch(err) 
	{
		case TRANSMIT_ERROR:
			current_state = current_state->nextState;
			current_state->status = EXECUTE;
			LED_RED_set_level(false);
			break;
		case BACK_OFF_FATAL_ERROR:
			LED_RED_set_level(false);
			while(1) {;}
			break;
		case TLS_ERROR:
			tls_connect_state.status = EXECUTE;
			LED_RED_set_level(false);
			break;
		case MQTT_CONNECT_ERROR:
			// Exponential back off variant with cvasi-constant max delay
			if(connect_attempts < MAX_BACK_OFF_RETRIES) 
			{
				current_state->status = PENDING;
				back_off_time = back_off_intervals[connect_attempts++] * 1000 + rand() % 1000;
				back_off_timer.callback_ptr = back_off_timer_cb;
				scheduler_timeout_create(&back_off_timer, back_off_time);
			} 
			else 
			{
				error_handler(BACK_OFF_FATAL_ERROR);
			}
			LED_RED_set_level(false);
			break;
		case NO_NTP_RES_ERROR: // Try again
			current_state->status = EXECUTE;
			LED_RED_set_level(false);
			break;
		case DNS_REPLY_ERROR: // Try again
			current_state->status = EXECUTE;
			LED_RED_set_level(false);
			break;
		case WIFI_INIT_ERROR: // Try again
			current_state->status = EXECUTE;
			LED_RED_set_level(false);
			break;
		case WIFI_CONNECT_ERROR:
			LED_RED_set_level(false);
			break;
		case UDP_SOCKET_ERROR:
			LED_RED_set_level(false);
			break;
		case BIND_ERROR:
			LED_RED_set_level(false);
			break;
		case TCP_SOCKET_ERROR:
			LED_RED_set_level(false);
			break;
		case TCP_CONNECT_ERROR:
			LED_RED_set_level(false);
			break;
		default:
			CLOUD_init();
			break;
	}
}

absolutetime_t wait_timer_cb(void *payload)
{
	current_state->status = FAILED;
	return 0;
}

absolutetime_t back_off_timer_cb(void *payload)
{
	current_state = &tls_connect_state;
	current_state->status = EXECUTE;
	return 0;
}

static void promice(state_t *current_state)
{
	switch(current_state->timeout)
	{
		case NO_TIMEOUT:
			current_state->status = PENDING;
			break;
		default:
			current_state->status = PENDING;
			wait_timer.callback_ptr = wait_timer_cb;
			scheduler_timeout_create(&wait_timer, current_state->timeout);
			break;
	}
}