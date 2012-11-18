//Copyright 2012 <>< Charles Lohr, under the MIT-x11 or NewBSD license.  You choose.

//Err this is a total spaghetti code file.  My interest is in the libraries, not this.

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "avr_print.h"
#include <stdio.h>
#include "iparpetc.h"
#include "enc424j600.h"
#include <avr/pgmspace.h>
#include "../dumbcraft.h"
#include <http.h>
#include <basicfat.h>
#include <string.h>
#include <util10.h>


/*
Useful ports:
  DDRC 0x07 = 0x2d
  PORTC 0x08  = 0x02

For Internal Temperature sensor:
   ADCSRA (5A) = 0xE4;
   ADCSRB (5B) = 0;
   ADMUX  (5C) = 0xE8;

   ADCH = 0x59 / ADCL = 0x58
*/


#define NOOP asm volatile("nop" ::)

static void setup_clock( void )
{
	/*Examine Page 33*/

	CLKPR = 0x80;	/*Setup CLKPCE to be receptive*/
	CLKPR = 0x00;	/*No scalar*/

	OSCCAL=0xff;
}

unsigned short frameno;
unsigned char cc;

#define RPORT 8001

#define POP enc424j600_pop8()
#define POP16 enc424j600_pop16()
#define PUSH(x) enc424j600_push8(x)
#define PUSH16(x) enc424j600_push16(x)

int8_t lastconnection = -1; //for dumbcraft
uint16_t bytespushed; //for dumbcraft

#ifdef NO_HTTP
#define HTTP_CONNECTIONS 0
#endif


int8_t MyGetFreeConnection( uint8_t minc, uint8_t max_not_inclusive )
{
	uint8_t i;
	for( i = minc; i < max_not_inclusive; i++ )
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

uint8_t TCPReceiveSyn( uint16_t portno )
{
	if( portno == 25565 )  //Must bump these up by 8... 
	{
		uint8_t ret = MyGetFreeConnection(HTTP_CONNECTIONS+1, TCP_SOCKETS );
		sendstr( "Conn attempt: " );
		sendhex2( ret );
		sendchr( '\n' );
		AddPlayer( ret  - HTTP_CONNECTIONS );
		return ret;
	}
#ifndef NO_HTTP
	else if( portno == 80 )
	{
		uint8_t ret = MyGetFreeConnection(1, HTTP_CONNECTIONS );
//		sendchr( 0 ); sendchr( 'x' ); sendhex2( ret );
		HTTPInit( ret );
		return ret;
	}
#endif
	return 0;
}

void TCPConnectionClosing( uint8_t conn )
{
	if( conn >= HTTP_CONNECTIONS )
	{
		sendstr( "Lostconn\n" );
		RemovePlayer( conn - HTTP_CONNECTIONS );
	}
#ifndef NO_HTTP
	else
	{
		HTTPClose( conn );
	}
#endif
}

uint8_t disable;

uint8_t CanSend( uint8_t playerno ) //DUMBCRAFT
{
	return TCPCanSend( playerno + HTTP_CONNECTIONS );
}

void SendStart( uint8_t playerno )  //DUMBCRAFT
{
	bytespushed = 0;
	lastconnection = playerno + HTTP_CONNECTIONS;
	disable = 0;
}

void PushByte( uint8_t byte ) //DUMBCRAFT
{
	if( disable )
	{
		return;
	}

	if( !TCPCanSend( lastconnection ) )
	{
		sendstr( "WARNING: TRYING TO SEND WHEN CANNOT SEND\n" );
		disable = 1;
		return;
	}

	if( bytespushed == 0 )
	{
		TCPs[lastconnection].sendtype = ACKBIT | PSHBIT;
		StartTCPWrite( lastconnection );
	}
	PUSH( byte );
	bytespushed++;
}

void EndSend( )  //DUMBCRAFT
{
	if( bytespushed == 0 )
	{
	}
	else
	{
		EndTCPWrite( lastconnection );
		//EmitTCP( lastconnection );
	}
}

void ForcePlayerClose( uint8_t playerno, uint8_t reason ) //DUMBCRAFT
{
	RequestClosure( playerno + HTTP_CONNECTIONS );
	RemovePlayer( playerno );	
}


uint16_t totaldatalen;
uint16_t readsofar;

uint8_t Rbyte()  //DUMBCRAFT
{
	if( readsofar++ > totaldatalen ) return 0;
	return POP;
}

uint8_t CanRead()  //DUMBCRAFT
{
	return readsofar < totaldatalen;
}

uint8_t TCPReceiveData( uint8_t connection, uint16_t totallen ) 
{
	if( connection >= HTTP_CONNECTIONS ) //DUMBCRAFT
	{
		totaldatalen = totallen;
		readsofar = 0;
		GotData( connection -  HTTP_CONNECTIONS );
		return 0; //Do this if we didn't send an ack.
	}
#ifndef NO_HTTP
	else
	{
		HTTPGotData( connection, totallen );
		return 0;
	}
#endif
	return 0;
}







//HTTP

#ifndef NO_HTTP


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
		EndTCPWrite( conn );
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
		else
		{
			PushStr( "text/html" );
		}

		PushStr( "\r\n\r\n" );
		EndTCPWrite( conn );
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
	EndTCPWrite( conn );

	h->bytesleft -= bytestoread;

	if( !h->bytesleft )
		h->isdone = 1;
}

#endif

unsigned char MyIP[4] = { 192, 168, 0, 142 };
unsigned char MyMask[4] = { 255, 255, 255, 0 };

unsigned char MyMAC[6];


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

	sei();

	//unsigned short phys[32];

#ifndef NO_HTTP
	if( initSD() )
	{
		sendstr( "Fatal error. Cannot open SD card.\n" );
		return -1;
	}

	openFAT();
#endif

	InitTCP();


	DDRC &= 0;
	if( enc424j600_init( MyMAC ) )
	{
		sendstr( "Failure.\n" );
		while(1);
	}
	sendstr( "OK.\n" );

	InitDumbcraft();

	while(1)
	{
		unsigned short r;

		r = enc424j600_recvpack( );
		if( r ) continue;

		UpdateServer();

		if( TIFR2 & _BV(TOV2) )
		{
			TIFR2 |= _BV(TOV2);
			sendchr( 0 );

			TickTCP();
#ifndef NO_HTTP
			HTTPTick();
#endif

			delayctr++;
			if( delayctr==10 )
			{
				delayctr = 0;
				TickServer();
			}
		}
	}

	return 0;
} 




















