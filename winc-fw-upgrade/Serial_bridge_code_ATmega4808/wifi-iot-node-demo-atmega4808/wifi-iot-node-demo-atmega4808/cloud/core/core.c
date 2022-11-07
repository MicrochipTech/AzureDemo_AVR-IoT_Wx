/*
    \file   core.c

    \brief  Application Core source file.

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
#include <stdio.h>
#include "winc/driver/include/m2m_wifi.h"
#include "timeout.h"
#include "core.h"
#include "../api.h"
#include "../network_control/network_control.h"
#include "../mqtt_client/mqtt_client.h"
#include "../crypto_client/crypto_client.h"
#include "adc_basic.h"
#include "atmel_start_pins.h"

#define WLAN_AP_NAME                "AVR.IoT"
#define WLAN_AP_CHANNEL				1
#define WLAN_AP_WEP_INDEX			0
#define WLAN_AP_WEP_SIZE			WEP_40_KEY_STRING_SIZE
#define WLAN_AP_WEP_KEY				"1234567890"
#define WLAN_AP_SECURITY			M2M_WIFI_SEC_OPEN
#define WLAN_AP_MODE				SSID_MODE_VISIBLE
#define WLAN_AP_DOMAIN_NAME			"microchipconfig.com"
#define WLAN_AP_IP_ADDRESS			{192, 168, 1, 1}
#define MQTT_CID_LENGTH 100
#define MQTT_TOPIC_LENGTH 30
#define MQTT_HOST "mqtt.googleapis.com"

SOCKET tcp_client_socket = -1;
SOCKET udp_socket = -1;

uint32_t epoch = 0;
uint32 mqttGoogleApisComIP;

char cid[MQTT_CID_LENGTH];
char mqtt_topic[MQTT_TOPIC_LENGTH];

char ssid[MAX_WIFI_CREDENTIALS_LENGTH];
char pass[MAX_WIFI_CREDENTIALS_LENGTH];
char auth_type[2];

// char project_region[] = "asia-east1";
// char registry_id[] = "my-registry";
// char device_id[] = "device-10";

char project_id[] = "civil-cascade-205811";
char project_region[] = "us-central1";
char registry_id[] = "my-registry";
char device_id[] = "my-device1";

static error_t enable_provision_ap(void);
static error_t wifi_connect(void);
static error_t bind_ntp_socket(void);
static error_t ntp_req(void);
static error_t dns_req(void);
static error_t tls_connect();
static error_t cloud_connect(void);
static error_t set_up_send_timer(void);

state_t ap_provision_state = {
	.state_func = enable_provision_ap,
	.timeout = NO_TIMEOUT,
	.status = EXECUTE
};

state_t wifi_connect_state = {
	.state_func = &wifi_connect,
	.nextState = &bind_ntp_socket_state,
	.status = EXECUTE,
	.timeout = 5000,
	.onFailError = WIFI_CONNECT_ERROR
};

state_t bind_ntp_socket_state = {
	.state_func = bind_ntp_socket,
	.nextState = &ntp_req_state,
	.status = EXECUTE,
	.timeout = 5000,
	.onFailError = BIND_ERROR
};

state_t ntp_req_state = {
	.state_func = ntp_req,
	.nextState = &dns_req_state,
	.status = EXECUTE,
	.timeout = 5000,
	.onFailError = NO_NTP_RES_ERROR
};

state_t dns_req_state = {
	.state_func = dns_req,
	.nextState = &tls_connect_state,
	.status = EXECUTE,
	.timeout = 5000,
	.onFailError = DNS_REPLY_ERROR,
};

state_t tls_connect_state = {
	.state_func = tls_connect,
	.nextState = &mqtt_connection_state,
	.status = EXECUTE,
	.timeout = 5000,
	.onFailError = TLS_ERROR
};

state_t mqtt_connection_state = {
	.state_func = cloud_connect,
	.nextState = &cloud_session_state,
	.status = EXECUTE,
	.timeout = 1000,
	.onFailError = MQTT_CONNECT_ERROR
};

state_t cloud_session_state = {
	.state_func = set_up_send_timer,
	.nextState = &ntp_req_state,
	.status = EXECUTE,
	.timeout = NO_TIMEOUT,
	.onFailError =	TRANSMIT_ERROR
};

timer_struct_t sendToCloudTimer;

static error_t enable_provision_ap(void)
{
	tstrM2MAPConfig ap_config = {
		WLAN_AP_NAME, // Access Point Name.
		WLAN_AP_CHANNEL, // Channel to use.
		WLAN_AP_WEP_INDEX, // Wep key index.
		WLAN_AP_WEP_SIZE, // Wep key size.
		WLAN_AP_WEP_KEY, // Wep key.
		WLAN_AP_SECURITY, // Security mode.
		WLAN_AP_MODE, // SSID visible.
		WLAN_AP_IP_ADDRESS // DHCP server IP
	};
	static CONST char gacHttpProvDomainName[] = WLAN_AP_DOMAIN_NAME;
	m2m_wifi_start_provision_mode((tstrM2MAPConfig *)&ap_config, (char*)gacHttpProvDomainName, 1);

	while(!WIFI_provision)
	{
		/* Handle pending events from network controller while waiting for credentials */
		m2m_wifi_handle_events(NULL);
	}

	return NO_ERROR;
}

static error_t wifi_connect(void)
{
	/* Connect to router. */
	if(M2M_SUCCESS != m2m_wifi_connect((char *)ssid, sizeof(ssid), atoi((char*)auth_type), (char *)pass, M2M_WIFI_CH_ALL))
	{
		return WIFI_CONNECT_ERROR;
	}

	socketInit();
	registerSocketCallback(NETWORK_CONTROL_socketHandler, NETWORK_CONTROL_dnsHandler);
	
	return NO_ERROR;
}

static error_t bind_ntp_socket(void)
{
	struct sockaddr_in addr_in;
	close(udp_socket);
	// Get NPT
	udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (udp_socket < 0) {
		return UDP_SOCKET_ERROR;
	}

	/* Initialize default socket address structure. */
	addr_in.sin_family = AF_INET;
	addr_in.sin_addr.s_addr = _htonl(0xffffffff);
	addr_in.sin_port = _htons(6666);

	bind(udp_socket, (struct sockaddr *)&addr_in, sizeof(struct sockaddr_in));

	return NO_ERROR;
}

static error_t ntp_req(void)
{
	struct sockaddr_in addr;
	int8_t cDataBuf[48];
	int16_t ret;

	memset(cDataBuf, 0, sizeof(cDataBuf));
	cDataBuf[0] = 0x1B; /* time query */

	addr.sin_family = AF_INET;
	addr.sin_port = _htons(123);
	addr.sin_addr.s_addr = _htonl(0xD8EF2308); // 0xD8EF2308 time.google.com; 0xC0A80037 - alex pc on zambiluta network

	/*Send an NTP time query to the NTP server*/
	ret = sendto(udp_socket, (int8_t *)&cDataBuf, sizeof(cDataBuf), 0, (struct sockaddr *)&addr, sizeof(addr));
	if( ret < 0 ) {
		return NTP_REQ_SEND_ERROR;
	}
	
	return NO_ERROR;
}

static error_t dns_req(void)
{
	uint8_t ret = gethostbyname((uint8_t*)MQTT_HOST);
	if(ret != SOCK_ERR_NO_ERROR)
	{
		return DNS_REQ_ERROR;
	}
	connect_attempts = 0;
	return NO_ERROR;
}

static error_t tls_connect(void)
{
	struct sockaddr_in addr;
	close(tcp_client_socket);
	if ((tcp_client_socket = socket(AF_INET, SOCK_STREAM, 1)) < 0)
	{
		return TCP_SOCKET_ERROR;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = _htons(8883);
	addr.sin_addr.s_addr = mqttGoogleApisComIP;
	//addr.sin_addr.s_addr = _htonl(0xC0A80037);
	//addr.sin_addr.s_addr = _htonl(0x40E9B8CE); // 0x40E9B8CE - mqtt.googleapis.com, 0xC0A80036 - 0.54, 0xC0A80037 - 0.55

	/* Connect server */
	sint8 ret = connect(tcp_client_socket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));

	if (ret < 0)
	{
		close(tcp_client_socket);
		tcp_client_socket = -1;
		return TCP_CONNECT_ERROR;
	}

	return NO_ERROR;
}

static error_t cloud_connect(void)
{
	CRYPTO_CLIENT_createJWT(mqtt_password, PASSWORD_SPACE, (uint32_t)epoch);

	sprintf(cid, "projects/%s/locations/%s/registries/%s/devices/%s", project_id, project_region, registry_id, device_id);
	sprintf(mqtt_topic, "/devices/%s/events", device_id);

	MQTT_CLIENT_connect();

	return NO_ERROR;
}

static error_t set_up_send_timer(void)
{
	sendToCloudTimer.callback_ptr = sendToCloud;
	scheduler_timeout_create(&sendToCloudTimer, 1);
	return NO_ERROR;
}
