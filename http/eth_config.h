//Copyright 2012 Charles Lohr under the MIT/x11, newBSD, LGPL or GPL licenses.  You choose.

#ifndef _ETH_CONFIG_H
#define _ETH_CONFIG_H

//Ethernet configuration file
//This controls what components are included, and how the chip is used.
//It expects an enc424j600 connected through GPIO pins.
//You can modify the send/receive SPI functions to use other interfaces, though.
//
//The core stack takes up ~2250 bytes.  This includes ARP and ICMP clients for
//address resolution and ping responses.
//
//with TCP+UDP, it takes ~4700 bytes.
//
//Other modules may be included at will.

#define ETPORT PORTC
#define ETPIN  PINC
#define ETDDR  DDRC

#define ETCSPORT PORTC
#define ETCSDDR  DDRC

#define ETCSP   5
#define ETINTP  8 //disabled.
#define ETSOP   4
#define ETSIP   3
#define ETSCKP  2   


#define ETCS	_BV(ETCSP)
#define ETINT   _BV(ETINTP)
#define ETSO	_BV(ETSOP)
#define ETSI	_BV(ETSIP)
#define ETSCK	_BV(ETSCKP)

//Select one or neither...  Neither will use small and slow (2x speed)
//extra 150 bytes...
//#define FAST_SPI
#define ASM_SPI

//It's the same size, faster, but not thread safe. Used in conjunction with ASM_SPI
//NOTE: This does not work on the ATMega168 for some reason, even at lower speeds?
//NOTE: This will not work at 28 MHz 
#define ASM_REALLY_FAST_SPI

//Minimum MTU every host must be able to handle; 
#define MAX_FRAMELEN     578

//Do this to disable printf's and save space
//#define MUTE_PRINTF

//UDP is pretty well tested.  It takes up ~350 bytes
//If you use UDP, you will need to implement the following function:
//  void HandleUDP( uint16_t len ) { ... }
//you will have access to the global variables:
//  localport, remoteport, and ipsource
//
//#define INCLUDE_UDP

//TCP is still in alpha.  It is still a little buggy.  It takes up ~2200 bytes
//You must call:
// 	InitTCP() at start
//  TickTCP() at ~100x/sec (USE INSIDE INTERRUPTS IS PROBABLY DANGEROUS)
//You must provide:
//  uint8_t TCPReceiveSyn( uint16_t portno )  (See demo)
//  void TCPConnectionClosing( uint8_t conn ) (called whenever TCP connections are closed)
//  uint8_t TCPReceiveData( uint8_t connection, uint16_t totallen )
//
#define INCLUDE_TCP

//Note: TCP Buffers are placed immediately after the scratchpad.
//Send buffer size for TCP packet
#define TCP_BUFFERSIZE (MAX_FRAMELEN + 42)
//Where to start allocating sockets in the enc's buffer ram.
//Allow a working area of 1600.
//
//NOTE: Packet #0 is a reserved packet, it is used for
//temporary actions
#define TCP_SOCKETS 10
#define TCP_MAX_RETRIES 40

//500 ms (cannot be more than 254 (or 2.54s))
#define TCP_TICKS_BEFORE_RESEND 50


//Additional applications
#define INCLUDE_HTTP_SERVER
#define HTTP_SERVER_TIMEOUT (10000) 
#define HTTP_CONNECTIONS 6
//#define HTTP_USE_MEMORY_FS


//Scratchpad for sending out packets like UDP, ICMP, ARP, etc.
#define TX_SCRATCHPAD_END  1024

#define RX_BUFFER_SIZE   (3264)
#define RX_BUFFER_END    0x5FFF

//Memory configuration
//The ENC424j600 has 0x6000 (24kB) bytes of memory
//We have to make good use of it.
// 0x0000
//  [Scratchpad]
// 0x0400
//  [TCP packets (578+42)*TCP_SOCKETS
// 0x1b84 (assuming 10 sockets)
//  [unused area]
// 0x5340 (RX_BUFFER_START (0x6000-RX_BUFFER_SIZE))
//  [RX Buffer]
// 0x6000 (End of standard SRAM)

#ifdef INCLUDE_TCP
#define FREE_ENC_START (TX_SCRATCHPAD_END + (TCP_SOCKETS * TCP_BUFFERSIZE) + 2)
#else
#define FREE_ENC_START (TX_SCRATCHPAD_END + 2)
#endif

#define FREE_ENC_END (RX_BUFFER_START-2)


#ifndef ASSEMBLY

extern unsigned char MyIP[4];
extern unsigned char MyMask[4];
extern unsigned char MyMAC[6];

#endif

#endif

