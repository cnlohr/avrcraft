//Copyright 2012 Charles Lohr under the MIT/x11, newBSD, LGPL or GPL licenses.  You choose.

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "avr_print.h"
#include <stdio.h>
#include "iparpetc.h"
#include "enc424j600.h"
#include <alloca.h>
#include <string.h>


unsigned char macfrom[6];
unsigned char ipsource[4];
unsigned short remoteport;
unsigned short localport;
static unsigned short iptotallen;
static unsigned short i;  //For use within the scope of an in-file function, not out of that function.

#define POP enc424j600_pop8()
#define POP16 enc424j600_pop16()
#define POPB(x,s) enc424j600_popblob( x,s )

#define PUSH(x) enc424j600_push8(x)
#define PUSH16(x) enc424j600_push16(x)
#define PUSHB(x,s) enc424j600_pushblob(x,s)
#define SIZEOFICMP 28

#ifdef INCLUDE_TCP
#include "tcp.h"
#endif

void send_etherlink_header( unsigned short type )
{
	PUSHB( macfrom, 6 );

// The mac does this for us.
//	PUSHB( MyMAC, 6 );

	PUSH16( type );
}

void send_ip_header( unsigned short totallen, const unsigned char * to, unsigned char proto )
{
/*
	//This uses about 50 bytes less of flash, but 12 bytes more of ram.  You can choose that tradeoff.
	static unsigned char data[] = { 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 64, 0x00, 0x00, 0x00 };
	data[2] = totallen >> 8;
	data[3] = totallen & 0xFF;
	data[9] = proto;
	PUSHB( data, 12 );
*/

	PUSH16( 0x4500 );
	PUSH16( totallen );

	PUSH16( 0x0000 ); //ID
	PUSH16( 0x4000 ); //Don't frgment, no fragment offset

	PUSH( 64 ); // TTL
	PUSH( proto ); // protocol

	PUSH16( 0 ); //Checksum

	PUSHB( MyIP, 4 );
	PUSHB( to, 4 );
}


static void HandleICMP()
{
	unsigned short id;
	unsigned short seqnum;
	unsigned char type;
	unsigned short payloadsize = iptotallen - SIZEOFICMP;

	unsigned short payload_from_start, payload_dest_start;

	type = POP;
	POP; //code
	POP16; //Checksum

	switch( type )
	{
#ifdef PING_CLIENT_SUPPORT
	case 0: //Response
{
		uint16_t id;

		id = POP16;
		if( id < PING_RESPONSES_SIZE )
		{
			ClientPingEntries[id].last_recv_seqnum = POP16; //Seqnum
		}
		enc424j600_stopop();

		enc424j600_finish_callback_now();
}
		break;
#endif

	case 8: //ping request

		//Tricky: We would ordinarily POPB to read out the payload, but we're using
		//the DMA engine to copy that data.
		//	POPB( payload, payloadsize );
		//Suspend reading for now (but don't allow over-writing of the data)
		enc424j600_stopop();
		payload_from_start = enc424j600_read_ctrl_reg16( EERXRDPTL );

		enc424j600_startsend( 0 );
		send_etherlink_header( 0x0800 );
		send_ip_header( iptotallen, ipsource, 0x01 );

		PUSH16( 0 ); //ping reply + code
		PUSH16( 0 ); //Checksum
	//	PUSH16( id );
	//	PUSH16( seqnum );

		//Packet confiugred.  Need to copy payload.
		//Ordinarily, we'd PUSHB for the payload, but we're currently using the DMA engine for our work here.
		enc424j600_stopop();
		payload_dest_start = enc424j600_read_ctrl_reg16( EEGPWRPTL );

		//+4 = id + seqnum (we're DMAing that, too)
		enc424j600_copy_memory( payload_dest_start, payload_from_start, payloadsize + 4, RX_BUFFER_START, RX_BUFFER_END-1 );  
		enc424j600_write_ctrl_reg16( EEGPWRPTL, payload_dest_start + payloadsize + 4 );

		enc424j600_finish_callback_now();

		//Calculate header and ICMP checksums
		enc424j600_start_checksum( 8, 20 );
		unsigned short ppl = enc424j600_get_checksum();
		enc424j600_start_checksum( 28, payloadsize + 8 );
		enc424j600_alter_word( 18, ppl );
		ppl = enc424j600_get_checksum();
		enc424j600_alter_word( 30, ppl );

		enc424j600_endsend();

		break;
	}

}


static void HandleArp( )
{
	unsigned char sendermac_ip_and_targetmac[16];
//	unsigned char senderip[10]; //Actually sender ip + target mac, put in one to shrink code.

	unsigned short proto;
	unsigned char opcode;
//	unsigned char ipproto;

	POP16; //Hardware type
	proto = POP16;
	POP16; //hwsize, protosize
	opcode = POP16;  //XXX: This includes "code" as well, it seems.

	switch( opcode )
	{
	case 1:	//Request
{
		unsigned char match;

		POPB( sendermac_ip_and_targetmac, 16 );

		match = 1;
//sendhex2( 0xff );

		//Target IP (check for copy)
		for( i = 0; i < 4; i++ )
			if( POP != MyIP[i] )
				match = 0;

		if( match == 0 )
			return;

		//We must send a response, so we termiante the packet now.
		enc424j600_finish_callback_now();
		enc424j600_startsend( 0 );
		send_etherlink_header( 0x0806 );

		PUSH16( 0x0001 ); //Ethernet
		PUSH16( proto );  //Protocol
		PUSH16( 0x0604 ); //HW size, Proto size
		PUSH16( 0x0002 ); //Reply

		PUSHB( MyMAC, 6 );
		PUSHB( MyIP, 4 );
		PUSHB( sendermac_ip_and_targetmac, 10 ); // do not send target mac.

		enc424j600_endsend();

//		sendstr( "have match!\n" );

		break;
}
#ifdef ARP_CLIENT_SUPPORT
	case 2: //ARP Reply
{
		uint8_t sender_mac_and_ip_and_comp_mac[16];
		POPB( sender_mac_and_ip_and_comp_mac, 16 );
		enc424j600_finish_callback_now();


		//First, make sure that we're the ones who are supposed to receive the ARP.
		for( i = 0; i < 6; i++ )
		{
			if( sender_mac_and_ip_and_comp_mac[i+10] != MyMAC[i] )
				break;
		}

		if( i != 6 )
			break;

		//We're the right recipent.  Put it in the table.
		memcpy( &ClientArpTable[ClientArpTablePointer], sender_mac_and_ip_and_comp_mac, 10 );

		ClientArpTablePointer = (ClientArpTablePointer+1)%ARP_CLIENT_TABLE_SIZE;
}
#endif

	default:
		//???? don't know what to do.
		return;
	}
}

void enc424j600_receivecallback( uint16_t packetlen )
{
	unsigned char i;
	unsigned char ipproto;

	//First and foremost, make sure we have a big enough packet to work with.
	if( packetlen < 8 ) return;

	//macto (ignore) our mac filter handles this.
	enc424j600_dumpbytes( 6 );

	POPB( macfrom, 6 );

	//Make sure it's ethernet!
	if( POP != 0x08 )
	{
		return;
	}

	//Is it ARP?
	if( POP == 0x06 )
	{
		HandleArp( );
		return;
	}

	//otherwise it's standard IP

	//So, we're expecting a '45

	if( POP != 0x45 )
	{
//		sendstr( "CFH\n" );
		return;
	}


	POP; //differentiated services field.

	iptotallen = POP16;
	enc424j600_dumpbytes( 5 ); //ID, Offset+FLAGS+TTL
	ipproto = POP;

	POP16; //header checksum

	POPB( ipsource, 4 );

	for( i = 0; i < 4; i++ )
	{
		unsigned char m = ~MyMask[i];
		unsigned char ch = POP;
		if( ch == MyIP[i] || (ch & m) == 0xff  ) continue;
		return; 
	}

	//XXX TODO Handle IPL > 5  (IHL?)

	switch( ipproto )
	{
	case 1: //ICMP
		HandleICMP();
		break;
#ifdef INCLUDE_UDP
	case 17:
	{
		remoteport = POP16;
		localport = POP16;
		//err is this dangerous?
		HandleUDP( POP16 );
		break;	
	}
#endif
#ifdef INCLUDE_TCP
	case 6: // TCP
	{
		remoteport = POP16;
		localport = POP16;
		iptotallen-=20;
		HandleTCP( iptotallen );
		break;
	}
#endif // HAVE_TCP_SUPPORT
	default:
		enc424j600_finish_callback_now();
	}
}


#ifdef INCLUDE_UDP

void util_finish_udp_packet( )// unsigned short length )
{
	volatile unsigned short ppl, ppl2;

	//Packet loaded.
	enc424j600_stopop();

	unsigned short length = enc424j600_get_write_length();

	//Write length in packet
	enc424j600_alter_word( 10, length );
	enc424j600_start_checksum( 8, 20 );
	enc424j600_alter_word( 32, length-34 );

	ppl = enc424j600_get_checksum();
	enc424j600_alter_word( 18, ppl );

	//Addenudm for UDP checksum

	enc424j600_alter_word( 34, 0x11 + 0x8 + length-42 ); //UDP number + size + length (of packet)
		//XXX I THINK 

	enc424j600_start_checksum( 20, length - 42 + 16 ); //sta

	ppl2 = enc424j600_get_checksum();
	if( ppl2 == 0 ) ppl2 = 0xffff;

	enc424j600_alter_word( 34, ppl2 );

	enc424j600_endsend();
}


#endif


#ifdef ARP_CLIENT_SUPPORT

int8_t RequestARP( uint8_t * ip )
{
	uint8_t i;

	for( i = 0; i < ARP_CLIENT_TABLE_SIZE; i++ )
	{
		if( strncmp( (char*)&ClientArpTable[i].ip, ip, 4 ) == 0 ) //XXX did I mess up my casting?
		{
			return i;
		}
	}

	//Set the address we want to send to (broadcast)
	for( i = 0; i < 6; i++ )
		macfrom[i] = 0xff;

	//No MAC Found.  Send an ARP request.
	enc424j600_finish_callback_now();
	enc424j600_startsend( 0 );
	send_etherlink_header( 0x0806 );

	PUSH16( 0x0001 ); //Ethernet
	PUSH16( 0x0800 ); //Protocol (IP)
	PUSH16( 0x0604 ); //HW size, Proto size
	PUSH16( 0x0001 ); //Request

	PUSHB( MyMAC, 6 );
	PUSHB( MyIP, 4 );
	PUSH16( 0x0000 );
	PUSH16( 0x0000 );
	PUSH16( 0x0000 );
	PUSHB( ip, 4 );

	enc424j600_endsend();

	return -1;
}

struct ARPEntry ClientArpTable[ARP_CLIENT_TABLE_SIZE];
uint8_t ClientArpTablePointer = 0;

#endif

#ifdef PING_CLIENT_SUPPORT

struct PINGEntries ClientPingEntries[PING_RESPONSES_SIZE];

int8_t GetPingslot( uint8_t * ip )
{
	uint8_t i;
	for( i = 0; i < PING_RESPONSES_SIZE; i++ )
	{
		if( !ClientPingEntries[i].id ) break;
	}

	if( i == PING_RESPONSES_SIZE )
		return -1;

	memcpy( ClientPingEntries[i].ip, ip, 4 );
	return i;
}

void DoPing( uint8_t pingslot )
{
	unsigned short ppl;	
	uint16_t seqnum = ++ClientPingEntries[pingslot].last_send_seqnum;
	uint16_t checksum = (seqnum + pingslot + 0x0800) ;

	int8_t arpslot = RequestARP( ClientPingEntries[pingslot].ip );

	if( arpslot < 0 ) return;

	//must set macfrom to be the IP address of the target.
	memcpy( macfrom, ClientArpTable[arpslot].mac, 6 );

	enc424j600_startsend( 0 );
	send_etherlink_header( 0x0800 );
	send_ip_header( 32, ClientPingEntries[pingslot].ip, 0x01 );

	PUSH16( 0x0800 ); //ping request + 0 for code
	PUSH16( ~checksum ); //Checksum
	PUSH16( pingslot ); //Idneitifer
	PUSH16( seqnum ); //Sequence number

	PUSH16( 0x0000 ); //Payload
	PUSH16( 0x0000 );

	enc424j600_start_checksum( 8, 20 );
	ppl = enc424j600_get_checksum();
	enc424j600_alter_word( 18, ppl );

	enc424j600_endsend();
}

#endif


