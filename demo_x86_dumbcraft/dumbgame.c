//Copyright 2012 <>< Charles Lohr, under the MIT-x11 or NewBSD license.  You choose.

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


/* General notes for writing the game portion:
    1) You have a limited send-size, it's around 512 bytes.  Split up your commands among multiple packets.
	2) Do not send when receiving.  Add extra flags to the player structure to send when it's time to send.
*/

#include "dumbcraft.h"
#include "dumbutils.h"
#include <string.h>
#include <stdlib.h>

uint8_t regaddr_set;
uint8_t regval_set;

volatile uint8_t regaddr_get;
volatile uint8_t regval_get;

uint8_t hasset_value;
uint8_t latch_setting_value;

uint8_t didflip = 1;
uint8_t flipx, flipy, flipz;

int GOFFSET = 0;

void InitDumbgame()
{
	//no code.
}

void DoCustomPreloadStep( )
{
	struct Player * p = &Players[playerid];

//	printf( "Custom preload.\n" );
//	SblockInternal( 16, 64, 16, 89 + 0 );

	SblockInternal( 3, 64, 2, BLOCK_OAK_SIGN_BASE_ID ); //create sign
	SignTextUp( 3, 64, 2, "Trigger", "<><" );
	SblockInternal( 3, 64, 1, BLOCK_OAK_SIGN_BASE_ID ); //create sign
	SignTextUp( 3, 64, 1, "Latch", "<><" );

	p->custom_preload_step = 0;

	SpawnEntity( 3, 58, 10*32, 64*32, 1*32 );

	//actually spawns
	p->x = (1<<FIXEDPOINT)/2;
	p->y = 100*(1<<FIXEDPOINT);
	p->stance = p->y + (1<<FIXEDPOINT);
	p->z = (1<<FIXEDPOINT)/2;
	p->need_to_send_lookupdate = 1;

	int slot = 0;
	static const uint16_t startslots[] = { 82, 83, 84, 85, 86, 87, 64, 146, 601 };
	for( slot = 0; slot < 9; slot++ )
	{
		StartSend();
		Sbyte( 0x17 ); //Update slot 1.15.2
		Sbyte( 0 ); //Player inventory
		Sshort( 36+slot ); //Slots 36-44 are in-hand. 0-4 are crafting.
		Sbyte( 0x1 ); //Present
		Svarint( startslots[slot] ); //Block id (White wool is 82)
		Sbyte( 100 ); //Item count
		Sbyte( 0 ); //No NBT
		DoneSend();
	}

}

void PlayerTickUpdate( )
{
	//printf( "%f %f %f\n", SetDouble(p->x), SetDouble(p->y), SetDouble(p->z) );
	struct Player * p = &Players[playerid];
	if( ( dumbcraft_tick & 0x0f ) == 0 )
	{
		p->need_to_send_keepalive = 1;

		if( p->y < 0 )
		{
			p->x = (1<<FIXEDPOINT)/2;
			p->y = 100*(1<<FIXEDPOINT);
			p->stance = p->y + (1<<FIXEDPOINT);
			p->z = (1<<FIXEDPOINT)/2;
			p->need_to_send_lookupdate = 1;
		}
	}
}

void PlayerBlockAction( uint8_t status, uint8_t x, uint8_t y, uint8_t z, uint8_t face )
{
//	printf( "Player Block Action %d %d %d %d\n", status, x, y, z, face );
}

void PlayerChangeSlot( uint8_t slotno )
{
//	printf( "Slot no change: %d\n", slotno );
}

void GameTick()
{
	static int frame;
	frame++;

#if 0
	//Update hand items from GOFFSET
		StartSend();
		Sbyte( 0x17 ); //Update slot 1.15.2
		Sbyte( 0 ); //Player inventory
		Sshort( (frame%10)+36 ); //Slots 36-44 are in-hand. 0-4 are crafting.
		Sbyte( 0x1 ); //Present
		Svarint( (frame%10)+GOFFSET*10 ); //Block id (White wool is 82)
		Sbyte( 100 ); //Item count
		Sbyte( 0 ); //No NBT
		DoneSend();
#endif

	if( didflip )
	{
		EntityUpdatePos( 3, rand()%512, 32*64, rand()%512, rand(), rand() );


		StartSend();
		Sbyte( 0x4F ); //1.5.2 Update day time.
		Slong( 0 );
		Slong( regaddr_get * 100 );
		DoneSend();

		StartSend();
		Sbyte( 0x52 ); //1.5.2 Sound Effect
		Svarint( 379 ); //Sound effect #
		Svarint( 0 ); //Category #
		Sint( (uint16_t)(flipx<<3) );
		Sint( (uint16_t)(flipy<<3) );
		Sint( (uint16_t)(flipz<<3) );
		Sfloat( 32 ); //100% volume
		Sfloat( 32 ); //100% speed
		DoneSend();

		didflip = 0;
	}
}

void PlayerUse( uint8_t hand )
{
	//No code.
}

void PlayerClick( uint8_t x, uint8_t y, uint8_t z, uint8_t dir )
{
	if( z == 2 && x == 4 )
	{
		hasset_value = 10;
		didflip = 2;	
		GOFFSET--;
	}
	else if( z == 1 && x == 4 )
	{

		latch_setting_value = !latch_setting_value;
		didflip = 2;
		GOFFSET++;
	}
	else if( z >= 4 && z < 12 )
	{
		z-=4;
		switch( x )
		{
		case 4: regaddr_set ^= (1<<z); didflip=3; break;
		case 7: regval_set ^= (1<<z); didflip=3; break;
		case 10: regaddr_get ^= (1<<z); didflip=3; break;
		}
	}

	if( didflip )
	{
		flipx = x;
		flipy = y;
		flipz = z;
	}
	else
	{
		const int8_t * dor = dir_offsets[dir];
		x += dor[0];
		y += dor[1];
		z += dor[2];
		//printf( "Block place %d %d %d %d\n", x, y, z, dir );
	}
}

void PlayerUpdate( )
{
	uint8_t i;
	struct Player * p = &Players[playerid];

	if( latch_setting_value )
	{
	}
	if( hasset_value )
	{
		hasset_value--;
	}

	SblockInternal( 4, 64, 2, BLOCK_LEVER_BASE_ID + hasset_value );
	SblockInternal( 4, 64, 1, BLOCK_LEVER_BASE_ID + latch_setting_value );

	switch( p->update_number & 7 )
	{
	case 0:
		SignUp( 3, 64, 2, "GO", GOFFSET );
		SblockInternal( 3, 64, 3, BLOCK_OAK_SIGN_BASE_ID ); //create sign
		SignUp( 3, 64, 3, "Addr", regaddr_set );
		for( i = 0; i < 8; i++ )
		{
			SblockInternal( 4, 64, i+4, BLOCK_LEVER_BASE_ID + (((regaddr_set)&(1<<i))?0:1) );
			SblockInternal( 3, 64, i+4, BLOCK_WOOL_BASE_ID + (((regaddr_set)&(1<<i))?WOOL_OFFSET_WHITE:WOOL_OFFSET_DARKBLACK) );
		}
		break;
	case 1:
		SblockInternal( 6, 64, 3, BLOCK_OAK_SIGN_BASE_ID ); //create sign
		SignUp( 6, 64, 3, "Value", regval_set );
		for( i = 0; i < 8; i++ )
		{
			SblockInternal( 7, 64, i+4, BLOCK_LEVER_BASE_ID + (((regval_set)&(1<<i))?0:1) );
			SblockInternal( 6, 64, i+4, BLOCK_WOOL_BASE_ID + (((regval_set)&(1<<i))?WOOL_OFFSET_WHITE:WOOL_OFFSET_DARKBLACK) );
		}
		break;
	case 2:
		SblockInternal( 9, 64, 3, BLOCK_OAK_SIGN_BASE_ID ); //create sign
		SignUp( 9, 64, 3, "Read", regaddr_get );
		for( i = 0; i < 8; i++ )
		{
			SblockInternal( 10, 64, i+4, BLOCK_LEVER_BASE_ID + (((regaddr_get)&(1<<i))?0:1) );
			SblockInternal( 9, 64, i+4, BLOCK_WOOL_BASE_ID + (((regaddr_get)&(1<<i))?WOOL_OFFSET_WHITE:WOOL_OFFSET_DARKBLACK) );
		}
		break;
	case 3:
	{
		SblockInternal( 12, 64, 3, BLOCK_OAK_SIGN_BASE_ID ); //create sign
		SignUp( 12, 64, 3, "Read Value", regval_get );
		for( i = 0; i < 8; i++ )
		{
			SblockInternal( 12, 64, i+4, BLOCK_WOOL_BASE_ID + ((regval_get)&(1<<i))?0:1 );
		}
		break;
	}
	default:
		break;
	}
}


void SetServerName( const char * stname );

uint8_t ClientHandleChat( char * chat, uint8_t chatlen )
{
	if( chat[0] == '/' )
	{
		if( strncmp( &chat[1], "title", 5 ) == 0 && chatlen > 8 )
		{
			chat[chatlen] = 0;
//			SetServerName( &chat[7] );
			return 0;
		}
	}
	return chatlen;
}


