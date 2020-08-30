#ifndef _DUMBCONFIG_H
#define _DUMBCONFIG_H

//#define DEBUG_DUMBCRAFT

#define MAX_PLAYERS 3
#define MAX_PLAYERS_STRING "3"

#define MAX_PLAYER_NAME 17

#define CHUNKS_TO_LOAD 64
#define CHUNKS_TO_LOAD_XPROFILE 3 //Number of "bits" "2" = 4 chunks profile.


//Overworld = 0
#define WORLDTYPE 0
#define GAMEMODE  1

#define RUNSPEED 5
#define WALKSPEED 3

#define SENDBUFFERSIZE  128

#define STATIC_SERVER_STAT_STRING
#define STATIC_MOTD_NAME

#define MOTD_NAME "hello"
//extern char my_server_name[];

//#define INCLUDE_ANNOUNCE_UTILS

#define MINECRAFT_PORT 25565
#define MINECRAFT_PORT_STRING "25565"

#define NEED_PLAYER_CLICK
//#define NEED_PLAYER_BLOCK_ACTION
//#define NEED_SLOT_CHANGE


#endif

