/*
    \file   network_control.c

    \brief  Network Control source file.

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
#include "../mqtt_client/mqtt_client.h"
#include "network_control.h"
#include "../core/core.h"
#include "../credentials_storage/credentials_storage.h"
#include "atmel_start_pins.h"

#define MAIN_WIFI_M2M_BUFFER_SIZE		48

uint8_t gau8SocketBuffer[MAIN_WIFI_M2M_BUFFER_SIZE];
uint8_t WIFI_provision = false;

void NETWORK_CONTROL_socketHandler(SOCKET sock, uint8_t u8Msg, void *pvMsg)
{
	int16_t ret;
	
	switch (u8Msg) {
	case SOCKET_MSG_BIND:
	{
		tstrSocketBindMsg *pstrBind = (tstrSocketBindMsg *)pvMsg;
		if (pstrBind && pstrBind->status == 0) {
			ret = recvfrom(sock, gau8SocketBuffer, MAIN_WIFI_M2M_BUFFER_SIZE, 0);
			if (ret != SOCK_ERR_NO_ERROR) {
				bind_ntp_socket_state.status = FAILED;
			} else {
				bind_ntp_socket_state.status = COMPLETED;
			}
		} else {
			bind_ntp_socket_state.status = FAILED;
		}

		break;
	}
	
	case SOCKET_MSG_RECVFROM:
	{
		/* printf("socket_cb: socket_msg_recvfrom!\r\n"); */
		tstrSocketRecvMsg *pstrRx = (tstrSocketRecvMsg *)pvMsg;
		if (pstrRx->pu8Buffer && pstrRx->s16BufferSize) {
			
			uint8_t packetBuffer[48];
			memcpy(packetBuffer, pstrRx->pu8Buffer, sizeof(packetBuffer));

			if ((packetBuffer[0] & 0x7) != 4) {                   /* expect only server response */
				//printf("socket_cb: Expecting response from Server Only!\r\n");
				return;                    /* MODE is not server, abort */
			} else {
				uint32_t secsSince1900 = 0;
				secsSince1900 = (uint32_t)((uint32_t)packetBuffer[40] << 24) | (uint32_t)((uint32_t)packetBuffer[41] << 16) | (uint32_t)((uint32_t)packetBuffer[42] << 8) | (uint32_t)((uint32_t)packetBuffer[43]);
			
				/* Now convert NTP time into everyday time.
				 * Unix time starts on Jan 1 1970. In seconds, that's 2208988800.
				 * Subtract seventy years.
				 */
				epoch = (secsSince1900 - 2208988800UL);
				ntp_req_state.status = COMPLETED;
				
				LED_GREEN_set_level(false);
				LED_RED_set_level(true);
					
				ret = close(sock);
				if (ret == SOCK_ERR_NO_ERROR) {
					udp_socket = -1;
				}
			}
		}
	}
	break;
	
	/* Socket connected */
	case SOCKET_MSG_CONNECT:
	{
		tstrSocketConnectMsg *pstrConnect = (tstrSocketConnectMsg *)pvMsg;
		if (pstrConnect && pstrConnect->s8Error >= 0) {
			tls_connect_state.status = COMPLETED;
		} else {
			tls_connect_state.status = FAILED;
			close(sock);
		}
	}
	break;

	case SOCKET_MSG_SEND:
	{
		recv(tcp_client_socket, gau8SocketBuffer, sizeof(gau8SocketBuffer), 0);
	}
	break;

	/* Message receive */
	case SOCKET_MSG_RECV:
	{
		tstrSocketRecvMsg *pstrRecv = (tstrSocketRecvMsg *)pvMsg;
			if (pstrRecv && pstrRecv->s16BufferSize > 0) {
				MQTT_CLIENT_receive(pstrRecv->pu8Buffer, pstrRecv->s16BufferSize);
			} else {
				cloud_session_state.status = COMPLETED;
				
				scheduler_timeout_delete(&sendToCloudTimer);
				close(tcp_client_socket);
				tcp_client_socket = -1;
				
				mqtt_connection_state.status = FAILED;
		}
	} 
	break;
	
	default:
		break;
	}
}

void NETWORK_CONTROL_wifiHandler(uint8_t u8MsgType, void *pvMsg) {
	tstrM2MProvisionInfo *pstrProvInfo;
	
	switch (u8MsgType) {
		case M2M_WIFI_REQ_DHCP_CONF:
			wifi_connect_state.status = COMPLETED;
			break;
		case M2M_WIFI_RESP_PROVISION_INFO:
			pstrProvInfo = (tstrM2MProvisionInfo*)pvMsg;
			if (pstrProvInfo->u8Status == M2M_SUCCESS) {
				memcpy(ssid, pstrProvInfo->au8SSID, strlen((char*)(pstrProvInfo->au8SSID)));
				memcpy(pass, pstrProvInfo->au8Password, strlen((char*)(pstrProvInfo->au8Password)));
				itoa(pstrProvInfo->u8SecType, (char*)auth_type, 10);
				WIFI_provision = true;
				CREDENTIALS_STORAGE_save(ssid, pass, auth_type);
			}
			break;
		default:
			break;
	}
}

void NETWORK_CONTROL_dnsHandler(uint8* pu8DomainName, uint32 u32ServerIP) {
	if(u32ServerIP != 0) {
		mqttGoogleApisComIP = u32ServerIP;
		dns_req_state.status = COMPLETED;
	} else {
		dns_req_state.status = FAILED;
	}
}
