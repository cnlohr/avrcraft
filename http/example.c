#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "avr_print.h"
#include <stdio.h>
#include "iparpetc.h"
#include "enc424j600.h"
#include "http.h"
#include <basicfat.h>
#include <util10.h>
#include <avr/pgmspace.h>
#include <string.h>

#define NOOP asm volatile("nop" ::)


static void setup_clock( void )
{
	/*Examine Page 33*/

	CLKPR = 0x80;	/*Setup CLKPCE to be receptive*/
	CLKPR = 0x00;	/*No scalar*/

//	OSCCAL=0xff;
}

unsigned short frameno;
unsigned char cc;

#define POP enc424j600_pop8()
#define POP16 enc424j600_pop16()
#define PUSH(x) enc424j600_push8(x)
#define PUSH16(x) enc424j600_push16(x)

#ifndef INCLUDE_TCP
#error Need TCP for the HTTP test.
#endif

unsigned char MyIP[4] = { 192, 168, 0, 142 };
unsigned char MyMask[4] = { 255, 255, 255, 0 };
unsigned char MyMAC[6];


uint8_t TCPReceiveSyn( uint16_t portno )
{
	sendhex4( portno );
	if( portno == 80 )
	{
		uint8_t ret = GetFreeConnection();
		if( ret < HTTP_CONNECTIONS )
		{
			HTTPInit( ret, ret );
			return ret;
		}
	}

	return 0;
}

void TCPConnectionClosing( uint8_t conn )
{
//	sendstr( "Lostconn\n" );
	printf( "HTTPCX %d\n", conn );
	curhttp = &HTTPConnections[conn];
	HTTPClose( );
}

uint8_t didsend;

uint8_t TCPReceiveData( uint8_t connection, uint16_t totallen )
{
	didsend = 0;
	HTTPGotData( connection, totallen );
	return 0;
}


void PushStr( const char * msg )
{
	while( *msg )
	{
		PUSH( *(msg++) );
	}
}

void HTTPCustomStart( )
{
	uint8_t i, clusterno;
	struct HTTPConnection * h = curhttp;
	const char * path = &h->pathbuffer[0];

	sendstr( "Request: " );
	sendstr( path );
	sendchr( '\n' );

	h->data.user.a = 5;
	h->isdone = 0;
	h->isfirst = 1;
}

uint8_t hex2val( unsigned char c )
{
	if( c >= '0' && c <= '9' )
		return c - '0';
	else if( c >= 'a' && c <= 'f' )
		return c - 'a' + 10;
	else if( c >= 'A' && c <= 'F' )
		return c - 'A' + 10;
	else
		return 0;
}

void int8tohex( unsigned char v, char * data )
{
	unsigned char nibble = v>>4;
	data[0] = (nibble<10)?(nibble+'0'):(nibble+'a'-10);
	nibble = v&0x0f;
	data[1] = (nibble<10)?(nibble+'0'):(nibble+'a'-10);
}


void int16tohex( unsigned short v, char * data )
{
	unsigned char nibble = v>>12;
	data[0] = (nibble<10)?(nibble+'0'):(nibble+'a'-10);
	nibble = (v>>8)&0x0f;
	data[1] = (nibble<10)?(nibble+'0'):(nibble+'a'-10);
	nibble = (v>>4)&0x0f;
	data[2] = (nibble<10)?(nibble+'0'):(nibble+'a'-10);
	nibble = v&0x0f;
	data[3] = (nibble<10)?(nibble+'0'):(nibble+'a'-10);
}

void HTTPCustomCallback( )
{
	uint16_t i, bytestoread;
	struct HTTPConnection * h = curhttp;

	if( h->isdone )
	{
		printf( "HTTPCloseY\n" );
		HTTPClose( h->socket );
		return;
	}

	if( h->state_deets && !h->isfirst )
	{
		uint8_t j;
		unsigned char i = h->state_deets;
		char outb[129] = {0};
		TCPs[h->socket].sendtype = ACKBIT | PSHBIT;
		StartTCPWrite( h->socket );
		memset( outb, 32, 128 );

		int8tohex( TCPs[i].state, &outb[0] );
		int16tohex( TCPs[i].this_port, &outb[4] );
		int16tohex( TCPs[i].dest_port, &outb[9] );
		int16tohex( TCPs[i].dest_addr>>16, &outb[14] );
		int16tohex( TCPs[i].dest_addr, &outb[18] );

		int16tohex( TCPs[i].seq_num>>16, &outb[23] );
		int16tohex( TCPs[i].seq_num, &outb[27] );

		int16tohex( TCPs[i].ack_num>>16, &outb[32] );
		int16tohex( TCPs[i].ack_num, &outb[36] );

		outb[44] = '/';

		int8tohex( TCPs[i].time_since_sent, &outb[45] );
		int16tohex( TCPs[i].idletime, &outb[48] );
		int8tohex( TCPs[i].retries, &outb[53] );
		int8tohex( TCPs[i].sendtype, &outb[56] );

		outb[59] = '/';
		int16tohex( TCPs[i].sendptr, &outb[60] );
		int16tohex( TCPs[i].sendlength, &outb[65] );
		outb[70] = ':';

		int8tohex( HTTPConnections[i].state, &outb[71] );
		int8tohex( HTTPConnections[i].state_deets, &outb[74] );

		for( j = 0; j < 10; j++ )
			if(  HTTPConnections[i].pathbuffer[j] )
				outb[77+j] =  HTTPConnections[i].pathbuffer[j];

		outb[89] = '*';
		int8tohex( HTTPConnections[i].is_dynamic, &outb[90] );
		int16tohex( HTTPConnections[i].timeout, &outb[93] );

		int16tohex( HTTPConnections[i].bytesleft>>16, &outb[98] );
		int16tohex( HTTPConnections[i].bytesleft, &outb[102] );

		int16tohex( HTTPConnections[i].bytessofar>>16, &outb[107] );
		int16tohex( HTTPConnections[i].bytessofar, &outb[111] );


		int8tohex( HTTPConnections[i].is404, &outb[116] );
		int8tohex( HTTPConnections[i].isdone, &outb[119] );
		int8tohex( HTTPConnections[i].isfirst, &outb[122] );
		int8tohex( HTTPConnections[i].socket, &outb[125] );

		PushStr( outb );
		PushStr( "\n" );
		EndTCPWrite( h->socket );
		h->state_deets++;
		if( h->state_deets == HTTP_CONNECTIONS )
		{
			h->isdone = 1;
		}
		return;
	}

	if( h->isfirst )
	{
		TCPs[h->socket].sendtype = ACKBIT | PSHBIT;
		StartTCPWrite( h->socket );
		//TODO: Content Length?  MIME-Type?
		PushStr( "HTTP/1.1 200 Ok\r\nConnection: close\r\n\r\n" );
		if( strncmp( h->pathbuffer, "/d/internal", 11 ) == 0 )
		{
			EndTCPWrite( h->socket );
			h->isfirst = 0;
			h->state_deets = 1;
			return;
		}
		else if( strncmp( h->pathbuffer, "/d/r1?", 6 ) == 0 )
		{
			char outb[3] = {0, 0, 0};
			char * bp = h->pathbuffer + 6;
			unsigned char address = 0;
			address += hex2val( *(bp++) )<<4;
			address += hex2val( *(bp++) );

			unsigned char * cc = (unsigned char*)address;

			int8tohex( *cc, outb );
			PushStr( outb );
		}
		else if( strncmp( h->pathbuffer, "/d/w1?", 6 ) == 0 )
		{
			char * bp = h->pathbuffer + 6;
			unsigned char address = 0;
			address += hex2val( *(bp++) )<<4;
			address += hex2val( *(bp++) );

			unsigned char value = 0;
			value += hex2val( *(bp++) )<<4;
			value += hex2val( *(bp++) );

			unsigned char * cc = (unsigned char*)address;
			*cc = value;
		}
		else if( strncmp( h->pathbuffer, "/d/r2?", 6 ) == 0 )
		{
			char outb[3] = {0, 0, 0};
			char * bp = h->pathbuffer + 6;
			unsigned char address = 0;
			address += hex2val( *(bp++) )<<4;
			address += hex2val( *(bp++) );

			unsigned short * cc = (unsigned char*)address;
			unsigned short vo = *cc;
			int8tohex( vo>>8, outb );
			PushStr( outb );
			int8tohex( vo&0xff, outb );
			PushStr( outb );
		}
		else if( strncmp( h->pathbuffer, "/d/w2?", 6 ) == 0 )
		{
			char * bp = h->pathbuffer + 6;
			unsigned char address = 0;
			address += hex2val( *(bp++) )<<4;
			address += hex2val( *(bp++) );

			unsigned short value = 0;
			value += hex2val( *(bp++) )<<12;
			value += hex2val( *(bp++) )<<8;
			value += hex2val( *(bp++) )<<4;
			value += hex2val( *(bp++) );

			unsigned short * cc = (unsigned char*)address;
			*cc = value;
		}
		else
		{
			PushStr( "Hello, World!\r\n" );
			PushStr( h->pathbuffer );
		}
		EndTCPWrite( h->socket );
		h->isfirst = 0;
		h->isdone = 1;
		return;
	}

}


int main( void )
{
	uint8_t delayctr;
	uint8_t marker;
restart:

	//Input the interrupt.
	DDRD &= ~_BV(2);
	cli();
	setup_spi();
	sendstr( "#\n" );
	setup_clock();

	//Configure T2 to "overflow" at 100 Hz, this lets us run the TCP clock
	TCCR2A = _BV(WGM21) | _BV(WGM20);
	TCCR2B = _BV(WGM22) | _BV(CS22) | _BV(CS21) | _BV(CS20);
	//T2 operates on clkIO, fast PWM.  Fast PWM's TOP is OCR2A
	#define T2CNT  ((F_CPU/1024)/100)
	#if( T2CNT > 254 )
	#undef T2CNT
	#define T2CNT 254
	#endif
	OCR2A = T2CNT;

#ifndef HTTP_USE_MEMORY_FS
	if( initSD() )
	{
		sendstr( "Fatal error. Cannot open SD card.\n" );
	}

	openFAT();
#endif

	sei();

	InitTCP();
	DDRC &= 0;
	if( enc424j600_init( MyMAC ) )
	{
		sendstr( "!\n" );
		goto restart;
		//return -5;
		//while(1);
	}
	sendstr( "OK.\n" );

	while(1)
	{
		unsigned short r;

		r = enc424j600_recvpack( );
		if( r ) continue; //may be more


		if( TIFR2 & _BV(TOV2) )
		{
			TIFR2 |= _BV(TOV2);

			HTTPTick(1);
			TickTCP();
		} else
		{
			HTTPTick(0);
		}
	}

	return 0;
} 




















