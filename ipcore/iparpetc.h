//Copyright 2012 Charles Lohr under the MIT/x11, newBSD, LGPL or GPL licenses.  You choose.

#ifndef _IPARPETC_H
#define _IPARPETC_H

#include <stdint.h>
#ifdef INCLUDE_TCP
#include "tcp.h"
#endif

#define IP_HEADER_LENGTH 20

//enc28j60 calls this.
void enc28j60_receivecallback( uint16_t packetlen );

//You must define these.
extern unsigned char MyIP[];
extern unsigned char MyMask[];
extern unsigned char MyMAC[];


extern unsigned char macfrom[6];
extern unsigned char ipsource[4];
extern unsigned short remoteport;
extern unsigned short localport;

//Utility out
void send_etherlink_header( unsigned short type );
void send_ip_header( unsigned short totallen, const unsigned char * to, unsigned char proto );

void util_finish_udp_packet();

#define HAVE_UDP_SUPPORT
#define HAVE_TCP_SUPPORT

#define ipsource_uint ((uint32_t*)&ipsource)

#ifdef INCLUDE_TCP
void HandleTCP( uint16_t iptotallen );
#endif

void HandleUDP( uint16_t len );

#endif

