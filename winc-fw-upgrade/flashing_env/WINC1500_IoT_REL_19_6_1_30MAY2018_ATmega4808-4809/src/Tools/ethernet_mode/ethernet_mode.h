#ifndef __ETHERNET_MODE__
#define __ETHERNET_MODE__
#include "m2m_test_config.h"

#define UIP_ETHTYPE_ARP 0x0806
#define ARP_REQ_OPCODE 0x01
#define ARP_REPLY_OPCODE 0x02
#define UIP_ETHTYPE_IP 0x0800
#define IP_HEADER_LENGTH 20
#define ICMP_HEADER_LENGTH 8
#define ETH_HEADER_LENGTH 14
#define PING_REQ_TYPE 0x08
#define PING_REPLY_TYPE 0x00
#define PING_PACKETS_COUNT 1000
#define TIMEOUT 100000
#define PING_IP_ADRESS "192.168.1.1"

 extern uint8 gau8ethRcvBuf[1500];
extern  uint8 gau8ChipIpAddr[4] ;
 extern uint8 gau8RemoteIpAddr[4];
extern  uint8 gau8RemoteMacAddr[6];
extern  uint32 g32RcvdPingPktsCnt;

 extern void send_arp_req(void);
 extern void send_ping_req(uint32 u32PacketsCount);
 extern uint32 inet_address(const char *pcIpAddr);
 extern uint16 chksum(uint16 sum, const uint8 *data, uint16 len);
 extern void ethernet_demo_cb(uint8 u8MsgType,void * pvMsg,void * pvCtrlBf);
 extern void ethernet_demo_init (void);

 #endif
