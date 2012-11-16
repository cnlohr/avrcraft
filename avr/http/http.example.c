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

unsigned char MyIP[4] = { 192, 168, 2, 205 };
unsigned char MyMask[4] = { 255, 255, 255, 0 };
unsigned char MyMAC[6];


uint8_t TCPReceiveSyn( uint16_t portno )
{
//	sendhex4( portno );
	if( portno == 80 )
	{
		uint8_t ret = GetFreeConnection();
		HTTPInit( ret );
		return ret;
	}

	return 0;
}

void TCPConnectionClosing( uint8_t conn )
{
//	sendstr( "Lostconn\n" );
	HTTPClose( conn );
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

struct FileInfo filedescriptors[HTTP_CONNECTIONS];

void HTTPCustomStart( uint8_t conn )
{
	uint8_t i;
	struct HTTPConnection * h = &HTTPConnections[conn];
	const char * path = &h->pathbuffer[0];

	if( h->pathbuffer[0] == '/' )
		path++;

	h->clusterno = FindClusterFileInDir( path, ROOT_CLUSTER, -1, &h->bytesleft );
	h->bytessofar = 0;

	sendchr( 0 );
	sendstr( "Getting: " );
	sendstr( path );
	sendhex4( h->clusterno );
	sendchr( '\n' );

	if( h->clusterno < 0 )
	{
		h->is404 = 1;
	}
	else
	{
		InitFileStructure( &filedescriptors[conn], h->clusterno );
		h->isfirst = 1;
		h->isdone = 0;
		h->is404 = 0;
	}
}

void HTTPCustomCallback( uint8_t conn )
{
	uint16_t i, bytestoread;
	struct HTTPConnection * h = &HTTPConnections[conn];

	if( h->isdone )
	{
		HTTPClose( conn );
		return;
	}
	if( h->is404 )
	{
		TCPs[conn].sendtype = ACKBIT | PSHBIT;
		StartTCPWrite( conn );
		PushStr( "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\nFile not found." );
		EndTCPWrite( conn, 0 );
		h->isdone = 1;
		return;
	}
	if( h->isfirst )
	{
		char stto[10];
		uint8_t slen = strlen( h->pathbuffer );
		const char * k;
		TCPs[conn].sendtype = ACKBIT | PSHBIT;
		StartTCPWrite( conn );
		//TODO: Content Length?  MIME-Type?
		PushStr( "HTTP/1.1 200 Ok\r\nConnection: close\r\nContent-Length: " );
		Uint32To10Str( stto, h->bytesleft );
		PushStr( stto );
		PushStr( "\r\nContent-Type: " );
		//Content-Type?
		while( slen && ( h->pathbuffer[--slen] != '.' ) );
		k = &h->pathbuffer[slen+1];
		if( strcmp( k, "mp3" ) == 0 )
		{
			PushStr( "audio/mpeg3" );
		}
		else if( strncmp( k, "htm", 3 ) == 0 )
		{
			PushStr( "text/html" );
		}
		else
		{
			PushStr( "text/plain" );
		}
		PushStr( "\r\n\r\n" );
		EndTCPWrite( conn, 0 );
		h->isfirst = 0;
		return;
	}

	StartTCPWrite( conn );
	StartReadFAT( &filedescriptors[conn] );

	bytestoread = ((h->bytesleft)>512)?512:h->bytesleft;
	for( i = 0; i < bytestoread; i++ )
	{
		PUSH( read8FAT() );	
	}

	EndReadFAT();
	EndTCPWrite( conn, 0 );

	h->bytesleft -= bytestoread;

	if( !h->bytesleft )
		h->isdone = 1;
}


int main( void )
{
	uint8_t delayctr;
	uint8_t marker;

	//Input the interrupt.
	DDRD &= ~_BV(2);
	cli();
	setup_spi();
	sendstr( "HELLO\n" );
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

	if( initSD() )
	{
		sendstr( "Fatal error. Cannot open SD card.\n" );
		return -1;
	}

	openFAT();

	sei();

	InitTCP();
	DDRC &= 0;
	if( enc424j600_init( MyMAC ) )
	{
		sendstr( "Failure.\n" );
		while(1);
	}
	sendstr( "OK.\n" );

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
	}

	return 0;
} 




















