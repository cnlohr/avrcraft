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
#include "dumbcraft.h"
#include <http.h>
#include <string.h>
#include <basicfat.h>
#include <avr/eeprom.h> 
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
uint8_t  EEMEM my_server_name_eeprom[16]; 
char my_server_name[16];

void SetServerName( const char * stname )
{
	memcpy( my_server_name, stname, strlen( stname ) + 1 );
	eeprom_write_block( stname, my_server_name_eeprom, strlen( stname ) + 1 );
}

#ifdef NO_HTTP
#undef HTTP_CONNECTIONS
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
	if( portno == MINECRAFT_PORT )  //Must bump these up by 8... 
	{
		uint8_t ret = MyGetFreeConnection(HTTP_CONNECTIONS+1, TCP_SOCKETS );
		sendstr( "Conn attempt: " );
		sendhex2( ret );
		sendchr( '\n' );
		AddPlayer( ret - HTTP_CONNECTIONS - 1 );
		return ret;
	}
#ifndef NO_HTTP
	else if( portno == 80 )
	{
		uint8_t ret = MyGetFreeConnection(1, HTTP_CONNECTIONS+1 );
//		sendchr( 0 ); sendchr( 'x' ); sendhex2( ret );
		HTTPInit( ret-1, ret );
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
		RemovePlayer( conn - HTTP_CONNECTIONS - 1 );
	}
#ifndef NO_HTTP
	else
	{
		curhttp = &HTTPConnections[conn-1];
		HTTPClose(  );
	}
#endif
}
/////////////////////////////////////////UDP AREA//////////////////////////////////////////////

void HandleUDP( uint16_t len )
{
	POP16; //Checksum
	len -= 8; //remove header.

	//You could pop things, or check ports, etc. here.

	return;
}


///////////////////////////////////////DUMBCRAFT AREA//////////////////////////////////////////

uint8_t disable;

uint8_t CanSend( uint8_t playerno ) //DUMBCRAFT
{
	return TCPCanSend( playerno + HTTP_CONNECTIONS + 1 );
}

void SendStart( uint8_t playerno )  //DUMBCRAFT
{
	bytespushed = 0;
	lastconnection = playerno + HTTP_CONNECTIONS + 1;
	disable = 0;
}

//Must be power-of-two
#define CIRC_BUFFER_SIZE 4096
#if ( (FREE_ENC_END) - (FREE_ENC_START) ) <= (CIRC_BUFFER_SIZE)
#error Not enough free space on enc424j600
#endif
#define CIRC_END ((FREE_ENC_START) + (CIRC_BUFFER_SIZE))

uint16_t circ_buffer_at = 0;
uint8_t incirc = 0;
uint16_t GetCurrentCircHead()
{
	return circ_buffer_at;
}

uint8_t UnloadCircularBufferOnThisClient( uint16_t * whence )
{
	//TRICKY: we're in the middle of a transaction now.
	//We need to back out, copy the memory, then pretend nothing happened.
	uint16_t w = *whence;
	uint16_t togomaxB = (circ_buffer_at - w)&((CIRC_BUFFER_SIZE)-1);

	if( togomaxB == 0 ) return 0;


	ETCSPORT |= ETCS;
	uint16_t esat = enc424j600_read_ctrl_reg16( EEGPWRPTL );

	uint16_t togomaxA = 512 - ( esat - sendbaseaddress );
	uint16_t togo = (togomaxA>togomaxB)?togomaxB:togomaxA;

//printf( "w: %04x togo: %04x (%04x/%04x)  %04x %04x %04x\n", w, togo, togomaxA, togomaxB, FREE_ENC_START, CIRC_END, circ_buffer_at );
	enc424j600_copy_memory( esat, w + (FREE_ENC_START), togo, (FREE_ENC_START), CIRC_END-1 );

	enc424j600_write_ctrl_reg16( EEGPWRPTL, esat + togo );
	ETCSPORT &= ~ETCS;
	espiW( EWGPDATA );

	*whence = (w + togo)&((CIRC_BUFFER_SIZE)-1);

	return togo != togomaxB;

}


void StartupBroadcast()
{
	ETCSPORT |= ETCS;

	//is_in_outcirc = 1;
	enc424j600_write_ctrl_reg16( EEUDAWRPTL, circ_buffer_at + (FREE_ENC_START) );
	enc424j600_write_ctrl_reg16( EEUDASTL, FREE_ENC_START );
	enc424j600_write_ctrl_reg16( EEUDANDL, CIRC_END - 1);

	ETCSPORT &= ~ETCS;
	espiW( EWUDADATA );
	incirc = 1;
}

void DoneBroadcast()
{
	ETCSPORT |= ETCS;
	circ_buffer_at = enc424j600_read_ctrl_reg16( EEUDAWRPTL ) - (FREE_ENC_START);
	incirc = 0;
}

void extSbyte( uint8_t byte ) //DUMBCRAFT
{
	if( !incirc )
	{
		if( bytespushed == 0 )
		{
			TCPs[lastconnection].sendtype = ACKBIT | PSHBIT;
			StartTCPWrite( lastconnection );
		}
		bytespushed++;
	}
	PUSH( byte );
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
	if( connection > HTTP_CONNECTIONS ) //DUMBCRAFT
	{
		totaldatalen = totallen;
		readsofar = 0;
		GotData( connection -  HTTP_CONNECTIONS - 1 );
		return 0; //Do this if we didn't send an ack.
	}
#ifndef NO_HTTP
	else
	{
		HTTPGotData( connection-1, totallen );
		return 0;
	}
#endif
	return 0;
}







//HTTP

#ifndef NO_HTTP

void HTTPCustomStart( )
{
	//curhttp->is404 = 1;
	curhttp->bytesleft = 0xffffffff;
}

void HTTPCustomCallback( )
{
	uint8_t i = 0;
	if( curhttp->isfirst || curhttp->is404 || curhttp->isdone )
	{
		HTTPHandleInternalCallback();
		return;
	}

	StartTCPWrite( curhttp->socket );
	do
	{
		enc424j600_push8( 0 );
		enc424j600_push8( 0xff );
	} while( ++i );
	EndTCPWrite( curhttp->socket );
}

#endif



unsigned char MyIP[4] = { 192, 168, 33, 142 };
unsigned char MyMask[4] = { 255, 255, 255, 0 };
unsigned char MyGateway[4] = { 192, 168, 0, 1 };
unsigned char MyMAC[6];


void GotDHCPLease()
{
	int i;
	char st[5];

//	puts( "New Lease." );
//	printf( "IP: %d.%d.%d.%d\n", MyIP[0], MyIP[1], MyIP[2], MyIP[3] );
//	printf( "MS: %d.%d.%d.%d\n", MyMask[0], MyMask[1], MyMask[2], MyMask[3] );
//	printf( "GW: %d.%d.%d.%d\n", MyGateway[0], MyGateway[1], MyGateway[2], MyGateway[3] );
	sendstr( "Got DHCP Lease" );
	for( i = 0; i < 4; i++ )
	{
		Uint8To10Str( st, MyIP[i] );
		sendstr( st );
		sendchr( '.' );
	}
	sendchr( '\n' );
	
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

	eeprom_read_block( &my_server_name[0], &my_server_name_eeprom[0], 16 );
	if( my_server_name[0] == 0 || my_server_name[0] == (char)0xff )
		SetServerName( "AVRCraft" );

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
#ifndef HTTP_USE_MEMORY_FS
	if( initSD() )
	{
		sendstr( "Fatal error. Cannot open SD card.\n" );
	}

	openFAT();
#endif
#endif

	InitTCP();

	DDRC &= 0;
	if( enc424j600_init( MyMAC ) )
	{
		sendstr( "Failure.\n" );
		while(1);
	}
	sendstr( "OK.\n" );

	SetupDHCPName( my_server_name );

	InitDumbcraft();

	while(1)
	{
		unsigned short r;

		r = enc424j600_recvpack( );

		if( r ) continue;

		UpdateServer();

#ifndef NO_HTTP
			HTTPTick(0);
#endif

		if( TIFR2 & _BV(TOV2) )
		{
			TIFR2 |= _BV(TOV2);
			sendchr( 0 );

			TickTCP();

			delayctr++;
			if( delayctr==10 )
			{
				TickDHCP();
				delayctr = 0;
				TickServer();
			}

#ifndef NO_HTTP
			HTTPTick(1);
#endif

		}
	}

	return 0;
} 




















