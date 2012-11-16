//Copyright 2012 <>< Charles Lohr, under the MIT-x11 or NewBSD license.  You choose.
//
//Remaining TODO
//
//* TCP Keepalive.
//* Handle FIN correctly.

#include "tcp.h"
#include <stdint.h>
#include <string.h>
#include "avr_print.h"

#ifdef INCLUDE_TCP

struct tcpconnection TCPs[TCP_SOCKETS];

#define POP enc424j600_pop8()
#define POP16 enc424j600_pop16()
#define PUSH(x) enc424j600_push8(x)
#define PUSH16(x) enc424j600_push16(x)

//Consider putting these in the core stack.
static uint32_t POP32()
{
	uint32_t ret = ((uint32_t)POP)<<24;
	ret |= ((uint32_t)POP)<<16;
	ret |= ((uint32_t)POP)<<8;
	ret |= ((uint32_t)POP);
	return ret;
}

static void PUSH32( uint32_t o )
{
	PUSH( o >> 24 );
	PUSH( ( o >> 16 ) & 0xFF );
	PUSH( ( o >> 8 ) & 0xFF );
	PUSH( ( o ) & 0xFF );
}

static int8_t GetAssociatedPacket()
{
	int8_t ret, i;
	struct tcpconnection * t = &TCPs[0];

	for( i = 1; i < TCP_SOCKETS; i++ )
	{
		t++;
		if( t->state && t->this_port == localport && t->dest_port == remoteport && t->dest_addr == *((uint32_t*)&ipsource[0]) )
		{
			return i;
		}
	}
	return 0;
}

static void write_tcp_header( uint8_t c )
{
	struct tcpconnection * t = &TCPs[c];

	enc424j600_startsend( t->sendptr );
	send_etherlink_header( 0x0800 );
	send_ip_header( 0x00, (const uint8_t *)&t->dest_addr, TCP_PROTOCOL_NUMBER ); //Size, etc. will be filled into IP header later.
	PUSH16( t->this_port );
	PUSH16( t->dest_port );
	PUSH32( t->seq_num );
	PUSH32( t->ack_num );
	PUSH( (TCP_HEADER_LENGTH/4)<<4 ); //Data offset, reserved, NS flags all 0

	PUSH( t->sendtype );

//	PUSH16( (MAX_FRAMELEN-52) ); //window
	PUSH16( (8192) );

	PUSH16( 0x0000 ); //checksum

	PUSH16( 0x0000 ); //Urgent (no)
}

//////////////////////////EXTERNAL CALLABLE FUNCTIONS//////////////////////////

void InitTCP()
{
	//Eh?
}


void HandleTCP(uint16_t iptotallen)
{
	// ipsource is a global
	int rsck = GetAssociatedPacket();
	struct tcpconnection * t = &TCPs[rsck];

	uint32_t sequence_num = POP32();
	uint32_t ack_num = POP32();

	uint8_t hlen = (POP>>2) & 0xFC;
	uint8_t flags = POP;
	uint16_t window_size = POP16;

	POP16;  //Checksum
	POP16;  //Urgent PTR

	iptotallen -= hlen;

	//Clear out rest of header
	hlen -= 20;
	while( hlen-- )
		POP;

#ifdef TCP_HAS_SERVER
	if( flags & SYNBIT )	//SYN
	{
		struct tcpconnection * t;
		int8_t rsck = TCPReceiveSyn( localport );

		if( rsck == 0 ) goto reset_conn0;
		t = &TCPs[rsck];

		t->this_port = localport;
		t->dest_port = remoteport;
		t->dest_addr = *((uint32_t*)ipsource);
		t->ack_num = sequence_num + 1;
		t->seq_num = 0xAAAAAAAA; //should be random?
		t->state = SYN_RECEIVED;
		t->time_since_sent = 0;
		t->idletime = 0;

		//Don't forget to send a syn,ack back to the host.

		enc424j600_finish_callback_now();

		t->sendtype = SYNBIT | ACKBIT; //SYN+ACK
		StartTCPWrite( rsck );

		t->seq_num++;
		t->next_seq_num = t->seq_num;

		//The syn overrides everything, so we can return.
		goto end_and_emit;
	}
#endif

	//If we don't have a real connection, we're going to have to bail here.
	if( !rsck )
	{
		goto reset_conn0;
	}

	if( flags & RSTBIT)
	{
		TCPConnectionClosing( rsck );
		TCPs[rsck].sendtype = RSTBIT | ACKBIT;
		TCPs[rsck].state = 0;
		goto send_early;
	}

	//We have an associated connection on T.

	if( flags & ACKBIT ) //ACK
	{
		int16_t diff = ack_num - t->next_seq_num;// + iptotallen; // I don't know about this part)
//		printf( "%d / %d\n", ack_num, t->next_seq_num );

		//Lost a packet...
		if( diff < 0 )
		{
		}
		//XXX TODO THIS IS PROBABLY WRONG  (was <=, meaning it could throw away packets)
		else if( diff == 0 )
		{
			//t->seq_num = ack_num; //SEQ_NUM  This seems more like a bandaid.
			//The above line seems to be useful for bandaiding errors, when doing a more complete check, uncomment it.
			t->idletime = 0;
			t->time_since_sent = 0;

			t->seq_num = t->next_seq_num;
		}
	}

	//XXX TODO Handle FIN correctly.
	if( flags & FINBIT )
	{
		t->ack_num++;              //SEQ NUM
		t->sendtype = ACKBIT | FINBIT;
		t->state = 0;
		TCPConnectionClosing( rsck );
		goto send_early;
	}

	//XXX: TODO: Consider if time_since_sent is still set...
	//This is because if we have a packet that was lost, we can't plough over it with an ack.
	//If we do, the packet will be lost, and we will be out-of-sync.

	//TODO: How do we handle PDU's?  They are sent across multiple packets.  Should we supress ACKs until we have a full PDU?
	//if( flags & PSHBIT ) //PSH 
	if( iptotallen )
	{
		if( t->ack_num == sequence_num )
		{
			t->ack_num = sequence_num;   //XXX TODO remove this?
			if( iptotallen > 0 )
			{
				t->ack_num += iptotallen;
			}
			else
			{
				t->ack_num++; //Is this needed?
			}

			if( !TCPReceiveData( rsck, iptotallen ) )
			{
				if( t->time_since_sent )
				{
					//hacky, we need to send an ack, but we can't overwrite the existing packet.
					TCPs[0].seq_num = t->next_seq_num;//t->seq_num;
					TCPs[0].ack_num = t->ack_num;
					TCPs[0].sendtype = ACKBIT;
					rsck = 0;
					goto send_early;
				}
				else
				{
					t->sendtype = ACKBIT;
					goto send_early;
				}
			}
		}
		else
		{
			//Otherwise, discard packet.
		}
	}
	
	enc424j600_finish_callback_now();
	return;

reset_conn0:
	rsck = 0;
	TCPs[0].ack_num = 0;//sequence_num;
	TCPs[0].sendtype = RSTBIT;
	TCPs[0].state = 0;

send_early: //This requires a RST to be sent back.
	enc424j600_finish_callback_now();

	if( rsck == 0)
	{
		TCPs[0].dest_addr = *((uint32_t*)ipsource);
		TCPs[0].dest_port = remoteport;
		TCPs[0].this_port = localport;
	}
	write_tcp_header( rsck );
end_and_emit:
	EndTCPWrite( rsck, 0 );
	//EmitTCP( rsck );
}

int8_t GetFreeConnection()
{
	uint8_t i;
	for( i = 1; i < TCP_SOCKETS; i++ )
	{
		struct tcpconnection * t = &TCPs[i];
		if( !t->state )
		{
			memset( t, 0, sizeof( struct tcpconnection ) );
			t->sendptr = TX_SCRATCHPAD_END + TCP_BUFFERSIZE * (i-1);
			return i;
		}
	}
	return 0;
}

void TickTCP()
{
	uint8_t i;
	for( i = 0; i < TCP_SOCKETS; i++ )
	{
		struct tcpconnection * t = &TCPs[i];


		if( !t->state )
			continue;

		t->idletime++;

		//XXX: TODO: Handle timeout

		if( !t->time_since_sent )
			continue;

		t->time_since_sent++;

		if( t->time_since_sent > TCP_TICKS_BEFORE_RESEND )
		{
			StartTCPWrite( i );
			EndTCPWrite( i, 1 );  //Mark as re-transmit, this will update any of the needed fields on xmit.

			t->time_since_sent = 1;
			if( t->retries++ > TCP_MAX_RETRIES )
			{
				TCPConnectionClosing( i );
				t->state = 0;
			}
		}
	}
}

//XXX TODO This needs to be done better.
void RequestClosure( uint8_t c )
{
	if( !TCPs[c].state ) return;
	TCPs[c].sendtype = FINBIT;//RSTBIT;
//	TCPs[c].state = 0;
	TCPs[c].time_since_sent = 0;
//	TCPs[c].seq_num--; //???
	StartTCPWrite( c );
	EndTCPWrite( c, 0 );	
}


uint8_t TCPCanSend( uint8_t c )
{
	return !TCPs[c].time_since_sent;
}

void StartTCPWrite( uint8_t c )
{

	write_tcp_header( c );
}

void EndTCPWrite( uint8_t c, uint8_t is_retransmit )
{
	uint16_t ppl, ppl2;
	struct tcpconnection * t = &TCPs[c];
	unsigned short length;
	unsigned short payloadlen;
	unsigned short base = t->sendptr;

	//Done.  Load 'er up and send 'er out.
	//XXX TODO FIX THIS

	enc424j600_stopop();

	if( is_retransmit )
	{
		length = t->sendlength;
	}
	else
	{
		length = enc424j600_get_write_length();
		t->sendlength = length;
	}

	payloadlen = length - 34 - 20;

	if( !is_retransmit )
	{
		//Expecting an ACK.
		if( t->sendtype & ( PSHBIT | SYNBIT | FINBIT ) ) //PSH, SYN, RST or FIN packets
		{
			t->time_since_sent = 1;
			t->retries = 0;

			if( payloadlen == 0)
				t->next_seq_num = t->seq_num + 1;               //SEQ NUM
			else
				t->next_seq_num += payloadlen;   //SEQ NUM
		}
	}

	//Write length in IP header
	enc424j600_alter_word( 10, length - 14 );
	enc424j600_start_checksum( 8, 20 );
	ppl = enc424j600_get_checksum();
	enc424j600_alter_word( 18, ppl );

	//Calc TCP checksum

	//Initially, write pseudo-header into the checksum area
	enc424j600_alter_word( 0x2C, TCP_PROTOCOL_NUMBER + length - 28 - 6 );

	enc424j600_start_checksum( 20, length - 26 );
	ppl2 = enc424j600_get_checksum();
	enc424j600_alter_word( 0x2C, ppl2 );

	//XXX: TODO: Should there even be an IF statement here?  Shouldn't we be using xmitpacket?
	if( !is_retransmit )
	{
		enc424j600_endsend();
	}
	else
	{
		//We are re-transmitting so we have to hoke with the data
		 enc424j600_xmitpacket( t->sendptr, length );
	}
}

void EmitTCP( uint8_t c )
{
	struct tcpconnection * t = &TCPs[c];
	enc424j600_xmitpacket( t->sendptr, t->sendlength );
}


#endif
