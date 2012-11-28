//Copyright 2012 <>< Charles Lohr, under the MIT-x11 or NewBSD license.  You choose.

#ifndef TCP_H
#define TCP_H

#include <stdint.h>
#include "enc424j600.h"
#include "iparpetc.h"

#ifdef INCLUDE_TCP

#define TCP_PROTOCOL_NUMBER 6
#define TCP_HEADER_LENGTH 20
#define TCP_AND_HEADER_SIZE (34 - 20)

//General processes:
//
//1: Request a connection to a foreign host.
//   a. Get a free socket.
//   b. Flag it as SYN_SENT
//   c. load local_port, dest_port, dest_addr with values.
//   d. load seq_num with a random value.
//   e. load ack_num will already be filled with 0.
//   f. set sendtype to be 0x01

#define TCP_HAS_SERVER

//USER MUST IMPLEMENT
//Must return a socket ID, or 0 if no action is to be taken, i.e. dump the socket.
uint8_t TCPReceiveSyn( uint16_t portno );

//Return 1 if you DID emit a packet (with ack) Return 0 if you JUST received data and we need to 
uint8_t TCPReceiveData( uint8_t connid, uint16_t totallen );

//User must provide this for being called back
//This is called right before the socket /must/ be closed.  Like if the remote host sent us a RST
void    TCPConnectionClosing( uint8_t conn );

//user must call this 100x/second
void TickTCP(); 

//user must init stack.
void InitTCP();

//Rest of API

void HandleTCP(uint16_t iptotallen);
int8_t GetFreeConnection();
void RequestClosure( uint8_t c );
void RequestConnect( uint8_t c );
uint8_t TCPCanSend( uint8_t c );

//always make sure 	enc28j60_finish_callback_now(); is already called. 
//Also, make sure waiting_for_ack is not set in socket before updating data.  This does not check for that.
//After calling this you can push packets.

//Initialize a TCP socket write
void StartTCPWrite( uint8_t c );
//End a TCP socket write. This computes checksum, length, etc. information
void EndTCPWrite( uint8_t c );
//this emits a stored and written TCP datagram
void EmitTCP( uint8_t );

typedef enum {
	CLOSED = 0,
	LISTEN,
	SYN_SENT,
	SYN_RECEIVED,
	ESTABLISHED,
	FIN_WAIT_1,
	FIN_WAIT_2,
	CLOSE_WAIT,
	LAST_ACK,
	TIME_WAIT,
} connection_state;

//Size is 36 bytes (try to keep this as low as possible)
struct tcpconnection
{
	connection_state state;
	uint16_t this_port;
	uint16_t dest_port;
	uint32_t dest_addr;
	uint32_t next_seq_num; //We use this to pre-compute the next sequence number.  We could theoretically calculate this on-the-fly? (try optimizing?)
	uint32_t seq_num; //The values on our /send to foreign/ - use this when we receive an ack.
	uint32_t ack_num; //The values on our /receive from foreign/
	uint8_t  remote_mac[6];

//For sending
	uint8_t time_since_sent; //if zero, then no packets are pending, if nonzero, it counts up.
	uint16_t idletime;
	uint8_t retries;
	uint8_t sendtype; //TCP Flags section

	//Send pointer?
	uint16_t sendptr; //On the enc424j600
	uint16_t sendlength;
};

//All sockets (including 0) are valid.
extern struct tcpconnection TCPs[TCP_SOCKETS];


#define ACKBIT _BV(4)
#define PSHBIT _BV(3)
#define RSTBIT _BV(2)
#define SYNBIT _BV(1)
#define FINBIT _BV(0)

#endif

#endif

