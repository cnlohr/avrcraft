//Copyright 2012 <>< Charles Lohr, under the MIT-x11 or NewBSD license.  You choose.

#ifndef _DUMBCRAFT_H
#define _DUMBCRAFT_H

#define MAX_PLAYERS 3
#define MAX_PLAYER_NAME 17
#define PROTO_VERSION 73
#define PROTO_VERSION_STR "73"

#define PLAYER_EID_BASE 0x20

//Overworld
#define WORLDTYPE 0
#define GAMEMODE  0

//For floating-point values when converting to int16's
//Smaller numbers yeild less accuracy, but bigger maximums
#define FIXEDPOINT 5

#define MAPSIZECHUNKS 1

#define RUNSPEED 5
#define WALKSPEED 3

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
void SendStart( uint8_t playerno ); //prepare a buffer for send
void Sbyte( uint8_t byte );  //Push to either player _or_ circular buffer.
uint8_t CanSend( uint8_t playerno ); //can this buffer be a send?
void EndSend( );
void ForcePlayerClose( uint8_t playerno, uint8_t reason ); //you still must call removeplayer, this is just a notification.
uint8_t UnloadCircularBufferOnThisClient( uint16_t * whence ); //Push to the current client everything remaining on their stack
uint16_t GetCurrentCircHead();
void StartupBroadcast(); //Set up broadcast output.
void DoneBroadcast(); //Done with broadcast mode.
void UpdatePlayerSpeed( uint8_t playerno, uint8_t speed );  //before shifting is done by FIXEDPOINT

//Game section

struct Player
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
	uint8_t next_chunk_to_load;
	uint8_t custom_preload_step; //if nonzero, then do pre-load, when done, set to 0 and set p->need_to_send_lookupdate = 1;

	uint8_t update_number;

	uint8_t tick_since_update:1;
	uint8_t ticks_since_heard;
	uint8_t keepalivevalue;    //must be 128-255 to be valid.
	uint8_t playername[MAX_PLAYER_NAME]; //todo: store playername length
} Players[MAX_PLAYERS];

#endif

