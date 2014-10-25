//Copyright 2012 Charles Lohr under the MIT/x11, newBSD, LGPL or GPL licenses.  You choose.

#ifndef _IPARPETC_H
#define _IPARPETC_H

#include <stdint.h>

#include "eth_config.h"
#ifdef INCLUDE_TCP
#ifdef AWFUL_TCP
#include "tcp_awful.h"
#else
#include "tcp.h"
#endif
#endif

#define IP_HEADER_LENGTH 20

//enc28j60 calls this.
void et_receivecallback( uint16_t packetlen );

//You must define these.
extern unsigned char MyIP[];
extern unsigned char MyMask[];
extern unsigned char MyMAC[];
extern unsigned char MyGateway[];

extern unsigned char macfrom[6];
extern unsigned char ipsource[4];
extern unsigned short remoteport;
extern unsigned short localport;

extern unsigned long icmp_in;
extern unsigned long icmp_out;

//Utility, for getting a new scratchpad.
uint16_t NetGetScratch();

//Utility out
void SwitchToBroadcast();
void send_etherlink_header( unsigned short type );
void send_ip_header( unsigned short totallen, const unsigned char * to, unsigned char proto );
void util_finish_udp_packet();

#define ipsource_uint ((uint32_t*)&ipsource)

#ifdef INCLUDE_TCP
void HandleTCP( uint16_t iptotallen );
#endif

void HandleUDP( uint16_t len );

#ifdef ARP_CLIENT_SUPPORT

//Returns -1 if arp not found yet.
//Otherwise returns entry into ARP table.

int8_t RequestARP( uint8_t * ip );

struct ARPEntry
{
	uint8_t mac[6];
	uint8_t ip[4];
};

extern uint8_t ClientArpTablePointer;
extern struct ARPEntry ClientArpTable[ARP_CLIENT_TABLE_SIZE];


#endif

#ifdef PING_CLIENT_SUPPORT

#ifndef ARP_CLIENT_SUPPORT
#error Client pinging requires ARP Client.
#endif

struct PINGEntries
{
	uint8_t ip[4];
	uint8_t id;  //If zero, not in use.
	uint16_t last_send_seqnum;
	uint16_t last_recv_seqnum;
};

extern struct PINGEntries ClientPingEntries[PING_RESPONSES_SIZE];

int8_t GetPingslot( uint8_t * ip );
void DoPing( uint8_t pingslot );


#endif

#ifdef ENABLE_DHCP_CLIENT

//NOTE: This cannot exceed 255
#ifndef DHCP_TICKS_PER_SECOND
#define DHCP_TICKS_PER_SECOND 10
#endif

extern uint8_t did_get_dhcp;
void SetupDHCPName( const char * name  );
void TickDHCP(); //Call this DHCP_TICKS_PER_SECOND times per second.

//If DHCP is enabled, you must write this function:
void GotDHCPLease();

#else
static inline void SetupDHCPName( const char * name  ) { }
static inline void TickDHCP() { }
#endif


#endif

/*
     Modification of "avrcraft" IP Stack.
    Copyright (C) 2014 <>< Charles Lohr
		CRC Code from: http://www.hackersdelight.org/hdcodetxt/crc.c.txt

    Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"),
	to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense,
	and/or sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
	in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
	CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
	TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
