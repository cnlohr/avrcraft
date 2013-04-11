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
#include <ntsc.h>
#include <video.h>
#include <tcp.h>

#define NOOP asm volatile("nop" ::)

const char * twiddles = "/-\\|";
uint8_t twiddle = 0;



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

int8_t termconn = -1;

uint8_t TCPReceiveSyn( uint16_t portno )
{
//	sendhex4( portno );

	if( portno == 80 )
	{
		uint8_t ret = GetFreeConnection();
		HTTPInit( ret, ret );
		return ret;
	}

	if( portno == 23 )
	{
		if( termconn == -1 )
		{
			sendstr( "New.\n" );
			termconn = GetFreeConnection();
			return termconn;
		}
		else
		{
			sendstr( "Ful.\n" );			
		}
	}

	return 0;
}

void TCPConnectionClosing( uint8_t conn )
{
	if( conn == termconn )
	{
		termconn = -1;
	}
	else
	{
		HTTPClose( conn );
	}
}

uint8_t didsend;

uint8_t TCPReceiveData( uint8_t connection, uint16_t totallen )
{
	if( connection == termconn )
	{
		while( totallen-- )
			sendchr( POP );
	}
	else
	{
		didsend = 0;
		HTTPGotData( connection, totallen );
		return 0;
	}
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

void HTTPCustomCallback( )
{
	uint16_t i, bytestoread;
	struct HTTPConnection * h = curhttp;

	if( h->isdone )
	{
		HTTPClose( h->socket );
		return;
	}
	if( h->isfirst )
	{
		TCPs[h->socket].sendtype = ACKBIT | PSHBIT;
		StartTCPWrite( h->socket );
		//TODO: Content Length?  MIME-Type?
		PushStr( "HTTP/1.1 200 Ok\r\nConnection: close\r\n\r\n" );

		if( strncmp( h->pathbuffer, "/d/r1?", 6 ) == 0 )
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


volatile extern char framebuffer[];


int main( void )
{
	char stip[4];
	uint8_t delayctr, i;
	uint8_t marker;

	termconn = -1;

	//Input the interrupt.
	DDRD &= ~_BV(2);
	cli();

	setup_clock();
	SetupNTSC();
	setup_spi();

restart:

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
		sendstr( "SD card Fail.\n" );
	}

	openFAT();
#endif

	sei();

	InitTCP();
	DDRC &= 0;
	if( enc424j600_init( MyMAC ) )
	{
		sendstr( "Network Fail!\n" );
		goto restart;
	}

	sendstr( "Network Ok at " );

	for( i = 0; i < 4; i++ )
	{
		Uint8To10Str( stip, MyIP[i] );
		sendstr( stip );
		if( i != 3 )
			sendchr( '.' );
	}


	sendstr( "\nBoot Ok.\n" );

	while(1)
	{
		unsigned short r;

		r = enc424j600_recvpack( );
		if( r ) continue; //may be more

		HTTPTick();

		if( TIFR2 & _BV(TOV2) )
		{
			TIFR2 |= _BV(TOV2);

			TickTCP();
		}

		framebuffer[NTWIDTH*NTLINES-1] = twiddles[(twiddle++)&3];

	}

	return 0;
} 




















