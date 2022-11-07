/**
 *
 * \file
 *
 * \brief Wifi NMI Iperf demo.
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

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
 INCLUDES
 *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

#ifdef IPERF_TEST

#include "Iperf.h"

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
 GLOBALS
 *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
extern uint8 gbIperfTestActive;
extern uint8 sendTCPpacket;
static uint8 gau8SocketTestBuffer[TEST_BUFFER_SIZE];

/* Statistics for received packets.
 */
static tstrSocketTestInfo gastrTestSocketInfo[MAX_SOCKET];
static uint8 gu8CurrentState = 0;
static uint8 gu8Retx = 0;
static uint32 gu32RemoteIPAddress = 0;
static uint32 gu8Num_bytes = 0;
/**
 */
uint32 gu32TestPacketCount = 0;

/* Sockets.
 */

SOCKET gUdpTxSocket = -1;
SOCKET UdpClientSocket = -1;
SOCKET UdpServerSocket = -1;
SOCKET TcpServerSocket = -1;
SOCKET TcpClientSocket = -1;
uint32 time = 0;
uint32 ms_ticks = 0;

struct iperf_tcp_client clients[CLIENT_COUNT] =
{
{ 0, }, };

/************************************************************************/
/*          Static functions                                            */
/************************************************************************/

sint8 write_UDP_FIN(SOCKET sock);
void IperfBufferInit(uint8* pu8SocketTestBuffer, int len);
void SetServerIpAddr(char* addr, uint32* u32RemoteIPAddress);

static sint32 check_digit(char c)
{
	return (c >= '0') && (c <= '9');
}
/*********************************************************************
 Function
 SocketSendTestPacket

 Description

 Send data using the connected UDP socket,
 until a termination flag is reached.
 Does not close the socket.
 
 Return
 None.

 Author
 Abderlrahman Osama Diab

 Version
 1.0

 Date

 *********************************************************************/
void SocketSendTestPacket(SOCKET sockID)
{
	if ((sockID >= 0) && (sockID < MAX_SOCKET))
	{
		sint8 rc = 0;
		if (gastrTestSocketInfo[sockID].bIsActive)
		{
			struct UDP_datagram* mBuf_UDP =
					(struct UDP_datagram*) gau8SocketTestBuffer;
			uint8 *pu8UdpTxBuffer = gau8SocketTestBuffer;
			//client_hdr	*pstrPktHdr = (client_hdr*)pu8UdpTxBuffer;

			/* Build the TEST Packet header.
			 */

			// store datagram ID into buffer 
			if (gu8CurrentState == TEST_STATE_UDP_TX)
				mBuf_UDP->id =
						_htonl( (uint32)gastrTestSocketInfo[sockID].u16TxSQN );

			rc = send(sockID, pu8UdpTxBuffer, TEST_BUFFER_SIZE, 0);

			if (rc < 0)
			{
				if (rc != -14)
					M2M_ERR("[%d] ERROR rc %d\n", ms_ticks, rc);
				gu8Retx = 1;
			}
			else
			{
				/* Update the TX SQN.
				 */
				//M2M_INFO("sending seq no = %d\r\n",gu32TestPacketCount);
				if (gu8CurrentState == TEST_STATE_UDP_TX)
					gu32TestPacketCount--;
				gastrTestSocketInfo[sockID].u16TxSQN++;
				gu8Retx = 0;
			}
		}
	}
}

/*********************************************************************
 Function
 UDP_StartClient

 Description

 Return
 Client Socket.

 Author
 Abderlrahman Osama Diab

 Version
 1.0

 Date

 *********************************************************************/

SOCKET UDP_StartClient(void)
{
	sint8 ret = M2M_SUCCESS;
	struct sockaddr_in addr;
	client_hdr* temp_hdr;
	UDP_datagram *UDPhdr = (UDP_datagram *) gau8SocketTestBuffer;
	static uint32 u8EnableCallbacks = 0;

	UdpClientSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (UdpClientSocket >= 0)
	{
		gastrTestSocketInfo[UdpClientSocket].bIsActive = 1;
		gastrTestSocketInfo[UdpClientSocket].u16TxSQN = 0;
		gastrTestSocketInfo[UdpClientSocket].u16RxSQN = 0;
		gastrTestSocketInfo[UdpClientSocket].u16Timeout = 0;

		addr.sin_family = AF_INET;
		addr.sin_port = _htons(IPERF_SERVER_PORT);
		addr.sin_addr.s_addr = gu32RemoteIPAddress;

		/* Build the Client Packet header.
		 */

		UDPhdr->id =
				_htonl( (uint32)gastrTestSocketInfo[UdpClientSocket].u16TxSQN );
		UDPhdr->tv_sec = 0; //_htonl( reportstruct->packetTime.tv_sec );
		UDPhdr->tv_usec = 0; //_htonl( reportstruct->packetTime.tv_usec );

		temp_hdr = (client_hdr*) (UDPhdr + 1);
		temp_hdr->flags = 0; //_htonl(HEADER_VERSION1);
		temp_hdr->bufferlen = 0; //_htonl(TEST_BUFFER_SIZE);
		temp_hdr->mWinBand = _htonl(UDP_RATE);
		temp_hdr->mPort = _htonl(IPERF_SERVER_PORT);
		temp_hdr->numThreads = _htonl(NUM_THREADS);
		temp_hdr->mAmount = 0; //_htonl(-(long)TEST_TIME);

		setsockopt(UdpClientSocket, SOL_SOCKET, SO_SET_UDP_SEND_CALLBACK,
				&u8EnableCallbacks, 0);

		ret = sendto(UdpClientSocket, gau8SocketTestBuffer, TEST_BUFFER_SIZE, 0,
				(struct sockaddr*) &addr, sizeof(addr));

		if (ret < 0)
		{
			M2M_ERR("ERROR Sock %d %d\n", UdpClientSocket, ret);
		}
		else
		{
			gastrTestSocketInfo[UdpClientSocket].u16TxSQN++;
			SocketSendTestPacket(UdpClientSocket);
		}
	}
	return UdpClientSocket;
}

/*********************************************************************
 Function
 TCP_StartClient

 Description

 Return
 Client Socket.

 Author
 Abderlrahman Osama Diab

 Version
 1.0

 Date

 *********************************************************************/

SOCKET TCP_StartClient(void)
{
	sint8 ret = M2M_SUCCESS;
	struct sockaddr_in addr;
	static uint32 u8EnableCallbacks = 1;

	TcpClientSocket = socket(AF_INET, SOCK_STREAM, 0);

	if (TcpClientSocket >= 0)
	{
		gastrTestSocketInfo[TcpClientSocket].bIsActive = 1;
		gastrTestSocketInfo[TcpClientSocket].u16TxSQN = 0;
		gastrTestSocketInfo[TcpClientSocket].u16RxSQN = 0;
		gastrTestSocketInfo[TcpClientSocket].u16Timeout = 0;

		addr.sin_family = AF_INET;
		addr.sin_port = _htons(IPERF_CLIENT_PORT);
		addr.sin_addr.s_addr = gu32RemoteIPAddress;

		/* Connect to Server.
		 */

		if ((ret = bind(TcpClientSocket, (struct sockaddr*) &addr,
				sizeof(struct sockaddr_in))) < 0)
			return ret;

		if ((ret = connect(TcpClientSocket, (struct sockaddr*) &addr,
				sizeof(struct sockaddr_in))) < 0)
			return ret;

		ret = setsockopt(TcpClientSocket, SOL_SOCKET, SO_SET_UDP_SEND_CALLBACK,
				&u8EnableCallbacks, 0);

		M2M_INFO("Preparation of test has been completed %d\r\n", ret);
	}

	return TcpClientSocket;
}

/*********************************************************************
 Function
 UDP_ClientSpawn

 Description

 Return
 None.

 Author
 Abderlrahman Osama Diab

 Version
 1.0

 Date

 *********************************************************************/
SOCKET UDP_ClientSpawn(void)
{
	/* Start Throughput Test. */
	gUdpTxSocket = UDP_StartClient();

	gu8CurrentState = TEST_STATE_UDP_TX;

	return gUdpTxSocket;
}

/*********************************************************************
 Function
 TCP_ClientSpawn

 Description

 Return
 None.

 Author
 Abderlrahman Osama Diab

 Version
 1.0

 Date

 *********************************************************************/
SOCKET TCP_ClientSpawn(void)
{
	/* Start Throughput Test. */
	TcpClientSocket = TCP_StartClient();

	gu8CurrentState = TEST_STATE_TCP_TX;

	return TcpClientSocket;
}

/*********************************************************************
 Function
 write_UDP_FIN

 Description

 Send a datagram on the socket. The datagram's contents should signify
 a FIN to the application. Keep re-transmitting until an
 acknowledgement datagram is received.
 Return
 None.

 Author
 Abderlrahman Osama Diab

 Version
 1.0

 Date

 *********************************************************************/
sint8 write_UDP_FIN(SOCKET sock)
{
	sint8 ret = M2M_SUCCESS;

	// write data
	struct UDP_datagram* mBuf_UDP = (struct UDP_datagram*) gau8SocketTestBuffer;
	uint8 *pu8UdpTxBuffer = gau8SocketTestBuffer;
	client_hdr *pstrPktHdr = (client_hdr*) pu8UdpTxBuffer;

	/* Build the TEST Packet header.		*/

	// store datagram ID into buffer 
	mBuf_UDP->id = _htonl( - (uint32)gastrTestSocketInfo[sock].u16TxSQN );
	mBuf_UDP->tv_sec = 0; //_htonl( reportstruct->packetTime.tv_sec );
	mBuf_UDP->tv_usec = 0; //_htonl( reportstruct->packetTime.tv_usec );

	pstrPktHdr->mAmount = 0;

	ret = send(sock, pu8UdpTxBuffer, TEST_BUFFER_SIZE, 0);
	return ret;

}

/*********************************************************************
 Function
 UDP_SocketEventHandler

 Description


 Return
 None.

 Author
 Ahmed Ezzat

 Version
 1.0

 Date

 *********************************************************************/

void UDP_IperfEventHandler(SOCKET sock, uint8 u8Msg, void * pvMsg)
{

	if (u8Msg == SOCKET_MSG_SENDTO && gbIperfTestActive)
	{
		if (gu32TestPacketCount == 0)
		{
			if (write_UDP_FIN(sock) == M2M_SUCCESS)
			{
				recvfrom(sock, gau8SocketTestBuffer, TEST_BUFFER_SIZE, 0);

				M2M_INFO("Iperf finished on: SOCKET   %02d    Next TX SQN = %04d\r\n", sock, gastrTestSocketInfo[sock].u16TxSQN);

				m2m_memset((uint8*) &gastrTestSocketInfo[sock], 0, sizeof(tstrSocketTestInfo));

				close(gUdpTxSocket);
				gUdpTxSocket = -1;
				gbIperfTestActive = 0;
				return;
			}
		}
		else if (sock == gUdpTxSocket)
		{
			SocketSendTestPacket(sock);
		}
	}

}

/*********************************************************************
 Function

 Description

 Return
 None.

 Author
 Ahmed Ezzat

 Version
 1.0

 Date

 *********************************************************************/
static NMI_API void TEST_APPSocketEventHandler(SOCKET sock, uint8 u8Msg,
		void * pvMsg)
{
	sint8 ret = -1;
	ms_ticks = app_os_timer_get_time();
	if (sock == UdpClientSocket)
	{
		if (u8Msg == SOCKET_MSG_SENDTO)
		{
			M2M_INFO("SOCKET_MSG_SENDTO\n");
		}

	}
	else if (sock == UdpServerSocket)
	{
		switch (u8Msg)
		{
		case SOCKET_MSG_BIND:
		{
			tstrSocketBindMsg *pstrBind = (tstrSocketBindMsg*) pvMsg;
			if (pstrBind != NULL)
			{
				if (pstrBind->status == 0)
				{
					M2M_INFO("Bind sucess\n");
					recvfrom(UdpServerSocket, gau8SocketTestBuffer,
							TEST_BUFFER_SIZE, 0);
				}
				else
				{
					M2M_ERR("bind\n");
				}
			}
		}
			break;
		case SOCKET_MSG_RECVFROM:
		{
			static int report = 1;
			static int outoforder = 0;
			uint32 id = 0;
			tstrSocketRecvMsg *pstrRx = (tstrSocketRecvMsg*) pvMsg;
			if (pstrRx->pu8Buffer != NULL)
			{
				UDP_datagram * UDPhdr = (UDP_datagram *) pstrRx->pu8Buffer;
				id = _ntohl(UDPhdr->id);

				if ((id & NBIT31) == NBIT31)
				{

					unsigned diff = 0;
					server_hdr* temp_hdr;
					UDP_datagram *UDPhdr = (UDP_datagram *) gau8SocketTestBuffer;

					UDPhdr->id = _htonl( (uint32)id);

					temp_hdr = (server_hdr*) (UDPhdr + 1);
					temp_hdr->flags = _htonl(0x80000000);
					temp_hdr->datagrams = _htonl(-id);
					temp_hdr->error_cnt = 0;
					temp_hdr->jitter1 = 0;
					temp_hdr->jitter2 = 0;
					temp_hdr->outorder_cnt = _htonl(outoforder);
					diff = (ms_ticks - time);
					temp_hdr->stop_sec = _htonl(diff/1000);
					temp_hdr->stop_usec = _htonl(diff%1000) * 1000;
					temp_hdr->total_len1 = 0;
					temp_hdr->total_len2 = _htonl(gu32TestPacketCount);

					ret = sendto(sock, (uint8*) gau8SocketTestBuffer,
							TEST_BUFFER_SIZE, 0,
							(struct sockaddr*) &pstrRx->strRemoteAddr,
							sizeof(struct sockaddr_in));
					if (ret != M2M_SUCCESS)
					{
						M2M_INFO("Fail %d\n", ret);
					}
					if (!report)
						M2M_INFO("end test payload %d byte time %d ms\n", gu32TestPacketCount, diff);
					report = 1;

				}
				else
				{
					if (report == 1)
					{
						time = 0;
						gu32TestPacketCount = 0;
						outoforder = 0;
						report = 0;
					}
					if (time == 0)
					{
						M2M_INFO("start app\n");
						time = ms_ticks;
					}
					if (pstrRx->s16BufferSize > 0)
					{
						gu32TestPacketCount += pstrRx->s16BufferSize;
					}
				}
			}
			else if (pstrRx->s16BufferSize <= 0)
			{
				M2M_INFO("udp server error %d\r\n", pstrRx->s16BufferSize);
			}
			recvfrom(UdpServerSocket, gau8SocketTestBuffer,
					sizeof(gau8SocketTestBuffer), 0);
		}
			break;
		}
	}
	else
	{
		if (gu8CurrentState == TEST_STATE_TCP_RX)
		{
			/* Find client from socket. */
			uint8 client_id;

			switch (u8Msg)
			{
			case SOCKET_MSG_ACCEPT:
			{
				tstrSocketAcceptMsg *msg = (tstrSocketAcceptMsg*) pvMsg;

				for (client_id = 0; client_id < CLIENT_COUNT; client_id++)
				{
					if (clients[client_id].used == 0)
						break;
				}

				if (client_id >= CLIENT_COUNT)
				{
					M2M_INFO("Has no space in client session\r\n");
					close(sock);
					break;
				}

				clients[client_id].sock = msg->sock;

				ret = recv(msg->sock, gau8SocketTestBuffer, TEST_BUFFER_SIZE,
						0);

				accept(TcpServerSocket, NULL, 0);
				M2M_INFO("New client has been connected\r\n");
			}
				break;
			case SOCKET_MSG_RECV:
			{
				tstrSocketRecvMsg *msg = (tstrSocketRecvMsg*) pvMsg;

				for (client_id = 0; client_id < CLIENT_COUNT; client_id++)
				{
					if (clients[client_id].sock == sock)
						break;
				}

				if (client_id >= CLIENT_COUNT)
					break;

				gu8Num_bytes += msg->s16BufferSize;

				if (msg->s16BufferSize > 0)
				{
					recv(sock, gau8SocketTestBuffer, TEST_BUFFER_SIZE, 0);
				}
				else if (msg->s16BufferSize <= 0)
				{
					M2M_INFO("Connection refused by peer %d\r\n", gu8Num_bytes);
				}
			}
				break;
			}
		}
		else if (gu8CurrentState == TEST_STATE_TCP_TX)
		{

			switch (u8Msg)
			{
			case SOCKET_MSG_CONNECT:
			{
				sint32 ret = 0;
				sendTCPpacket = 1;
				M2M_INFO("Sending packet\r\n");
				ret = send(TcpClientSocket, gau8SocketTestBuffer,
						TEST_BUFFER_SIZE, 0);

				if (ret < 0)
				{
					M2M_ERR("ERROR Sock %d %d\n", TcpClientSocket, ret);
				}
				else
				{
					gastrTestSocketInfo[TcpClientSocket].u16TxSQN++;

				}

			}
				break;

			case SOCKET_MSG_SEND:
			{
				sint16 s16SentBytes = *((sint16*) pvMsg);

				if (sock == TcpClientSocket)
				{
					gu32TestPacketCount--;

					if (s16SentBytes < 0)
					{
						M2M_INFO("Error %d: Iperf finished on: SOCKET   %02d    Next TX SQN = %04d\r\n", s16SentBytes, sock, gastrTestSocketInfo[0].u16TxSQN);

						m2m_memset((uint8*) &gastrTestSocketInfo[sock], 0,
								sizeof(tstrSocketTestInfo));

						TcpClientSocket = -1;
						gbIperfTestActive = 0;
						return;
					}
					else if (gu32TestPacketCount <= 0)
					{
						{
							M2M_INFO(
									"Iperf finished on: SOCKET   %02d    Next TX SQN = %04d\r\n", sock, gastrTestSocketInfo[0].u16TxSQN);


							m2m_memset((uint8*) &gastrTestSocketInfo[sock], 0,
									sizeof(tstrSocketTestInfo));

							close(sock);

							TcpClientSocket = -1;
							gbIperfTestActive = 0;
							return;
						}
					}
				}
			}
				break;
			}
		}
	}
}

/*********************************************************************
 Function
 IperfBufferInit
 Description
 Initialize the UDP buffer to the pattern used by Iperf
 Return
 None.

 Author
 Abderlrahman Osama Diab

 Version
 1.0

 Date

 *********************************************************************/
void IperfBufferInit(uint8* pu8SocketTestBuffer, int len)
{
	if (pu8SocketTestBuffer == NULL)
		return;

	while (len-- > 0)
	{
		pu8SocketTestBuffer[len] = (len % 10) + '0';
	}
}

/*********************************************************************
 Function
 SetServerIpAddr
 Description
 Initialize the Iperf UDP server IP address
 Return
 None.

 Author
 Abderlrahman Osama Diab

 Version
 1.0

 Date

 *********************************************************************/
void SetServerIpAddr(char* addr, uint32* u32RemoteIPAddress)
{
	uint8 value = 0;
	uint8 offset = 0;
	if (addr == NULL || u32RemoteIPAddress == NULL)
	{
		return;
	}

	*u32RemoteIPAddress = 0;

	while (*addr)
	{
		if (check_digit((char) *addr))
		{
			value *= 10;
			value += *addr - '0';
		}
		else
		{
			*u32RemoteIPAddress |= (value << offset);
			value = 0;
			offset += 8;
		}

		addr++;
	}

	if (offset == 24)
		*u32RemoteIPAddress |= (value << offset);

	M2M_PRINT(
			"Server IP Address %u.%u.%u.%u\r\n", (gu32RemoteIPAddress & 0x000000FF), ((gu32RemoteIPAddress >> 8) & 0x000000FF), ((gu32RemoteIPAddress >> 16) & 0x000000FF), ((gu32RemoteIPAddress >> 24) & 0x000000FF));
}

/*********************************************************************
 Function
 IperfUdpClientStart
 Description
 Initialize the UDP client
 Return
 Iperf client socket.

 Author
 Abderlrahman Osama Diab

 Version
 1.0

 Date

 *********************************************************************/
SOCKET IperfUdpClientStart(void)
{
	M2M_INFO("(APP) Starting Iperf Udp Client Throughput Test...\r\n");
	gu32TestPacketCount = IPERF_CLIENT_TX_PACKET_COUNT;

	SetServerIpAddr(SERVER_IP_ADDRESS, &(gu32RemoteIPAddress));

	/* Initialize Sockets */
	socketInit();

	registerSocketCallback(TEST_APPSocketEventHandler, NULL);
	IperfBufferInit(gau8SocketTestBuffer, TEST_BUFFER_SIZE);

	return UDP_ClientSpawn();
}

/*********************************************************************
 Function
 IperfUdpServerStart
 Description
 Initialize the UDP server
 Return
 Iperf client socket.

 Author
 Abderlrahman Osama Diab

 Version
 1.0

 Date

 *********************************************************************/
SOCKET IperfUdpServerStart(void)
{
	struct sockaddr_in addr_in;
	int ret;

	M2M_INFO("(APP) Starting Iperf Throughput Test...\r\n");
	M2M_INFO("Mode = Udp server\r\n");

	gu32TestPacketCount = 0;
	/* Initialize Sockets */
	socketInit();

	registerSocketCallback(TEST_APPSocketEventHandler, NULL);

	/*prepare server socket. */
	UdpServerSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (UdpServerSocket >= 0)
	{
		gu8CurrentState = TEST_STATE_UDP_RX;

		addr_in.sin_family = AF_INET;
		addr_in.sin_port = _htons(IPERF_SERVER_PORT); /*iperf default port is 5001 */
		addr_in.sin_addr.s_addr = 0; /* INADDR_ANY */

		if ((ret = bind(UdpServerSocket, (struct sockaddr*) &addr_in,
				sizeof(struct sockaddr_in))) < 0)
			return ret;

		M2M_INFO("Preparation of test has been completed succesfully\r\n");
	}

	return UdpServerSocket;
}
/*********************************************************************
 Function
 IperfTcpClientStart
 Description
 Initialize the TCP client
 Return
 Iperf client socket.

 Author
 Abderlrahman Osama Diab

 Version
 1.0

 Date

 *********************************************************************/
SOCKET IperfTcpClientStart(void)
{
	M2M_INFO("(APP) Starting Iperf Tcp Client Throughput Test...\r\n");
	gu32TestPacketCount = IPERF_CLIENT_TX_PACKET_COUNT;

	SetServerIpAddr(SERVER_IP_ADDRESS, &(gu32RemoteIPAddress));

	/* Initialize Sockets */
	socketInit();

	registerSocketCallback(TEST_APPSocketEventHandler, NULL);
	IperfBufferInit(gau8SocketTestBuffer, TEST_BUFFER_SIZE);

	return TCP_ClientSpawn();
}

/*********************************************************************
 Function
 IperfTcpServerStart
 Description
 Initialize the TCP server
 Return
 Iperf server socket.

 Author
 Abderlrahman Osama Diab

 Version
 1.0

 Date

 *********************************************************************/
SOCKET IperfTcpServerStart(void)
{

	struct sockaddr_in addr_in;
	int ret;

	M2M_INFO("(APP) Starting Iperf Tcp Server Throughput Test...\r\n");
	M2M_INFO("Mode = TCP server\r\n");

	/* Initialize Sockets */
	socketInit();

	registerSocketCallback(TEST_APPSocketEventHandler, NULL);

	/*prepare server socket. */
	TcpServerSocket = socket(AF_INET, SOCK_STREAM, 0);

	gu8CurrentState = TEST_STATE_TCP_RX;

	addr_in.sin_family = AF_INET;
	addr_in.sin_port = _htons(5001); /*iperf default port is 5001 */
	addr_in.sin_addr.s_addr = 0; /* INADDR_ANY */

	if ((ret = bind(TcpServerSocket, (struct sockaddr*) &addr_in,
			sizeof(struct sockaddr_in))) < 0)
		return ret;

	if ((ret = listen(TcpServerSocket, CLIENT_COUNT)) < 0)
		return ret;

	if ((ret = accept(TcpServerSocket, NULL, 0)) < 0)
		return ret;

	M2M_INFO("Preparation of test has been completed succesfully\r\n");

	return TcpServerSocket;
}

#endif
