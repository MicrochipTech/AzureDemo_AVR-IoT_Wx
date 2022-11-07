/*
    \file   core.h

    \brief  Application Core header file.

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

#ifndef CORE_H_
#define CORE_H_

#include "umqtt/umqtt.h"
#include "timeout.h"
#include "../../winc/socket/include/socket.h"
#include "timeout.h"

#define NO_TIMEOUT 0xFFFFFFFF
#define MAX_PROJECT_METADATA_LENGTH 30
#define MAX_WIFI_CREDENTIALS_LENGTH 15

typedef enum {
	NO_ERROR,
	WIFI_INIT_ERROR,
	WIFI_CONNECT_ERROR,
	UDP_SOCKET_ERROR,
	BIND_ERROR,
	NTP_REQ_SEND_ERROR,
	NO_NTP_RES_ERROR,
	DNS_REQ_ERROR,
	DNS_REPLY_ERROR,
	TCP_SOCKET_ERROR,
	TCP_CONNECT_ERROR,
	TLS_ERROR,
	MQTT_CONNECT_ERROR,
	BACK_OFF_FATAL_ERROR,
	TRANSMIT_ERROR,
	AP_ENABLE_ERROR
} error_t;

typedef enum {
	EXECUTE,
	PENDING,
	COMPLETED,
	FAILED
} state_status_t;

typedef struct state {
	struct state * nextState;
	error_t (*state_func)(void);
	state_status_t status;
	absolutetime_t timeout;
	error_t onFailError;
} state_t;

extern SOCKET tcp_client_socket;
extern SOCKET udp_socket;

extern uint32_t epoch;
extern uint32 mqttGoogleApisComIP;
extern uint8_t connect_attempts;

extern char cid[];
extern char mqtt_topic[];

extern char ssid[];
extern char pass[];
extern char auth_type[];

extern char project_id[];
extern char project_region[];
extern char registry_id[];
extern char device_id[];

extern state_t wifi_connect_state, bind_ntp_socket_state, ntp_req_state, dns_req_state, tls_connect_state, mqtt_connection_state, cloud_session_state, reconnect_state, ap_provision_state;

extern timer_struct_t sendToCloudTimer;

#endif /* CORE_H_ */