//Copyright 2012 <>< Charles Lohr, under the MIT-x11 or NewBSD license.  You choose.

#ifndef _DUMBCRAFT_H
#define _DUMBCRAFT_H

#define PROTO_VERSION 47
#define PROTO_VERSION_STR "47"
#define LONG_PROTO_VERSION "1.8"

#define PLAYER_EID_BASE 0x20
#define PLAYER_LOGIN_EID_BASE 0x40
#define MAX_CHATLEN 100

#include "dumbconfig.h"

//For floating-point values when converting to int16's
//Smaller numbers yeild less accuracy, but bigger maximums
#define FIXEDPOINT 5

#include <stdint.h>

//Communications core section
void InitDumbcraft();
void UpdateServer();
void TickServer();
void AddPlayer( uint8_t playerno );
void RemovePlayer( uint8_t playerno );
void GotData( uint8_t playerno );



//you must provide (For communications, etc.)
uint8_t Rbyte();
uint8_t CanRead();
void Rposition( uint8_t * x, uint8_t * y, uint8_t * z );
void SendStart( uint8_t playerno ); //prepare a buffer for send
void extSbyte( uint8_t byte );  //Push to either player _or_ circular buffer.
uint8_t CanSend( uint8_t playerno ); //can this buffer be a send?
void EndSend( );
void ForcePlayerClose( uint8_t playerno, uint8_t reason ); //you still must call removeplayer, this is just a notification.
uint8_t UnloadCircularBufferOnThisClient( uint16_t * whence ); //Push to the current client everything remaining on their stack
uint16_t GetCurrentCircHead();
void StartupBroadcast(); //Set up broadcast output.
void DoneBroadcast(); //Done with broadcast mode.
void UpdatePlayerSpeed( uint8_t speed );  //before shifting is done by FIXEDPOINT
void PlayerChangeSlot( uint8_t slotno );
void PlayerBlockAction( uint8_t status, uint8_t x, uint8_t y, uint8_t z, uint8_t face );

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
	uint8_t next_chunk_to_load;
	uint8_t custom_preload_step; //if nonzero, then do pre-load, when done, set to 0 and set p->need_to_send_lookupdate = 1;

	uint8_t need_to_respawn:1;
	uint8_t handshake_state;
	uint8_t update_number;

	uint8_t tick_since_update:1;
	uint8_t ticks_since_heard;
	uint8_t playername[MAX_PLAYER_NAME]; //todo: store playername length

//	uint32_t keepalive_id; //can also be used for pinging
} Players[MAX_PLAYERS];

extern uint16_t dumbcraft_tick;
extern uint8_t dumbcraft_playercount;
extern uint8_t playerid;

//Tools for the user:
void Rbuffer( uint8_t * buffer, uint8_t size );
void StartSend(); //For single packet.
void DoneSend(); //For single packet
void Sbyte( uint8_t b );
void Svarint( uint16_t v );
uint32_t Rint();
uint16_t Rshort();
void Rstring( char * data, int16_t maxlen );
int16_t Rdouble();
int16_t Rfloat();
void Sint( uint32_t o );
void Sshort( uint16_t o );
void Sstring( const char * str, uint8_t len );
void Sbuffer( const uint8_t * buf, uint8_t len );
void SbufferWide( const uint8_t * buf, uint8_t len );
void SbufferPGM( const uint8_t * buf, uint8_t len );
void SstringPGM( const char * str );
void Sdouble( int16_t i );
void Sfloat( int16_t i );
void SignUp( uint8_t x, uint8_t y, uint8_t z, const char* st, uint8_t val );
void SblockInternal( uint8_t x, uint8_t y, uint8_t z, uint8_t bt, uint8_t meta );
void SSpawnPlayer( uint8_t pid );
void UpdatePlayerSpeed( uint8_t speed );

//You must write the following:
uint8_t ClientHandleChat( char * chat, uint8_t chatlen ); 
void PlayerUpdate( );
void GameTick();
void PlayerClick( uint8_t x, uint8_t y, uint8_t z, uint8_t dir );
void PlayerTickUpdate(  );
void DoCustomPreloadStep(  );
void InitDumbgame();



#endif

