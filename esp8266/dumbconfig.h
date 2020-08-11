#ifndef _DUMBCONFIG_H
#define _DUMBCONFIG_H

#include <esp82xxutil.h>

int strlen( const char * s );

//#define DEBUG_DUMBCRAFT

#define MAX_PLAYERS 16
#define MAX_PLAYERS_STRING "16"

#define MAX_PLAYER_NAME 32

//Overworld
#define WORLDTYPE 0
#define GAMEMODE  0

#define RUNSPEED 5
#define WALKSPEED 3

#define DFDEC ICACHE_FLASH_ATTR
#define SENDBUFFERSIZE  256

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

