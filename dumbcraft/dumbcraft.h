//Copyright 2012 <>< Charles Lohr, under the MIT-x11 or NewBSD license.  You choose.

#ifndef _DUMBCRAFT_H
#define _DUMBCRAFT_H

#define PROTO_VERSION 578
#define PROTO_VERSION_STR "578"
#define LONG_PROTO_VERSION "1.15.2"

#define BLOCK_GRASS_ID 9
#define BLOCK_WOOL_BASE_ID 1383
#define BLOCK_LEVER_BASE_ID 3781
#define BLOCK_OAK_SIGN_BASE_ID 3380

#define PLAYER_EID_BASE 0x20
#define PLAYER_LOGIN_EID_BASE 0x40
#define MAX_CHATLEN 100

#include "dumbconfig.h"

//Dumbcraft Function Decorator
#ifndef DFDEC
#define DFDEC
#endif


//For floating-point values when converting to int16's
//Smaller numbers yeild less accuracy, but bigger maximums
#define FIXEDPOINT 5

#include <stdint.h>

//Communications core section
void DFDEC InitDumbcraft();
void DFDEC UpdateServer();
void DFDEC TickServer();
void DFDEC AddPlayer( uint8_t playerno );
void DFDEC RemovePlayer( uint8_t playerno );
void DFDEC GotData( uint8_t playerno );



//you must provide (For communications, etc.)
uint8_t DFDEC Rbyte();
uint8_t DFDEC CanRead();
void DFDEC  Rposition( uint8_t * x, uint8_t * y, uint8_t * z );
void DFDEC SendStart( uint8_t playerno ); //prepare a buffer for send
void DFDEC extSbyte( uint8_t byte );  //Push to either player _or_ circular buffer.
uint8_t DFDEC CanSend( uint8_t playerno ); //can this buffer be a send?
void DFDEC EndSend( );
void DFDEC ForcePlayerClose( uint8_t playerno, uint8_t reason ); //you still must call removeplayer, this is just a notification.
uint8_t DFDEC UnloadCircularBufferOnThisClient( uint16_t * whence ); //Push to the current client everything remaining on their stack
uint16_t DFDEC GetCurrentCircHead();
void DFDEC StartupBroadcast(); //Set up broadcast output.
void DFDEC DoneBroadcast(); //Done with broadcast mode.
void DFDEC UpdatePlayerSpeed( uint8_t speed );  //before shifting is done by FIXEDPOINT
void DFDEC PlayerChangeSlot( uint8_t slotno );
void DFDEC PlayerBlockAction( uint8_t status, uint8_t x, uint8_t y, uint8_t z, uint8_t face );

//Game section

extern struct Player
{
	//Used for broadcast sending
	uint16_t outcirctail;

	int16_t x, y, stance, z, yaw, pitch;
	int16_t ox, oy, os, oz, ow, op;

	uint8_t nyaw, npitch;
	uint8_t did_not_clean_out_broadcast_last_time:1; //Didn't finish getting all the broadcast circular buffer cleared.
		//This prevents any 'update' code from happening next time round.
	uint8_t did_move:1;
	uint8_t did_pitchyaw:1;
	uint8_t just_spawned:1;

	uint8_t running:1;
	uint8_t doneupdatespeed:1;

	uint8_t onground:1;
	uint8_t active:1;
	uint8_t has_logged_on:1;

	uint8_t need_to_send_playerlist:1;
	uint8_t need_to_spawn:1;
	uint8_t need_to_login:1;
	uint8_t need_to_send_keepalive:1;
	uint8_t need_to_send_lookupdate:1;
	uint8_t need_to_reply_to_ping:1;
	uint8_t player_is_up_and_running:1;  //Sent after the custom preload is done.

	uint8_t next_chunk_to_load;
	uint8_t custom_preload_step; //if nonzero, then do pre-load, when done, set to 0 and set p->need_to_send_lookupdate = 1;

	uint8_t need_to_respawn:1;
	uint8_t handshake_state;
	uint8_t update_number;

	uint8_t set_compression:1; //Once set, need to handle packets differently.

	uint8_t tick_since_update:1;
	uint8_t ticks_since_heard;
	uint8_t playername[MAX_PLAYER_NAME]; //todo: store playername length

//	uint32_t keepalive_id; //can also be used for pinging
} Players[MAX_PLAYERS];

extern uint16_t dumbcraft_tick;
extern uint8_t dumbcraft_playercount;
extern uint8_t playerid;

//Tools for the user:
void DFDEC Rbuffer( uint8_t * buffer, uint8_t size );
void DFDEC StartSend(); //For single packet.
void DFDEC DoneSend(); //For single packet
void DFDEC Sbyte( uint8_t b );
void DFDEC Sdouble( int16_t i );
void DFDEC Sfloat( int16_t i );
void DFDEC Svarint( uint16_t v );
uint32_t DFDEC Rint();
uint16_t DFDEC Rshort();
void DFDEC Rstring( char * data, int16_t maxlen );
int16_t DFDEC Rdouble();
int16_t DFDEC Rfloat();
void DFDEC Sint( uint32_t o );
void DFDEC Sshort( uint16_t o );
void DFDEC Sstring( const char * str, uint8_t len );
void DFDEC Sbuffer( const uint8_t * buf, uint8_t len );
void DFDEC SbufferWide( const uint8_t * buf, uint8_t len );
void DFDEC SbufferPGM( const uint8_t * buf, uint8_t len );
void DFDEC SstringPGM( const char * str );
void DFDEC Sdouble( int16_t i );
void DFDEC Sfloat( int16_t i );
void DFDEC Suuid( uint16_t uuidid );
void DFDEC SignUp( uint8_t x, uint8_t y, uint8_t z, const char* st, uint8_t val );
void DFDEC SblockInternal( uint8_t x, uint8_t y, uint8_t z, uint16_t blockid );
void DFDEC SSpawnPlayer( uint8_t pid );
void DFDEC UpdatePlayerSpeed( uint8_t speed );

//You must write the following:
uint8_t DFDEC ClientHandleChat( char * chat, uint8_t chatlen ); 
void DFDEC PlayerUpdate( );
void DFDEC GameTick();
void DFDEC PlayerClick( uint8_t x, uint8_t y, uint8_t z, uint8_t dir );
void DFDEC PlayerTickUpdate(  );
void DFDEC DoCustomPreloadStep(  );
void DFDEC InitDumbgame();



#endif

