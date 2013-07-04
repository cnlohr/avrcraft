//Copyright 2012 <>< Charles Lohr, under the MIT-x11 or NewBSD license.  You choose.

//Protocol thanks to #mcdevs, http://www.wiki.vg/Entities#Entity_Metadata_Format
//Special thanks on the 0x14 packet problem to SirCmpwn for helping me out.

//This file is designed to work both on x86 and on 8-bit AVR architectures.
//Whenever I had to choose between more features or smaller code size, I normally
//sided with the code size.

#include "dumbcraft.h"
#include <string.h>
#include <stdio.h>
#include "754lib.h"
#include <util10.h>
#include <alloca.h>

#ifdef __AVR__
#include "avr_print.h"
#include <avr/pgmspace.h> 
#else
#define PROGMEM
#endif

//#define DEBUG_DUMBCRAFT

//We don't want to pass the player ID around with us, this helps us 
uint8_t    thisplayer;

//Tick # (could be used for time of day? Or doing work every nth tick?)
//You could reset it every 24,000 ticks to make the day/night cycle right.
uint16_t dumbcraft_tick = 0;

//We store one gzipped set of chunks.  DO NOT Store this in ram if you can avoid it!
//I'm having trouble generating these, this one seems to be recognized by the client.
//See the mapmake folder for more info.  It'd be great if someone could fix that...
const uint8_t mapdata[] PROGMEM = {
	0x78, 0xda, 0xed, 0xda, 0x41, 0x01, 0x00, 0x30, 0x08, 0xc4, 0xb0, 0x0d, 0xff, 0x9e, 0x37, 0x1b, 
	0x70, 0x24, 0x2a, 0xfa, 0x68, 0x15, 0x00, 0x00, 0x00, 0x90, 0xee, 0x00, 0x00, 0x00, 0x00, 0xf1, 
	0x1e, 0x00, 0xb0, 0x8e, 0x02, 0x02, 0x80, 0x7d, 0x1c, 0x10, 0x00, 0x00, 0x00, 0xe0, 0xff, 0x07, 
	0x00, 0x00, 0x00, 0xe6, 0x73, 0x40, 0x02, 0x80, 0xff, 0x1f, 0x00, 0xc8, 0xe7, 0x80, 0x00, 0x00, 
	0x00, 0x00, 0xff, 0x3f, 0x00, 0x00, 0x00, 0x30, 0x9f, 0x03, 0x12, 0x00, 0xfc, 0xff, 0x00, 0x40, 
	0x3e, 0x07, 0x04, 0x00, 0x00, 0x00, 0xf8, 0xff, 0x01, 0x00, 0x00, 0x80, 0xf9, 0x1c, 0x90, 0x00, 
	0xe0, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfa, 0x73, 0x40, 0x02, 
	0x80, 0xff, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe8, 0xcf, 0x01, 0x09, 
	0x00, 0xfe, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x3f, 0x07, 0x24, 
	0x00, 0xf8, 0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xfe, 0x1c, 0x90, 
	0x00, 0xe0, 0xff, 0x07, 0x00, 0xf2, 0xdd, 0xe5, 0x3e, 0x54, 0x61, 0x44, 0xc1, };

const uint8_t default_spawn_metadata[] PROGMEM = { 0x00, 0x00, 0x48, 0x00, 0x00, 0x00, 0x00, 0x7F };

//////////////////////////////////////////READING UTILITIES////////////////////

//Read in a buffer from the current connection of specified size.
//Can only be done in a read callback.
void Rbuffer( uint8_t * buffer, uint8_t size )
{
	uint8_t i;
	for( i = 0; i < size; i++ )
	{
		buffer[i] = Rbyte();
	}
}

//Reading utilities for various types.
uint32_t Rint()
{
	uint32_t ret = 0;
	ret |= ((uint32_t)Rbyte())<<24;
	ret |= ((uint32_t)Rbyte())<<16;
	ret |= Rbyte()<<8;
	ret |= Rbyte();
	return ret;
}

uint16_t Rshort()
{
	uint16_t ret = 0;
	ret |= Rbyte()<<8;
	ret |= Rbyte();
	return ret;
}

void Rstring( char * data, int16_t maxlen )
{
	uint16_t toread = Rshort();
	uint16_t len = 0;
	//if( toread > maxlen ) toread = maxlen;
	while( toread-- )
	{
		if( len < maxlen )
			data[len++] = Rshort();
		else
			Rshort();
	}
}

//Read a double and immediately convert it into a fixed-point int16.
int16_t Rdouble()
{
	uint8_t doublebuffer[8];
	int16_t ir;
	Rbuffer( doublebuffer, 8 );
	ir = DoubleToW( (double*)doublebuffer, FIXEDPOINT );
	return ir;
}

//Read a float and immediately convert it into a fixed-point int16.
int16_t Rfloat()
{
	uint8_t floatbuffer[4];
	Rbuffer( floatbuffer, 4 );
	return FloatToW( (float*)floatbuffer, FIXEDPOINT );
}


//////////////////////////////////////////SENDING UTILITIES////////////////////

//send- utility functions

void Sint( uint32_t o )
{
	Sbyte( o >> 24 );
	Sbyte( o >> 16 );
	Sbyte( o >>  8 );
	Sbyte( o );
}

void Sshort( uint16_t o )
{
	Sbyte( o >> 8 );
	Sbyte( o );
}

//Send a string, -1 for length automatically sends a null-terminated string.
void Sstring( const unsigned char * str, uint8_t len )
{
	if( len == 255 ) len = strlen( str );
	uint8_t i;
	Sshort( len );
	for( i = 0; i < len; i++ )
	{	
		Sbyte( 0 );
		Sbyte( str[i] );
	}
}

//Send a buffer (NOTE: this cannot be used with a string as the byte-sizes are two-wide)
void Sbuffer( const uint8_t * buf, uint8_t len )
{
	uint8_t i;
	for( i = 0; i < len; i++ )
	{	
		Sbyte( buf[i] );
	}
}

//Send a buffer from program memory.
void SbufferPGM( const uint8_t * buf, uint8_t len )
{
	uint8_t i;
	for( i = 0; i < len; i++ )
	{	
#ifdef __AVR__
		Sbyte( pgm_read_byte( &buf[i] ) );
#else
		Sbyte( buf[i] );
#endif
	}
}

//Same IEEE754 utilities as reading, but, for sending
void Sdouble( int16_t i )
{
	uint8_t data[8];
	WToDouble( i, FIXEDPOINT, (double*)data );
	Sbuffer( data, 8 );
}

void Sfloat( int16_t i )
{
	uint8_t data[4];
	WToFloat( i, FIXEDPOINT, (float*)data );
	Sbuffer( data, 4 );
}

//////////////////////////////////////////GAME UTILITIES///////////////////////

//Update a sign at a specific location with a string and a numerical value.
void SignUp( uint8_t x, uint8_t y, uint8_t z, const char* st, uint8_t val )
{
	char stmp[5];
	Sbyte( 0x82 ); //sign update
	Sint( x );
	Sshort( y );
	Sint( z );
	Sstring( st, -1 );
	Uint8To10Str( stmp, val );
	Sstring( stmp, 3 );
	stmp[0] = '0'; stmp[1] = 'x';
	Uint8To16Str( stmp+2, val );
	Sstring( stmp, -1 );
	Sstring( stmp, 0 );
}

//Update a block at a given x, y, z (good for 0..255 in each dimension)
void SblockInternal( uint8_t x, uint8_t y, uint8_t z, uint8_t bt, uint8_t meta )
{
	Sbyte(0x35);
	Sint( x ); //x
	Sbyte( y ); //y
	Sint( z ); //z
	Sshort( bt ); //block type
	Sbyte( meta ); //metadata
}

//Spawn player (used to notify other clients about the spawnage)
void SSpawnPlayer( uint8_t pid )
{
	struct Player * p = &Players[pid];

	Sbyte( 0x14 );

	Sint( pid + PLAYER_EID_BASE );
	Sstring( p->playername, -1 );
	Sint( p->x );
	Sint( p->y );
	Sint( p->z );
	Sbyte( p->nyaw );
	Sbyte( p->npitch );
	Sshort( 0 );

	SbufferPGM( default_spawn_metadata, sizeof( default_spawn_metadata ) );
}

void UpdatePlayerSpeed( uint8_t playerno, uint8_t speed )
{
	Sbyte(0x2c);
	Sint( playerno );
	Sint( 1 );
	Sstring( "generic.movementSpeed", strlen( "generic.movementSpeed" ) );	
	Sdouble( speed );
}

#include "dumbgame.h"

void InitDumbcraft()
{
	memset( &Players, 0, sizeof( Players ) );
#ifdef DEBUG_DUMBCRAFT
	printf( "I: SOP: %lu\n", sizeof( struct Player ) );
#endif
	InitDumbgame();
}

void UpdateServer()
{
	uint8_t player, i;
	uint16_t i16;
	for( player = 0; player < MAX_PLAYERS; player++ )
	{
		struct Player * p = &Players[player];

		if( !p->active || !CanSend( player ) ) continue;

		p->update_number++;
		SendStart( player );

		//If we didn't finish getting all the broadcast data last time
		//We must do that now.
		if( p->has_logged_on )
		{
			if( p->did_not_clean_out_broadcast_last_time )
				goto now_sending_broadcast;
			else
			{
				//From the game portion
				PlayerUpdate( player );
			}
		}

		//BIG NOTE:
		//Everything in here is SELECTIVE! This means it gets sent to the specific client
		//ready to receive data!
		//
		//Login process:
		//
		//For checking in with the server:  (p->need_to_send_playerlist) and that's about it.
		//For players who are actually joining the server:
		//	p->need_to_login then
		//	p->need_to_spawn then
		//  p->next_chunk_to_load = 1..? for all chunks, one per packet. Then
		//	p->custom_preload_step then
		//   (Do your custom step)... Then, when YOU ARE DONE set
		//  p->need_to_send_lookupdate
		//   Now, it is listening to broadcast messages.
		//  Now, you're cooking!

		if( p->has_logged_on )
		{
			//If we turn around too far, we MUST warp a reset, because if we get an angle too big,
			//we overflow the angle in our 16-bit fixed point.

			//It's too expensive to do the proper modulus on a floating point value.
			//Additionally, we need to say stance++, otherwise we will fall through the ground when we turn around.
			if( p->yaw < -11520 ) { p->yaw += 11520; p->need_to_send_lookupdate = 1; p->stance++; }
			if( p->yaw > 11520 )  { p->yaw -= 11520; p->need_to_send_lookupdate = 1; p->stance++; }
		}

		//I'm worried about things overflowing here, should we consider some mechanism to help prevent this?

		//I used to do things here, it's a useful place to stick things that need to happen every tick..
		//I don't really use it anymore
		if( p->tick_since_update )
		{
			p->tick_since_update = 0;
		}

		if( p->need_to_send_lookupdate )
		{
			Sbyte(0x0d);
			Sdouble( p->x );
			Sdouble( p->stance );
			Sdouble( p->y );
			Sdouble( p->z );
			Sfloat( p->yaw );
			Sfloat( p->pitch );
			Sbyte(p->onground);

			p->need_to_send_lookupdate = 0;
		}

		//We're just logging in!
		if( p->need_to_spawn )
		{
			uint8_t i;

			Sbyte( 0x09 );
			Sint( WORLDTYPE ); //overworld
			Sbyte( 0 ); //peaceful
			Sbyte( GAMEMODE ); //creative
			Sshort( 256 ); //world height
			Sstring( "default", 8 );

			p->need_to_spawn = 0;
			p->next_chunk_to_load = 1;
			p->has_logged_on = 1;
			p->just_spawned = 1;  //For next time round we send to everyone

			//Show us the rest of the players
			for( i = 0; i < MAX_PLAYERS; i++ )
			{
				if( i != player && Players[i].active )
					SSpawnPlayer( i );
			}

		}
		if( p->custom_preload_step )
		{
			DoCustomPreloadStep( player );

			//This is when we checkin to the updates. (after we've sent the map chunk updates)
//			p->outcirctail = outcirchead;
		}

		//Send the client a couple chunks to load on.
		//Right now we just send a bunch of copy-and-pasted chunks.
		if( p->next_chunk_to_load )
		{
			uint16_t ichk = p->next_chunk_to_load - 1;
			if( p->next_chunk_to_load > MAPSIZECHUNKS*MAPSIZECHUNKS )
			{
				p->next_chunk_to_load = 0;
				p->custom_preload_step = 1;
			}
			else
			{
				uint16_t i;
				p->next_chunk_to_load++;

				Sbyte( 0x33 );
				Sint( ichk/MAPSIZECHUNKS );
				Sint( ichk%MAPSIZECHUNKS );
				Sbyte( 1 );
				Sshort( 0xff ); //bit-map
				Sshort( 0xff ); //bit-map
				Sint( sizeof( mapdata ) );


				SbufferPGM( mapdata, sizeof( mapdata ) );
			}
		}

		//This is triggered when players want to actually join.
		if( p->need_to_login )
		{

			p->need_to_login = 0;
			Sbyte( 0x01 ); //Login request
			Sint( player );
			Sstring( "default", 7 );
			Sbyte( GAMEMODE ); //creative
			Sbyte( WORLDTYPE ); //overworld
			Sbyte( 0 ); //peaceful
			Sbyte( 0 ); //not used.
			Sbyte( MAX_PLAYERS );
			p->need_to_spawn = 1;
//			p->next_chunk_to_load = 1;

		}
		if( p->need_to_send_playerlist )
		{
			uint16_t optr = 0;
			uint8_t stt[40];

			//XXX TODO: Rewrite this part! It's kind of nasty and we could make better use of program space.
			//It chews up extra stack, program AND .data space.

			StrTack( stt, &optr, "\xa7\x31" );	stt[optr++] = 0;
			StrTack( stt, &optr, PROTO_VERSION_STR ); 	stt[optr++] = 0;
			StrTack( stt, &optr, "\x32\x2e\x34\xe2\x32" ); 	stt[optr++] = 0;
			StrTack( stt, &optr, "dumbcraft" ); 	stt[optr++] = 0;
			StrTack( stt, &optr, "0" ); 	stt[optr++] = 0;   //XXX FIXME
			StrTack( stt, &optr, "20" );                       //XXX FIXME
			Sbyte( 0xff );

			Sstring( stt, optr );

			p->need_to_send_playerlist = 0;
		}

		if( p->need_to_send_keepalive )
		{
			Sbyte( 0x00 );
			p->keepalivevalue++;
			p->keepalivevalue &= 0x7f;
			Sint( p->keepalivevalue );

			p->need_to_send_keepalive = 0;
		}

		if( p->has_logged_on && !p->doneupdatespeed )
		{
			UpdatePlayerSpeed( player, p->running?RUNSPEED:WALKSPEED );
			p->doneupdatespeed = 1;
		}

now_sending_broadcast:
		//Apply any broadcast messages ... if we just spawned, then there's nothing to send.
		if( p->has_logged_on && !p->just_spawned )
		{
			p->did_not_clean_out_broadcast_last_time = UnloadCircularBufferOnThisClient( &p->outcirctail );
		}

		EndSend();
	}
}


//This function should only be called ~10x/second
void TickServer()
{
	uint8_t player;
	dumbcraft_tick++;

#ifdef DEBUG_DUMBCRAFT
//	printf( "Tick.\n" );
#endif

	//Everything in here should be broadcast to all players.
	StartupBroadcast();

	for( player = 0; player < MAX_PLAYERS; player++ )
	{
		struct Player * p = &Players[player];
		if( !p->active ) continue;

		//Send a keep-alive every so often
		if( ( dumbcraft_tick & 0x2f ) == 0 )
		{
			p->need_to_send_keepalive = 1;
		}

		if( p->just_spawned )
		{
			p->just_spawned = 0;
			SSpawnPlayer( player );
			DoneBroadcast();
			p->outcirctail = GetCurrentCircHead(); //If we don't, we'll see ourselves.
			StartupBroadcast();
		}

		if( p->x != p->ox || p->y != p->oy || p->stance != p->os || p->z != p->oz )
		{
			int16_t diffx = p->x - p->ox;
			int16_t diffy = p->y - p->oy;
			int16_t diffs = p->stance - p->os;
			int16_t diffz = p->z - p->oz;
			if( diffx < -127 || diffx > 127 || diffy < -127 || diffy > 127 || diffz < -127 || diffz > 127 )
			{
				Sbyte( 0x22 );
				Sint( player + PLAYER_EID_BASE );
				Sint( p->x );
				Sint( p->y );
				Sint( p->z );
				Sbyte( p->nyaw );
				Sbyte( p->npitch );
			}
			else
			{
				Sbyte( 0x1f );
				Sint( player + PLAYER_EID_BASE );
				Sbyte( diffx );
				Sbyte( diffy );
				Sbyte( diffz );
			}
			p->ox = p->x;
			p->oy = p->y;
			p->oz = p->z;
			p->os = p->stance;
		}

		if( p->pitch != p->op || p->yaw != p->ow )
		{
			Sbyte( 0x20 );
			Sint( player + PLAYER_EID_BASE );
			Sbyte( p->nyaw );
			Sbyte( p->npitch );
			Sbyte( 0x23 );
			Sint( player + PLAYER_EID_BASE );
			Sbyte( p->nyaw );

			p->op = p->pitch; p->ow = p->yaw;
		}

		p->tick_since_update = 1;

		PlayerTickUpdate( player );
	}

	DoneBroadcast();

}

void AddPlayer( uint8_t playerno )
{
//	printf( "Add: %d\n", playerno );
	memset( &Players[playerno], 0, sizeof( struct Player ) );
	Players[playerno].active = 1;
}

void RemovePlayer( uint8_t playerno )
{
//	printf( "Remove: %d\n", playerno );
	Players[playerno].active = 0;
	//todo send 0xff command
}



//From user to dumbcraft (you call)
void GotData( uint8_t playerno )
{
#ifdef DEBUG_DUMBCRAFT
	static uint8_t lastcmd;
#endif
	uint8_t cmd, i8;
	uint16_t i16;
	struct Player * p = &Players[playerno];
	thisplayer = playerno;
	uint8_t * chat = 0;
	uint8_t chatlen = 0;

	//This is where we read in a packet from a client.
	//You can send to the broadcast cicular buffer, but you
	// MAY NOT! send back to the sender unless you wait till the end
	//even then that is a prohibitively bad idea!

	while( CanRead() )
	{
		cmd = Rbyte();
		switch( cmd )
		{
		case 0x00: //Keep-alive response
			if( Rint() == p->keepalivevalue )
			{
				p->ticks_since_heard = 0;
				p->keepalivevalue |= 0x80;
			}
			break;
		case 0x02:  //Player kind of wants to check out the server.
			//Check protocol version
			if( Rbyte() != PROTO_VERSION )
			{
				ForcePlayerClose( playerno, 'v' );
#ifdef DEBUG_DUMBCRAFT
				printf( "Bad proto v.\n" );
#endif
				return;
			}
			Rstring( p->playername, MAX_PLAYER_NAME );
			Rstring( 0, 0 );
			Rint();
			p->need_to_login = 1;
			break;
		case 0x03: //Handle chat!  (I thought for a while before including this)
			i16 = Rshort();
			if( i16 < 100 )
			{
				chat = alloca( i16 );

				chatlen = i16;
				for( i8 = 0; i8 < i16; i8++ )
				{
					chat[i8] = Rshort();
				}
			}
			break;
		case 0x0a: //On-ground, client sends this to an annyoing degree.
			p->onground = Rbyte();
			break;
		case 0x0b: //Player position
			p->x = Rdouble();
			p->y = Rdouble();
			p->stance = Rdouble();
			p->z = Rdouble();
			p->onground = Rbyte();
			break;
		case 0x0d: //Player Position and look
			p->x = Rdouble();
			p->y = Rdouble();
			p->stance = Rdouble();
			p->z = Rdouble();
		case 0x0c: //Player look, only.
			p->yaw = Rfloat();
			p->pitch = Rfloat();
			p->nyaw = p->yaw/45;    //XXX TODO PROBABLY SLOW seems to add <256 bytes, though.  Is there a better way?
			p->npitch = p->pitch/45;//XXX TODO PROBABLY SLOW
			p->onground = Rbyte();
			break;
		case 0x0e: //player digging.
			Rbyte(); //action player is taking against block
			Rint(); //block pos X
			Rbyte(); //block pos Y
			Rint(); //block pos Z
			Rbyte(); //which face?
			break;
		case 0x0f:	//Block placement / right-click, used for levers.
		{
			uint8_t x = Rint();
			uint8_t y = Rbyte();
			uint8_t z = Rint();
			Rbyte();
			Rshort();
			Rbyte();
			Rbyte();
			Rbyte();

			PlayerClick( playerno, x, y, z );
			break;
		}
		case 0x12: //animation
			Rint(); //pid
			Rbyte(); //animation id
			break;
		case 0x13: //what is this? Entity action?
		{
			uint16_t ent = Rint();
			uint8_t act = Rbyte();
			if( ent == playerno )
			{
				switch (act)
				{
				case 0x04: //Player is running
				case 0x05:
					p->running = (act==0x04);
					p->doneupdatespeed = 0;
					break;
				}
			}

			Rint(); //???
			break;
		}
		case 0xfe: //We probably should respond more intelligently, but for now.
		{
			Rbyte(); //magic?
			p->need_to_send_playerlist = 1;
			break;
		}
		case 0xfa: //plugins (we drop them)  (and follow through to 0x74)
			Rstring( 0, 0 );
		case 0x74: //No idea what this is...
		{
			uint16_t rs;
			rs = Rshort();
			while( rs-- ) Rbyte();
			break;
		}
		case 0xcc: //language or something?
		{
			Rstring( 0, 0 );
			Rint();
			break;
		}
		default:
#ifdef DEBUG_DUMBCRAFT
			printf( "UCMD: (LAST) %02x - NOW: %02x\n", lastcmd, cmd );
			printf( "UPKT:\n" );
			//for( i16 = thisptr; i16 < packetsize; i16++ )	
			while( CanRead() )
			{
				unsigned char u = Rbyte();
//				printf( "%02x (%c)", u, u );
				printf( "%02x ", u );
			}
			printf( "\n" );
#endif
			return;
		}

#ifdef DEBUG_DUMBCRAFT
		lastcmd = cmd;
#endif
	}

	if( chatlen )
	{
		uint8_t pll = strlen( p->playername );

		StartupBroadcast();

		Sbyte( 0x03 );

		Sshort( chatlen + pll + 3 );
		Sshort( '<' ) ;
		for( i8 = 0; i8 < pll; i8++ )
			Sshort( p->playername[i8] ) ;
		Sshort( '>' ) ;
		Sshort( ' ' ) ;

		for( i8 = 0; i8 < chatlen; i8++ )
			Sshort( chat[i8] ) ;

		DoneBroadcast();
	}
}

//From dumbcraft to user (you accept)
//void SendData( uint8_t playerno, unsigned char * data, uint16_t packetsize );
//void ForcePlayerClose( uint8_t playerno ); //you still must call removeplayer, this is just a notification.

