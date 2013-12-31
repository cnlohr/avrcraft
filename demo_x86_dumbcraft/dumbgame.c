//Copyright 2012 <>< Charles Lohr, under the MIT-x11 or NewBSD license.  You choose.

#ifndef _DUMBGAME_H
#define _DUMBGAME_H

//NOTE: I am in the middle of an architecture change!  the statics will be leaving and this can be its own object file.  :( ttyl 500 bytes
//This file is meant to be included into dumbcraft.c
//Your game code goes here.

//This file is complete sphagetti code, you'll probably want to gut it for your project.

//The following functions are exposed to you for use:

//For receving packets
//static void Rbuffer( uint8_t * buffer, uint8_t size );
//static uint16_t Rshort()
//static uint32_t Rint()
//static void Rstring( char * data, int16_t maxlen )
//static int16_t Rdouble()  
//static int16_t Rfloat()

//For sending packets
//Sbyte( uint8_t o )
//static void Sint( uint32_t o )
//static void Sshort( uint16_t o )
//static void Sstring( const unsigned char * str, uint8_t len )
//static void Sbuffer( const uint8_t * buf, uint8_t len )
//static void Sdouble( int16_t i )
//static void Sfloat( int16_t i )

//Utility
//static void Str10Print( char * str, uint8_t val )  //fixed 3-digit one-byte readout
//static void Str16Print( char * str, uint8_t val )  //fixed 2-digit one-byte readout
//static void StrTack( char * str, uint16_t * optr, const char * strin ) //Send  ((NOTE))
//static void SignUp( uint8_t x, uint8_t y, uint8_t z, const char* st, uint8_t val ) //Update a specific sign
//static void SblockInternal( uint8_t x, uint8_t y, uint8_t z, uint8_t bt, uint8_t meta ) //Update a specific block


//Functions you must write:
//  static void PlayerTickUpdate( uint8_t playerid );
//  static void PlayerClick( uint8_t playerid, uint8_t x, uint8_t y, uint8_t z );  (when a player right-clicks on soemthing)
//  static void PlayerUpdate( uint8_t playerid );
//  static void DoCustomPreloadStep( uint8_t playerid );
//  static void InitDumbgame();
//  int ClientHandleChat( chat, chatlen ); //return 1 if you want the chat to propogate.

/* General notes for writing the game portion:
    1) You have a limited send-size, it's around 512 bytes.  Split up your commands among multiple packets.
	2) Do not send when receiving.  Add extra flags to the player structure to send when it's time to send.
*/

#include "dumbcraft.h"
#include <string.h>

uint8_t regaddr_set;
uint8_t regval_set;

volatile uint8_t regaddr_get;
volatile uint8_t regval_get;

uint8_t hasset_value;
uint8_t latch_setting_value;

void InitDumbgame()
{
	//no code.
}

void DoCustomPreloadStep( uint8_t playerid )
{
	struct Player * p = &Players[playerid];

//	printf( "Custom preload.\n" );
	SblockInternal( 16, 64, 16, 89, 0 );

	SblockInternal( 3, 64, 2, 63, 12 ); //create sign

	StartSend();
	Sbyte( 0x33 ); //sign update
	Sint( 3 ); Sshort( 64 ); Sint( 2 );
	Sstring( "Trigger", -1 );
	Sstring( "", 0 );
	Sstring( "<><", 3 );
	Sstring( "", 0 );
	DoneSend();


	SblockInternal( 3, 64, 1, 63, 12 ); //create sign

	StartSend();
	Sbyte( 0x33 ); //sign update
	Sint( 3 ); Sshort( 64 ); Sint( 1 );
	Sstring( "Latch", -1 );
	Sstring( "", 0 );
	Sstring( "<><", 3 );
	Sstring( "", 0 );
	DoneSend();

	p->custom_preload_step = 0;


	//actually spawns
	p->x = (1<<FIXEDPOINT)/2;
	p->y = 100*(1<<FIXEDPOINT);
	p->stance = p->y + (1<<FIXEDPOINT);
	p->z = (1<<FIXEDPOINT)/2;
	p->need_to_send_lookupdate = 1;
}

void PlayerTickUpdate( int playerid )
{
	//printf( "%f %f %f\n", SetDouble(p->x), SetDouble(p->y), SetDouble(p->z) );
	struct Player * p = &Players[playerid];
	if( ( dumbcraft_tick & 0x0f ) == 0 )
	{
		p->need_to_send_keepalive = 1;

		if( p->y < 0 )
		{
			p->need_to_respawn = 1;
		}
	}
}

void PlayerClick( uint8_t playerid, uint8_t x, uint8_t y, uint8_t z )
{
	struct Player * p = &Players[playerid];

	uint8_t didflip = 1;

	if( z == 2 && x == 4 )
	{
#ifdef __AVR__
		uint8_t * t = (uint8_t*)( (intptr_t)(regaddr_set + 0x20) );
		*t = regval_set;
#endif
		hasset_value = 10;
		didflip = 2;		
	}
	else if( z == 1 && x == 4 )
	{

		latch_setting_value = !latch_setting_value;
		didflip = 2;
		
	}
	else if( z >= 4 && z < 12 )
	{
		z-=4;
		switch( x )
		{
		case 4: regaddr_set ^= (1<<z); break;
		case 7: regval_set ^= (1<<z); break;
		case 10: regaddr_get ^= (1<<z); break;
		default: didflip = 0; break;
		}
	}
	else
		didflip = 0;

	if( didflip )
	{
		StartupBroadcast();
		StartSend();
		Sbyte( 0x29 ); //effect
		Sstring( "random.click", -1 );
		Sint( x<<3 );
		Sint( y<<3 );
		Sint( z<<3 );
		Sfloat( 1<<FIXEDPOINT );	
		Sbyte( 63 );	
		DoneSend();
		DoneBroadcast();
	}

}

void PlayerUpdate( uint8_t playerid )
{
	uint8_t i;
	struct Player * p = &Players[playerid];

	if( latch_setting_value )
	{
#ifdef __AVR__
		uint8_t * t = (uint8_t*)(intptr_t)(regaddr_set + 0x20);
		*t = regval_set;
#endif
	}
	if( hasset_value )
	{
		hasset_value--;
	}
	SblockInternal( 4, 64, 2, 69, hasset_value?6:14 );
	SblockInternal( 4, 64, 1, 69, latch_setting_value?6:14 );

	switch( p->update_number & 7 )
	{
	case 0:
		SblockInternal( 3, 64, 3, 68, 2 ); //create sign
		SignUp( 3, 64, 3, "Addr", regaddr_set );
		for( i = 0; i < 8; i++ )
		{
			SblockInternal( 4, 64, i+4, 69, ((regaddr_set)&(1<<i))?17:9 );
			SblockInternal( 3, 64, i+4, 35, ((regaddr_set)&(1<<i))?0:15 );
		}
		break;
	case 1:
		SblockInternal( 6, 64, 3, 68, 2 ); //create sign
		SignUp( 6, 64, 3, "Value", regval_set );
		for( i = 0; i < 8; i++ )
		{
			SblockInternal( 7, 64, i+4, 69, ((regval_set)&(1<<i))?17:9 );
			SblockInternal( 6, 64, i+4, 35, ((regval_set)&(1<<i))?0:15 );
		}
		break;
	case 2:
		SblockInternal( 9, 64, 3, 68, 2 ); //create sign
		SignUp( 9, 64, 3, "Read", regaddr_get );
		for( i = 0; i < 8; i++ )
		{
			SblockInternal( 10, 64, i+4, 69, ((regaddr_get)&(1<<i))?17:9 );
			SblockInternal( 9, 64, i+4, 35, ((regaddr_get)&(1<<i))?0:15 );
		}
		break;
	case 3:
	{
		SblockInternal( 12, 64, 3, 68, 2 ); //create sign
#ifdef __AVR__
		regval_get = *((uint8_t*)(intptr_t)(regaddr_get+0x20));
#endif
		SignUp( 12, 64, 3, "Read Value", regval_get );
		for( i = 0; i < 8; i++ )
		{
			SblockInternal( 12, 64, i+4, 35, ((regval_get)&(1<<i))?0:15 );
		}
		break;
	case 4: //position updates
		SblockInternal( 6, 64, 1, 68, 1 ); //create sign
		SignUp( 6, 64, 1, "PosX", p->x>>FIXEDPOINT );
	}
	default:
		break;
	}
}



uint8_t ClientHandleChat( char * chat, uint8_t chatlen )
{
	if( chat[0] == '/' )
	{
		if( strncmp( &chat[1], "title", 5 ) == 0 && chatlen > 8 )
		{
			chat[chatlen] = 0;
			return 0;
		}
	}
	return chatlen;
}


#endif
