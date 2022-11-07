/**
 *
 * \file
 *
 * \brief Wifi NMI temperature sensor demo.
 *
 * Copyright (c) 2014 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

#ifndef IPERF_H_
#define IPERF_H_

#ifdef IPERF_TEST


#include "bsp/include/nm_bsp.h"
#include "driver/include/m2m_wifi.h"
#include "socket/include/socket.h"

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
MACROS
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

#define TEST_CMD_TCP_TX_START		0x80
#define TEST_CMD_TCP_RX_START		0x81

#define TEST_STATE_UDP_TX			1
#define TEST_STATE_UDP_RX			2
#define TEST_STATE_TCP_TX			3
#define TEST_STATE_TCP_RX			4

#define TEST_BUFFER_SIZE					1400
#define IPERF_CLIENT_TX_PACKET_COUNT		10000
//#define SERVER_IP_ADDRESS					"192.168.0.112"
#define SERVER_IP_ADDRESS					"192.168.43.1"
#define HEADER_VERSION1						0x80000000
#define UDP_RATE							40 * 1024 * 1024 // rate = 40 Mega
#define IPERF_SERVER_PORT					5001
#define IPERF_CLIENT_PORT					5001
#define NUM_THREADS							1
#define TEST_TIME							1000 // 10 sec


#define CLIENT_COUNT				4


struct iperf_tcp_client{
	char used;
	SOCKET sock;
	char buffer[TEST_BUFFER_SIZE];
};

extern uint32 				ms_ticks;
extern uint32				gu32TestPacketCount;
uint8 sendTCPpacket;

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
DATA TYPES
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

// used to reference the 4 byte ID number Iperf place in UDP datagrams
typedef struct UDP_datagram 
{
    uint32 id;
    uint32 tv_sec;
    uint32 tv_usec;

} UDP_datagram;

/*
 * The client_hdr structure is sent from clients
 * to servers to alert them of things that need
 * to happen. Order must be perserved in all 
 * future releases for backward compatibility.
 * 1.7 has flags, numThreads, mPort, and bufferlen
 */
typedef struct client_hdr {

    /*
     * flags is a bitmap for different options
     * the most significant bits are for determining
     * which information is available. So 1.7 uses
     * 0x80000000 and the next time information is added
     * the 1.7 bit will be set and 0x40000000 will be
     * set signifying additional information. If no 
     * information bits are set then the header is ignored.
     * The lowest order diferentiates between dualtest and
     * tradeoff modes, wheither the speaker needs to start 
     * immediately or after the audience finishes.
     */
    sint32 flags;
    sint32 numThreads;
    sint32 mPort;
    sint32 bufferlen;
    sint32 mWinBand;
    sint32 mAmount;
} client_hdr;

/*
 * The server_hdr structure facilitates the server
 * report of jitter and loss on the client side.
 * It piggy_backs on the existing clear to close
 * packet.
 */
typedef struct server_hdr {
    /*
     * flags is a bitmap for different options
     * the most significant bits are for determining
     * which information is available. So 1.7 uses
     * 0x80000000 and the next time information is added
     * the 1.7 bit will be set and 0x40000000 will be
     * set signifying additional information. If no 
     * information bits are set then the header is ignored.
     */
    sint32 flags;
    sint32 total_len1;
    sint32 total_len2;
    sint32 stop_sec;
    sint32 stop_usec;
    sint32 error_cnt;
    sint32 outorder_cnt;
    sint32 datagrams;
    sint32 jitter1;
    sint32 jitter2;

} server_hdr;

typedef struct{
	uint8		bIsActive;
	uint8		u8Retries;
	uint16		u16TxSQN;
	uint16		u16RxSQN;
	uint16		u16Timeout;
}tstrSocketTestInfo;


/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
FUNCTION PROTOTYPES
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/


NMI_API SOCKET UDP_ClientSpawn(void);

NMI_API SOCKET TCP_ClientSpawn(void);

NMI_API void TCP_SocketEventHandler(SOCKET sock, uint8 u8Msg, void * pvMsg);

NMI_API void UDP_IperfEventHandler(SOCKET sock, uint8 u8Msg, void * pvMsg);

NMI_API void SetRemoteIPAddress(uint32 u32IP);

NMI_API SOCKET TCP_StartClient(void);

NMI_API SOCKET UDP_StartClient(void);

NMI_API SOCKET UDP_StartServer(void);

NMI_API void SocketSendTestPacket(SOCKET sockID);

NMI_API void UDP_StartThroughputTest(void);

NMI_API void SocketSendCtrlCMD(uint32 u32Cmd, uint32 u32Arg);

NMI_API SOCKET IperfUdpClientStart(void);

NMI_API SOCKET IperfTcpClientStart(void);

NMI_API SOCKET IperfTcpServerStart(void);

NMI_API SOCKET IperfUdpServerStart(void);

#endif /* IPERF_TEST */

#endif /* IPERF_H_ */
