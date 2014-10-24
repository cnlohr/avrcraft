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
#include <avr/pgmspace.h>

unsigned long icmp_in = 0;
unsigned long icmp_out = 0;

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

uint16_t NetGetScratch()
{
	static uint8_t scratch = 0;
	scratch++;
	if( scratch == TX_SCRATCHES ) scratch = 0;
	return scratch * MAX_FRAMELEN;
}


#ifdef ENABLE_DHCP_CLIENT

#ifndef INCLUDE_UDP
#error ERROR: You must have UDP support for DHCP support.
#endif

void RequestNewIP( uint8_t mode, uint8_t * negotiating_ip, uint8_t * dhcp_server );

uint8_t dhcp_clocking = 1;
uint16_t dhcp_seconds_remain = 0;
uint8_t dhcp_ticks_in_sec = 0;
const char * DHCPName = 0;

void HandleDHCP( uint16_t len )
{
	uint8_t tmpip[4];
	uint8_t tmp[4];
	uint8_t optionsleft = 48; //We only allow for up to 48 options
	uint8_t is_ack_packet = 0;
	uint16_t first4, second4;

	POP16; //Clear out checksum

	//Process DHCP!
	if( POP != 2 ) return; //Not a reply?
	if( POP != 1 ) return; //Not Ethernet?
	POP16; //size of packets + Hops

	//Make sure transaction IDs match.
	enc424j600_popblob( tmp, 4 );
	if( memcmp( tmp, MyMAC, 4 ) != 0 )
	{
		//Not our request?
		return;
	}

	enc424j600_dumpbytes( 8 ); //time elapsed + bootpflags + Client IP address

	enc424j600_popblob( tmpip, 4 );	//MY IP ADDRESS!!!	
	
	enc424j600_dumpbytes( 0x18 + 0xc0 ); //Next IP + Relay IP + Mac + Padding + server name + boot name

	first4 = POP16;
	second4 = POP16;

	if( first4 != 0x6382 || second4 != 0x5363 )
	{
		return;
	}

	//Ok, we know we have a valid DHCP packet.

	//We dont want to get stuck, so we will eventually bail if we have an issue pasrsing.
	while( optionsleft-- )
	{
		uint8_t option = POP;
		uint8_t length = POP;

		switch(option)
		{
		case 0x35: //DHCP Message Type
		{
			if( length < 1 ) return;
			uint8_t rqt = POP;

			if( rqt == 0x02 ) 
			{
				//We have an offer, extend a request.
				//We will ignore the rest of the packet.
				enc424j600_finish_callback_now();
				RequestNewIP( 3, tmpip, ipsource );  //Request
				return;
			}
			else if( rqt == 0x05 ) // We received an ack, accept it.
			{
				//IP Is valid.
				is_ack_packet = 1;
				if( 0 == dhcp_seconds_remain )
					dhcp_seconds_remain = 0xffff;
				memcpy( MyIP, tmpip, 4 );
			}

			length--;
			break;
		}
		case 0x3a: //Renewal time
		{
			if( length < 4 || !is_ack_packet ) break;
			first4 = POP16;
			second4 = POP16;
//			printf( "LEASE TIME: %d %d\n", first4, second4 );
			if( first4 )
			{
				dhcp_seconds_remain = 0xffff;
			}
			else
			{
				dhcp_seconds_remain = second4;
			}

			length -= 4;
			break;
		}
		case 0x01: //Subnet mask
		{
			if( length < 4 || !is_ack_packet ) break;
			enc424j600_popblob( MyMask, 4 );
			length -= 4;
			break;
		}
		case 0x03: //Router mask
		{
			if( length < 4 || !is_ack_packet ) break;
			enc424j600_popblob( MyGateway, 4 );
			length -= 4;
			break;
		}
		case 0xff:  //End of list.
			enc424j600_finish_callback_now();
			if( is_ack_packet )
				GotDHCPLease();
			return; 
		case 0x42: //Time server
		case 0x06: //DNS server
		default:
			break;
		}
		enc424j600_dumpbytes( length );
	}
}

void SetupDHCPName( const char * name  )
{
	DHCPName = name;
}

void TickDHCP()
{
	if( dhcp_ticks_in_sec++ < DHCP_TICKS_PER_SECOND )
	{
		return;
	}

	dhcp_ticks_in_sec = 0;
//		sendhex4( dhcp_seconds_remain );

	if( dhcp_seconds_remain == 0 )
	{
		//No DHCP received yet.
		if( dhcp_clocking == 250 ) dhcp_clocking = 0;
		dhcp_clocking++;
		RequestNewIP( 1, MyIP, 0 );
	}
	else
	{
		dhcp_seconds_remain--;
	}
}

//Mode = 1 for discover, Mode = 3 for request - if discover, dhcp_server should be 0.
void RequestNewIP( uint8_t mode, uint8_t * negotiating_ip, uint8_t * dhcp_server )
{
	uint8_t oldip[4];
	SwitchToBroadcast();

	enc424j600_stopop();
	enc424j600_startsend( NetGetScratch() );
	send_etherlink_header( 0x0800 );

	//Tricky - backup our IP - we want to spoof it to 0.0.0.0
	memcpy( oldip, MyIP, 4 );
	MyIP[0] = 0; MyIP[1] = 0; MyIP[2] = 0; MyIP[3] = 0;
	send_ip_header( 0, "\xff\xff\xff\xff", 17 ); //UDP Packet to 255.255.255.255
	memcpy( MyIP, oldip, 4 );
	
/*
	enc424j600_push16( 68 );  //From bootpc
	enc424j600_push16( 67 );  //To bootps
	enc424j600_push16( 0 ); //length for later
	enc424j600_push16( 0 ); //csum for later

	//Payload of BOOTP packet.
	//	1, //Bootp request
	//	1, //Ethernet
	enc424j600_push16( 0x0101 );
	//	6, //MAC Length
	//	0, //Hops
	enc424j600_push16( 0x0600 );
*/
	enc424j600_pushpgmblob( PSTR("\x00\x44\x00\x43\x00\x00\x00\x00\x01\x01\x06"), 12 ); //NOTE: Last digit is 0 on wire, not included in string.

	enc424j600_pushblob( MyMAC, 4 );

	enc424j600_push16( dhcp_clocking ); //seconds
	enc424j600_pushzeroes( 10 ); //unicast, CLIADDR, YIADDR
	if( dhcp_server )
		enc424j600_pushblob( dhcp_server, 4 );
	else
		enc424j600_pushzeroes( 4 );
	enc424j600_pushzeroes( 4 ); //GIADDR IP
	enc424j600_pushblob( MyMAC, 6 ); //client mac
	enc424j600_pushzeroes( 10 + 0x40 + 0x80 ); //padding + Server Host Name
	enc424j600_push16( 0x6382 ); //DHCP Magic Cookie
	enc424j600_push16( 0x5363 );

	//Now we are on our DHCP portion
	enc424j600_push8( 0x35 );  //DHCP Message Type
	enc424j600_push16( 0x0100 | mode );

	{
		enc424j600_push16( 0x3204 ); //Requested IP address. (4 length)
		enc424j600_pushblob( negotiating_ip, 4 );
	}

	if( dhcp_server ) //request
	{
		enc424j600_push16( 0x3604 );
		enc424j600_pushblob( dhcp_server, 4 );
	}

	if( DHCPName )
	{
		uint8_t namelen = strlen( DHCPName );
		enc424j600_push8( 0x0c ); //Name
		enc424j600_push8( namelen );
		enc424j600_pushblob( DHCPName, namelen );
	}

	enc424j600_push16( 0x3702 ); //Parameter request list
	enc424j600_push16( 0x0103 ); //subnet mask, router
//	enc424j600_push16( 0x2a06 ); //NTP server, DNS server  (We don't use either NTP or DNS)
	enc424j600_push8( 0xff ); //End option

	enc424j600_pushzeroes( 32 ); //Padding

	util_finish_udp_packet();
}

#endif


void send_etherlink_header( unsigned short type )
{
	PUSHB( macfrom, 6 );

// The mac does this for us.
	PUSHB( MyMAC, 6 );

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

	icmp_in++;

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

		enc424j600_startsend( NetGetScratch() );
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
		enc424j600_start_checksum( 8+6, 20 );
		unsigned short ppl = enc424j600_get_checksum();
		enc424j600_start_checksum( 28+6, payloadsize + 8 );
		enc424j600_alter_word( 18+6, ppl );
		ppl = enc424j600_get_checksum();
		enc424j600_alter_word( 30+6, ppl );

		enc424j600_endsend();
		icmp_out++;

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
		enc424j600_startsend( NetGetScratch() );
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
	uint8_t is_the_packet_for_me = 1;
	unsigned char i;
	unsigned char ipproto;

	//First and foremost, make sure we have a big enough packet to work with.
	if( packetlen < 8 )
	{
#ifdef ETH_DEBUG
		sendstr( "Runt\n" );
#endif
		return;
	}

	//macto (ignore) our mac filter handles this.
	enc424j600_dumpbytes( 6 );

	POPB( macfrom, 6 );

	//Make sure it's ethernet!
	if( POP != 0x08 )
	{
#ifdef ETH_DEBUG
		sendstr( "Not ethernet.\n" );
#endif
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
#ifdef ETH_DEBUG
		sendstr( "Not IP.\n" );
#endif
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
		is_the_packet_for_me = 0;
	}

	//Tricky, for DHCP packets, we have to detect it even if it is not to us.
	if( ipproto == 17 )
	{
		remoteport = POP16;
		localport = POP16;
#ifdef ENABLE_DHCP_CLIENT
		if( localport == 68 && !dhcp_seconds_remain )
		{
			HandleDHCP( POP16 );
			return;
		}
#endif
	}

	if( !is_the_packet_for_me )
	{
#ifdef ETH_DEBUG
		sendstr( "not for me\n" );
#endif
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
		break;
	}

//finishcb:
//	enc424j600_finish_callback_now();
}


#ifdef INCLUDE_UDP

void util_finish_udp_packet( )// unsigned short length )
{
	volatile unsigned short ppl, ppl2;

	//Packet loaded.
	enc424j600_stopop();

	unsigned short length = enc424j600_get_write_length() - 6; //6 = my mac

	//Write length in packet
	enc424j600_alter_word( 10+6, length-14 ); //Experimentally determined 14, 0 was used for a long time.
	enc424j600_start_checksum( 8+6, 20 );
	enc424j600_alter_word( 32+6, length-34 );

	ppl = enc424j600_get_checksum();
	enc424j600_alter_word( 18+6, ppl );

	//Addenudm for UDP checksum

	enc424j600_alter_word( 34+6, 0x11 + 0x8 + length-42 ); //UDP number + size + length (of packet)
		//XXX I THINK 

	enc424j600_start_checksum( 20+6, length - 42 + 16 ); //sta

	ppl2 = enc424j600_get_checksum();
	if( ppl2 == 0 ) ppl2 = 0xffff;

	enc424j600_alter_word( 34+6, ppl2 );

	enc424j600_endsend();
}


#endif



void SwitchToBroadcast()
{
	//Set the address we want to send to (broadcast)
	for( i = 0; i < 6; i++ )
		macfrom[i] = 0xff;
}

#ifdef ARP_CLIENT_SUPPORT

int8_t RequestARP( uint8_t * ip )
{
	uint8_t i;

	for( i = 0; i < ARP_CLIENT_TABLE_SIZE; i++ )
	{
		if( memcmp( (char*)&ClientArpTable[i].ip, ip, 4 ) == 0 ) //XXX did I mess up my casting?
		{
			return i;
		}
	}

	SwitchToBroadcast();

	//No MAC Found.  Send an ARP request.
	enc424j600_finish_callback_now();
	enc424j600_startsend( NetGetScratch() );
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

	enc424j600_startsend( NetGetScratch() );
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


