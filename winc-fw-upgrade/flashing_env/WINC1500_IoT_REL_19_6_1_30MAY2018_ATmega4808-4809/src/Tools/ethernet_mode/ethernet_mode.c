#include "ethernet_mode.h"
#ifdef ETH_MODE
#undef UDP_TEST
#undef GROWL
#undef M2M_MGMT
#undef WIFI_DIRECT_TEST

 #define NBUFFER 5 
 #define BUFLEN 1000
 uint8 au8EthRcvBuffer[NBUFFER][BUFLEN];
 
 uint8 gau8ethRcvBuf[1500];
 uint8 gau8ChipIpAddr[4] ;
 uint8 gau8RemoteIpAddr[4];
 uint8 gau8RemoteMacAddr[6]={0} ;
 uint32 g32RcvdPingPktsCnt;
 uint8 	mac_addr[6];
 uint8 PingReceived = 0;
 uint32 inet_address(const char *pcIpAddr)
{
	uint8	tmp;
	uint32	u32IP = 0;
	uint8	au8IP[4];
	uint8 	c;
	uint8	i, j;

	tmp = 0;

	for(i = 0; i < 4; ++i) 
	{
		j = 0;
		do 
		{
			c = *pcIpAddr;
			++j;
			if(j > 4) 
			{
				return 0;
			}
			if(c == '.' || c == 0) 
			{
				au8IP[i] = tmp;
				tmp = 0;
			} 
			else if(c >= '0' && c <= '9') 
			{
				tmp = (tmp * 10) + (c - '0');
			} 
			else 
			{
				return 0;
			}
			++pcIpAddr;
		} while(c != '.' && c != 0);
	}
	m2m_memcpy((uint8*)&u32IP, au8IP, 4);
	return u32IP;
}
uint16 chksum(uint16 sum, const uint8 *data, uint16 len)
{
	uint16 t;
	const uint8 	*dataptr;
	const uint8 	*last_byte;
	uint32		u32Sum = sum;
	uint32		u32SumH = 0, u32SumL = 0;
	const uint8		*pu8Last = data + len - (len % 10);
	
	dataptr	= data;
	last_byte	= data + len - 1;

	if(len >= 10)
	{
		while(dataptr < pu8Last) 
		{	
			u32SumL += dataptr[1] + dataptr[3] + dataptr[5]  + dataptr[7] + dataptr[9];
			u32SumH += dataptr[0] + dataptr[2] + dataptr[4]  + dataptr[6] + dataptr[8];
			dataptr += 10;
		}
		u32Sum += u32SumL + (u32SumH << 8);
	}
	
	while(dataptr < last_byte) 
	{	
		t = (dataptr[0] << 8) + dataptr[1];
		u32Sum += t;
		dataptr += 2;
	}

	if(dataptr == last_byte) 
	{
		t = (dataptr[0] << 8) + 0;
		u32Sum += t;
	}
	sum = (uint16)u32Sum + (uint16)(u32Sum >> 16);
	return sum;
}
void send_arp_req(void)	
{	
	uint8 au8EthernePacket[42]={0} ; 
	M2M_INFO("\r SENDING ARP Request  to %s \n\n",PING_IP_ADRESS);
	m2m_memset(au8EthernePacket,0xFF,6);//BROADCAST message
	m2m_memcpy(&(au8EthernePacket[6]),mac_addr,6);
	au8EthernePacket[12]	= UIP_ETHTYPE_ARP>>8  ;
	au8EthernePacket[13]	= (UIP_ETHTYPE_ARP  & 0xFF);
	au8EthernePacket[14]	=  0x00 ; au8EthernePacket[15]	=  0x01;//Hardware Type Ethernet
	au8EthernePacket[16]	=  0x08; au8EthernePacket[17]	=  0x00;//prtotocol Type : IP
	au8EthernePacket[18]	=  0x06;//Hardware Size
	au8EthernePacket[19]	=  0x04;//Protocol Size
	au8EthernePacket[20]	=  0x00;au8EthernePacket[21]=  0x01;//OpCode :request
	m2m_memcpy(&(au8EthernePacket[22]),mac_addr,6); //Sender Mac address 
	m2m_memcpy(&(au8EthernePacket[28]),gau8ChipIpAddr,4); //Sender IP Address
	//m2m_memcpy(&(au8EthernePacket[32]),&(au8EthernePacket[0]),6);//Target Mac address
	m2m_memcpy(&(au8EthernePacket[38]),gau8RemoteIpAddr,4); //Target IP address
	m2m_wifi_send_ethernet_pkt(au8EthernePacket,42);
}

 void send_ping_req(uint32 u32PacketsCount)	
{	
	uint8 au8EthernePacket[74]={0} ; 
	uint16 u16ipChkSum =0;
	uint32 u32Count=0;
	static uint32 timeout = 0;
	g32RcvdPingPktsCnt=0;
	M2M_INFO("\r SENDING %u PING PACKETS to %s \n\n",u32PacketsCount,PING_IP_ADRESS);
	m2m_memcpy(au8EthernePacket,gau8RemoteMacAddr,6);
	m2m_memcpy(&(au8EthernePacket[6]),mac_addr,6);
	au8EthernePacket[12]	= UIP_ETHTYPE_IP>>8  ;
	au8EthernePacket[13]	= (uint8)UIP_ETHTYPE_IP  ;
	//////////////////////////////IP HEADER/////////////////////////////////////////
	au8EthernePacket[14]=0x45;//Version:IPV4 & IHL:Header length = 20 bytes (5*4 bytes per word)
	au8EthernePacket[15]=0x00;//DSCP:
	au8EthernePacket[16]=0x00;//Total Length: ICMP_HEADER_LENGTH +IP_HEADER_LENGTH_Size of Ping Data
	au8EthernePacket[17]=0x3c;//
	au8EthernePacket[18]=0xAB;//au8packet[ETH_HEADER_LENGTH+PKT_DATA_OFFSET+4];//ID (Set as user!)
	au8EthernePacket[19]=0xCD;//au8packet[ETH_HEADER_LENGTH+PKT_DATA_OFFSET+5];
	au8EthernePacket[20]=0x00;//FLAGS and part of FO
	au8EthernePacket[21]=0x00;//Fragment Offset
	au8EthernePacket[22]=0x80;//Time To Live
	au8EthernePacket[23]=0x01;//Protocol: 1(ICMP)
	au8EthernePacket[24]=0x00;//CHECKSUM NEEDS CALCULATIONS!
	au8EthernePacket[25]=0x00;
	m2m_memcpy(&(au8EthernePacket[26]),gau8ChipIpAddr,4); //Source IP Address
	m2m_memcpy(&(au8EthernePacket[30]),gau8RemoteIpAddr,4); //Destination IP address
	u16ipChkSum =~ chksum(0, &(au8EthernePacket[ETH_HEADER_LENGTH]), IP_HEADER_LENGTH);
	au8EthernePacket[24] = (uint8)(u16ipChkSum>>8);
	au8EthernePacket[25] = (uint8)u16ipChkSum;
	/////////////////////////////ICMP Header////////////////////////////////////////
	au8EthernePacket[34]= PING_REQ_TYPE ;//Type: Echo Reply
	au8EthernePacket[35]=0 ;//Code:0
	au8EthernePacket[36]=0xF7;//CheckSum
	au8EthernePacket[37]=0xFF; //
	/*the rest 4 Bytes of ICMP header are zeros*//*Fixed ID to have Fixed Cheksum*/
	/////////////////////////////ECHO Packet Data/////////////////////////////////////
	m2m_memset(&(au8EthernePacket[42]),0xFF,32);//Echo Pkt Data

	while (u32Count++ < u32PacketsCount)
		{	
			PingReceived=0;
			timeout=0;
			M2M_INFO("\r SENT PACKET NO %u  \n\n",u32Count);
			m2m_wifi_send_ethernet_pkt(au8EthernePacket,sizeof(au8EthernePacket));
			while ((PingReceived==0) && (timeout<TIMEOUT))
				{
				m2m_wifi_handle_events(NULL) ;
				timeout++;

				}
			if(timeout>=TIMEOUT) {M2M_INFO("\r TIMEOUT for Ping Packet NO %u  \n\n",u32Count);}
		}
		M2M_INFO("\r Recieved PING Reply (%.2f %%)  \n\n",(float)g32RcvdPingPktsCnt*100/PING_PACKETS_COUNT);

}
void ethernet_demo_init (void)
{
#ifdef CONF_STATIC_IP_ADDRESS 
	uint32 u32ip = inet_address(STATIC_IP_ADDRESS);
#else
	uint32 u32ip = inet_address("192.168.1.100");
#endif
	gau8ChipIpAddr[0]=(uint8)u32ip;
	gau8ChipIpAddr[1]=(uint8)(u32ip>>8);
	gau8ChipIpAddr[2]=(uint8)(u32ip>>16);
	gau8ChipIpAddr[3]=(uint8)(u32ip>>24);
	u32ip = inet_address(PING_IP_ADRESS);
	gau8RemoteIpAddr[0]=(uint8)u32ip;
	gau8RemoteIpAddr[1]=(uint8)(u32ip>>8);
	gau8RemoteIpAddr[2]=(uint8)(u32ip>>16);
	gau8RemoteIpAddr[3]=(uint8)(u32ip>>24);
	m2m_wifi_get_mac_address(mac_addr);
}

 void ethernet_demo_cb(uint8 u8MsgType,void * pvMsg,void * pvCtrlBf)
{
	int i = 0;
	static uint16 u16EthRxBuffInd = 0;
	if(u8MsgType == M2M_WIFI_RESP_ETHERNET_RX_PACKET)
	{
		
		uint8 	au8RemoteIpAddr[4];
		uint8       *au8packet = (uint8*)pvMsg;
		tstrM2mIpCtrlBuf     *PstrM2mIpCtrlBuf =(tstrM2mIpCtrlBuf *)pvCtrlBf;
		uint16 	u16ipChkSum =0;
		
		/********************ARP REQUEST***************************/
		if( (((au8packet[12]<<8)|(au8packet[13]))==UIP_ETHTYPE_ARP)&&(m2m_strncmp(au8packet,mac_addr,6)==0)/*Chip is destination*/
			&&(au8packet[21]==ARP_REQ_OPCODE))
		{
			M2M_INFO("\r Recieved ARP Request \n\n");
			m2m_memcpy(au8RemoteIpAddr,&(au8packet[28]),4); //extract sender IP first
			m2m_memcpy(au8packet,&(au8packet[6]),6);//Destination Mac address
			m2m_memcpy(&(au8packet[6]),mac_addr,6);//Source Mac address
			au8packet[14]	=  0x00 ; au8packet[15]	=  0x01;//Hardware Type Ethernet
			au8packet[16]	=  0x08; au8packet[17]	=  0x00;//prtotocol Type : IP
			au8packet[18]	=  0x06;//Hardware Size
			au8packet[19]	=  0x04;//Protocol Size
			au8packet[20]	=  0x00;au8packet[21]=  0x02;//OpCode :reply
			m2m_memcpy(&(au8packet[22]),mac_addr,6); //Sender Mac address 
			m2m_memcpy(&(au8packet[28]),gau8ChipIpAddr,4); //Sender IP Address
			m2m_memcpy(&(au8packet[32]),&(au8packet[0]),6);//Target Mac address
			m2m_memcpy(&(au8packet[38]),au8RemoteIpAddr,4); //Target IP address
			m2m_wifi_send_ethernet_pkt(au8packet,42);
		}
		/********************PING REQUEST***************************/
		else if ( (((au8packet[12]<<8)|(au8packet[13]))==UIP_ETHTYPE_IP)&&(m2m_strncmp(au8packet,mac_addr,6)==0)
			&&(au8packet[ETH_HEADER_LENGTH+IP_HEADER_LENGTH]==PING_REQ_TYPE))
		{
			M2M_INFO("\r Recieved PING Request \n\n");
			m2m_memcpy(au8RemoteIpAddr,&(au8packet[26]),4); //extract sender IP first
			/////////////////////////////ETHERNET HEADER///////////////////////////////////
			m2m_memcpy(au8packet,&(au8packet[6]),6);//Destination Mac address
			m2m_memcpy(&(au8packet[6]),mac_addr,6);//Source Mac address
			au8packet[12]	= UIP_ETHTYPE_IP>>8  ;
			au8packet[13]	= (uint8)UIP_ETHTYPE_IP  ;
			//////////////////////////////IP HEADER/////////////////////////////////////////
			au8packet[14]=0x45;//Version:IPV4 & IHL:Header length = 20 bytes (5*4 bytes per word)
			au8packet[15]=0x00;//DSCP:
			au8packet[16]=au8packet[ETH_HEADER_LENGTH+2];//Total Length of (IP Header +Data ) in bytes
			au8packet[17]=au8packet[ETH_HEADER_LENGTH+3];
			au8packet[18]=0xAB;//au8packet[ETH_HEADER_LENGTH+PKT_DATA_OFFSET+4];//ID (Set as user!)
			au8packet[19]=0xCD;//au8packet[ETH_HEADER_LENGTH+PKT_DATA_OFFSET+5];
			au8packet[20]=0x00;//FLAGS and part of FO
			au8packet[21]=0x00;//Fragment Offset
			au8packet[22]=0x80;//Time To Live
			au8packet[23]=0x01;//Protocol: 1(ICMP)
			au8packet[24]=0x00;au8packet[25]=0x00;//CHECKSUM NEEDS CALCULATIONS!	
			m2m_memcpy(&(au8packet[26]),gau8ChipIpAddr,4);//Source IP Address
			m2m_memcpy(&(au8packet[30]),au8RemoteIpAddr,4);//Destination IP address
			u16ipChkSum =~ chksum(0, &(au8packet[ETH_HEADER_LENGTH]), IP_HEADER_LENGTH);
			au8packet[24] = (uint8)(u16ipChkSum>>8); au8packet[25]	= (uint8)(u16ipChkSum);//CHECKSUM
			/////////////////////////////ICMP Header////////////////////////////////////////
			au8packet[34]= 0x00 ;//Type: Echo Reply
			au8packet[35]=0x00 ;//Code:0
			au8packet[36]= au8packet[ETH_HEADER_LENGTH+IP_HEADER_LENGTH+2]+0x08;//CheckSum
			au8packet[37]=au8packet[ETH_HEADER_LENGTH+IP_HEADER_LENGTH+3] ; //+1 for other case
			/////////////////////////////ECHO Packet Data/////////////////////////////////////
			m2m_memcpy(&(au8packet[42]),&(au8packet[ETH_HEADER_LENGTH+IP_HEADER_LENGTH+8]),PstrM2mIpCtrlBuf->u16DataSize-42);//Echo Pkt Data
			m2m_wifi_send_ethernet_pkt(au8packet,PstrM2mIpCtrlBuf->u16DataSize);
		}
		/********************ARP REPLY****************************/
		else if( (((au8packet[12]<<8)|(au8packet[13]))==UIP_ETHTYPE_ARP)&&(m2m_strncmp(au8packet,mac_addr,6)==0)/*Chip is destination*/
			&&(au8packet[21]==ARP_REPLY_OPCODE)&&(m2m_strncmp(&au8packet[28],gau8RemoteIpAddr,4)==0))
		{
			M2M_INFO("\r Recieved ARP Reply \n\n");
			m2m_memcpy(gau8RemoteMacAddr,&au8packet[22],6);//Extract mac address of the targetted station to be pinged
			send_ping_req(PING_PACKETS_COUNT);
		}
		/********************PING REPLY***************************/
		else if ( (((au8packet[12]<<8)|(au8packet[13]))==UIP_ETHTYPE_IP)
			&&(m2m_strncmp(au8packet,mac_addr,6)==0)
			&&(m2m_strncmp(&au8packet[ETH_HEADER_LENGTH+IP_HEADER_LENGTH-4],gau8ChipIpAddr,4)==0)
			&&(au8packet[ETH_HEADER_LENGTH+IP_HEADER_LENGTH]==PING_REPLY_TYPE)&&(PingReceived==0))
		{
			M2M_INFO("\r Recieved PING Reply \n\n");
			g32RcvdPingPktsCnt++;
			PingReceived=1;
		}

		/********************UDP PACKET ***************************/
		else if ((( (au8packet[12]<<8)|(au8packet[13]))==UIP_ETHTYPE_IP) && (au8packet[23]==0x11))
		{
			/* Put you code here         */
			/* used in multicast testing */
			M2M_DBG("Received Multicast frame\r\n");

		
		}

		m2m_memset(au8EthRcvBuffer[u16EthRxBuffInd],0, sizeof(au8EthRcvBuffer[0]));
		m2m_wifi_set_receive_buffer(au8EthRcvBuffer[u16EthRxBuffInd++], sizeof(au8EthRcvBuffer[0]));
		u16EthRxBuffInd = (u16EthRxBuffInd < NBUFFER)?u16EthRxBuffInd:0;


	}

}
#endif /* ETH_MODE */
