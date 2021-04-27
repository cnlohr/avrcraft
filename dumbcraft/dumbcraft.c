//Copyright 2012 <>< Charles Lohr, under the MIT-x11 or NewBSD license.  You choose.

//Protocol thanks to #mcdevs, http://www.wiki.vg/Protocol http://www.wiki.vg/Entities#Entity_Metadata_Format
//Special thanks on the 0x14 packet problem to SirCmpwn for helping me out.

//This file is designed to work both on x86 and on 8-bit AVR architectures.
//Whenever I had to choose between more features or smaller code size, I normally
//sided with the code size.

#include "dumbcraft.h"
#include <string.h>
#include "754lib.h"
#include <util10.h>
#include <alloca.h>

#define ONGROUND 1

#ifdef __AVR__
#include "avr_print.h"
#include <avr/pgmspace.h> 
#else

#define pgm_read_byte(x) (*(x))

#define PROGMEM
#define PSTR
#define memcpy_P memcpy
#endif

struct Player Players[MAX_PLAYERS];
uint8_t playerid;

//#define DEBUG_DUMBCRAFT

//Tick # (could be used for time of day? Or doing work every nth tick?)
//You could reset it every 24,000 ticks to make the day/night cycle right.
uint16_t dumbcraft_tick = 0;

//This is the raw data we're supposed to put on the wire for
//sending "compress packets; here's the 0,0 chunk; don't compress packets"
//look in 'mkmk2' for more info.
uint8_t compeddata[] PROGMEM = {
	0x36, 0x92, 0x62, 0x78, 0xda, 0xed, 0xc1, 0x31, 0x0d, 0x00, 0x20, 0x0c, 0x00, 0xb0, 0x91, 0x70, 
	0x70, 0x4e, 0x02, 0xda, 0x50, 0xb2, 0x03, 0xe1, 0x3c, 0x98, 0x80, 0xb4, 0x9d, 0x71, 0xb5, 0xb1, 
	0x57, 0x46, 0x65, 0x0f, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0xf5, 0xf5, 0x00, 0x00, 0x00, 
	0x00, 0xdf, 0x3b, 0xbb, 0x10, 0x41, 0xb0, };

const char pingjson1[] PROGMEM = "{\"description\":\"";
const char pingjson2[] PROGMEM = "\", \"players\":{\"max\":";
const char pingjson3[] PROGMEM = ",\"online\":";
const char pingjson4[] PROGMEM = "},\"version\":{\"name\":\""LONG_PROTO_VERSION"\",\"protocol\":"PROTO_VERSION_STR"}}";

const int8_t dir_offsets[6][3] = { { 0, -1, 0 }, { 0, 1, 0 }, { 0, 0, -1 }, { 0, 0, 1 }, { -1, 0, 0 }, { 1, 0, 0 } };

uint8_t dumbcraft_playercount;

/////////////////////////////////////////BIZARRE UTILITIES/////////////////////

void mstrcat( char * out, char * in, int maxlen )
{
	uint8_t i, j = 0;
	for( i = 0; i < maxlen; i++ )
	{
		if( out[i] == 0 ) break;
	}

	for( ; i < maxlen; i++ )
	{
		out[i] = in[j++];
	}

	if( i < maxlen )
		out[i] = 0;
}

void mstrcatp( char * out, const char * in, int maxlen )
{
	uint8_t i, j = 0;
	for( i = 0; i < maxlen; i++ )
	{
		if( out[i] == 0 ) break;
	}

	for( ;i < maxlen; i++ )
	{
		out[i] = pgm_read_byte(&in[j++]);
	}

	if( i < maxlen )
		out[i] = 0;
}

//////////////////////////////////////////READING UTILITIES////////////////////

uint16_t cmdremain = 0;

uint8_t dcrbyte()
{
	if( cmdremain )
	{
		cmdremain--;
		return Rbyte();
	}
	return 0;
}

static uint8_t localsendbuffer[SENDBUFFERSIZE];
static uint8_t sendsize;

void StartSend()
{
	sendsize = 0;
}

void DoneSend()
{
	uint8_t i;

	uint8_t comp = playerid == MAX_PLAYERS || Players[playerid].set_compression;
	if( comp )
		sendsize++;

	if( sendsize > 127 )
	{
		extSbyte( 128 | (sendsize&127) );
		extSbyte( sendsize >> 7 );
	}
	else
	{
		extSbyte( sendsize );
	}

	//Mark packet as uncompressed.
	if( comp )
	{
		extSbyte( 0 );
		sendsize--;
	}

	for( i = 0; i < sendsize; i++ )
	{
		extSbyte( localsendbuffer[i] );
	}
}

void SendRawPGMData( const uint8_t * PROGMEM dat, uint16_t len )
{
	uint16_t i;
	for( i = 0; i < len; i++ )
	{
		extSbyte( pgm_read_byte(&dat[i]) );
	}
}


void Sbyte( uint8_t b )
{
	if( sendsize < SENDBUFFERSIZE )
		localsendbuffer[sendsize++] = b;
}

//Read in a buffer from the current connection of specified size.
//Can only be done in a read callback.
void Rbuffer( uint8_t * buffer, uint8_t size )
{
	uint8_t i;
	for( i = 0; i < size; i++ )
	{
		buffer[i] = dcrbyte();
	}
}

void Rdump( uint16_t length )
{
	while( length-- ) dcrbyte();
}

//Reading utilities for various types.
uint32_t Rint()
{
	uint32_t ret = 0;
	ret |= ((uint32_t)dcrbyte())<<24;
	ret |= ((uint32_t)dcrbyte())<<16;
	ret |= dcrbyte()<<8;
	ret |= dcrbyte();
	return ret;
}

uint16_t Rshort()
{
	uint16_t ret = 0;
	ret |= dcrbyte()<<8;
	ret |= dcrbyte();
	return ret;
}

void Rposition( uint8_t * x, uint8_t * y, uint8_t * z )
{
	dcrbyte();
	dcrbyte();
	*x = dcrbyte()<<2;
	*x |= dcrbyte()>>6;
	dcrbyte();
	*z = dcrbyte()<<4;
	*z = dcrbyte()>>4;
	*y = dcrbyte();	

/*	dcrbyte();
	dcrbyte();
	*x = dcrbyte()<<2;
	uint8_t nr = dcrbyte();
	*x |= nr>>6;
	*y = nr<<6;
	*y |= dcrbyte()>>2;
	dcrbyte();
	dcrbyte();
	*z = dcrbyte(); */
}

uint16_t Rvarint()
{
	uint16_t ret = dcrbyte();
	if( ret & 0x80 )
	{
		ret = (ret&0x7f)|(dcrbyte()<<7);
	}

	return ret;
}

uint16_t Rslot()
{
/*	uint16_t ret = */Rshort();
/*	uint8_t  count = */dcrbyte();
/*	uint16_t damage = */Rshort();
	uint16_t nbt = Rshort();
	if( nbt != 0xffff )
	{
		Rdump( nbt );
	}
	return nbt;
}

void Rstring( char * data, int16_t maxlen )
{
	uint16_t toread = Rvarint();
	uint16_t len = 0;
	//if( toread > maxlen ) toread = maxlen;
	while( toread-- )
	{
		if( len < maxlen )
			data[len++] = dcrbyte();
		else
			dcrbyte();
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

void Svarint( uint16_t v )
{
	if( v > 16383 )
	{
		Sbyte( 128 | (v&127) ); v >>= 7;	//Normally would shift sign bit but we're unsigned.
		Sbyte( 128 | (v&127) ); v >>= 7;
		Sbyte( v );
	}
	else if( v > 127 )
	{
		Sbyte( 128 | (v&127) );
		Sbyte( v >> 7 );
	}
	else
		Sbyte( v );
}

void Sint( uint32_t o )
{
	Sbyte( o >> 24 );
	Sbyte( o >> 16 );
	Sbyte( o >>  8 );
	Sbyte( o );
}

void Slong( uint32_t o )
{
	Sint( 0 );
	Sint( o );
}
void Sshort( uint16_t o )
{
	Sbyte( o >> 8 );
	Sbyte( o );
}

void Suuid( uint16_t uuid )
{
	int i;
	char stmp[38];
	for( i = 0; i < 36; i++ )
		stmp[i] = '0';
	stmp[36] = 0;
	stmp[8] = '-';
	stmp[13] = '-';
	stmp[18] = '-';
	stmp[23] = '-';
	stmp[14] = '3';
	stmp[19] = '8';
	stmp[1] = ((uuid>>13)&0x07) + '0';
	stmp[2] = ((uuid>>10)&0x07) + '0';
	stmp[3] = ((uuid>>7)&0x07) + '0';
	stmp[4] = ((uuid>>4)&0x07) + '0';
	stmp[5] = ((uuid>>1)&0x07) + '0';
	stmp[6] = ((uuid>>0)&0x01) + '0';
	Sstring( stmp, -1 );  //UUID (Yuck) 00000000-0000-0000-0000-000000000000
}

void Sencuuid( uint16_t uuidid )
{
	//send as 16-byte 2-longs.
	Slong( 0 );
	Slong( uuidid );
}

//Send a string, -1 for length automatically sends a null-terminated string.
void Sstring( const char * str, uint8_t len )
{
	if( len == 255 ) len = strlen( (const char*)str );
	uint8_t i;
	Svarint( len );
	for( i = 0; i < len; i++ )
	{	
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

void SbufferWide( const uint8_t * buf, uint8_t len )
{
	uint8_t i;
	for( i = 0; i < len; i++ )
	{	
		Sshort( buf[i] );
	}
}

//Send a buffer from program memory.
void SbufferPGM( const uint8_t * buf, uint8_t len )
{
	uint8_t i;
	for( i = 0; i < len; i++ )
	{	
		Sbyte( pgm_read_byte( &buf[i] ) );
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

//Spawn player (used to notify other clients about the spawnage)
void SSpawnPlayer( uint8_t pid )
{
//	if( pid == playerid ) return;
	struct Player * p = &Players[pid];

	//Thanks to @Tachyon_ from #mcdevs for helping with this.

	//Player Info
	StartSend();
	Sbyte( 0x32 ); //1.16.5 Player Info
	Svarint( 0 ); //Add player
	Svarint( 1 ); //1 player
	Sencuuid( pid + PLAYER_EID_BASE );
	Sstring( (char*)p->playername, -1 );
	Svarint( 0 ); //No props
	Svarint( GAMEMODE ); //Game mode
	Svarint( 0 ); //Ping
	Sbyte( 0 ); //Has display name
	DoneSend();

	StartSend();
	Sbyte( 0x56 );  //1.16.5 Entity teleport
	Svarint( (uint8_t)(pid + PLAYER_EID_BASE) );
	Sdouble( p->x );
	Sdouble( p->y );
	Sdouble( p->z );
	Sbyte( p->nyaw );
	Sbyte( p->npitch );
	Sbyte( 0x01 ); //On Ground
	DoneSend();

	//Will set location and close loading screen.
	//EntityUpdatePos( pid + PLAYER_EID_BASE, p->x, p->y, p->z, p->nyaw, p->npitch );
}

void UpdatePlayerSpeed( uint8_t speed )
{
	StartSend();
	Sbyte(0x58); //1.16.5 Entity Properties
	Svarint( playerid );
	Sint( 1 );
	Sstring( "generic.movementSpeed", -1 );	
	//SstringPGM( PSTR("generic.movementSpeed") );
	Sdouble( speed );
	Svarint(0);
	DoneSend();
}


#ifdef INCLUDE_ANNOUNCE_UTILS
#ifndef INCLUDE_UDP
#error ERROR! You must include UDP if you want to have broadcast.
#endif
#include "iparpetc.h"
void SendAnnounce( )
{
	const char * sending;
/*
	macfrom[0] = 0x01;
	macfrom[1] = 0x00;
	macfrom[2] = 0x5e;
	macfrom[3] = 0x00;
	macfrom[4] = 0x02;
	macfrom[5] = 0x3c;
*/
	SwitchToBroadcast();

	enc424j600_stopop();
	enc424j600_startsend( 0 );
	send_etherlink_header( 0x0800 );
//	send_ip_header( 0, "\xE0\x00\x02\x3c", 17 ); //UDP Packet to 224.0.2.60
	send_ip_header( 0, "\xff\xff\xff\xff", 17 ); //UDP Packet to 255.255.255.255

	enc424j600_push16( MINECRAFT_PORT+1 );
	enc424j600_push16( 4445 );
	enc424j600_push16( 0 ); //length for later
	enc424j600_push16( 0 ); //csum for later

#ifdef STATIC_MOTD_NAME
	enc424j600_pushpgmstr( PSTR( "[MOTD]"MOTD_NAME"[/MOTD][AD]"MINECRAFT_PORT_STRING"[/AD]" ) );
#else
	enc424j600_pushpgmstr( PSTR( "[MOTD]" ) );
	enc424j600_pushstr( MOTD_NAME );
	enc424j600_pushpgmstr( PSTR( "[/MOTD][AD]"MINECRAFT_PORT_STRING"[/AD]" ) );
#endif

	util_finish_udp_packet();
}
#endif

void InitDumbcraft()
{
	memset( &Players, 0, sizeof( Players ) );
#ifdef DEBUG_DUMBCRAFT
	printf( "I: SOP: %d\n", sizeof( struct Player ) );
#endif
	InitDumbgame();
}

#include <stdio.h>
void LextSbyte( char c )
{
	//FILE * f = fopen( "packetlog.dat", "ab" );
	//fprintf( f, "%c", c );
	//fclose( f );
	extSbyte( c );
}

void UpdateServer()
{
	uint8_t localplayercount = 0;
	for( playerid = 0; playerid < MAX_PLAYERS; playerid++ )
	{
		struct Player * p = &Players[playerid];

		if( p->active ) localplayercount++;

		if( !p->active || !CanSend( playerid ) ) continue;

		//Send the client a couple chunks to load on. 
		//this happens after Updated (Join Game)
		//This is a very careful operation - we cannot interrupt it with another packet!!
		if( p->next_chunk_to_load )
		{
			int nnc = p->next_chunk_to_load;
			int pnc = ++p->next_chunk_to_load_state;
			
			int cx = (nnc-1) & ((1<<CHUNKS_TO_LOAD_XPROFILE)-1);
			int cz = (nnc-1) >> CHUNKS_TO_LOAD_XPROFILE;
			cx -= 1;
			cz -= 1;
			SendStart( playerid );
			if( pnc == 1 )
			{
				//SUPER BIG THANKS TO <x10A94> FOR HELPING ME UNDERSTAND THIS.

				int sendsize = 1 + 1 + 4 + 4 + 1 /* varint, bitmask */ 
					+ (1+25+36*8) /* heightmaps */ 
					+ 1 + 1024*4 +
					 1 /* varint, sizeof data in bytes */ + 1 /* varint sizeof array */;

				extSbyte( 128 | (sendsize&127) );
				extSbyte( sendsize >> 7 );
				extSbyte( 0 );
				LextSbyte( 0x22 );
				uint8_t ch = (cx<0)?0xff:0x00;
				LextSbyte( ch ); LextSbyte( ch ); LextSbyte( ch ); LextSbyte( cx );
				ch = (cz<0)?0xff:0x00;
				LextSbyte( ch ); LextSbyte( ch ); LextSbyte( ch ); LextSbyte( cz );
				LextSbyte( 1 ); //Full-chunk.
				LextSbyte( 0x00 ); //Bitmap

				const char * heightmapblob = "\x0A\0\0\x0C\0\x0fMOTION_BLOCKING\0\0\0\x24";
				int i;
				for( i = 0; i < 25; i++ ) LextSbyte( heightmapblob[i] );
				for( i = 0; i < 288; i++ ) LextSbyte( 0 ); //Heightmap (in longs)
				LextSbyte( 0 );

				//XXX TODO HERE!!! PICK UP HERE!!! 
				//1: Global palette!
				//2: Why does client think chunk is unloaded?

				//SendRawPGMData( compeddata, sizeof(compeddata) );
#define BLOCKS256  16
#define BLOCKS256B  0
			}
			else if( pnc < (BLOCKS256+2))
			{
				int i;
				for( i = 0; i < 1024/BLOCKS256; i++ )
				{
					LextSbyte( 0 );
					LextSbyte( 0 );
					LextSbyte( 0 );
					LextSbyte( 0x7f );
				}
				//2..65
			}
			else if( pnc < (BLOCKS256+3) )
			{
				LextSbyte( 0 );
				LextSbyte( 0 ); //Block entities 
			}
			else
			{
				//35+
				int chk = pnc - (BLOCKS256+3);
				if( chk == 16 || cx < 0 || cz < 0 )
				{
					//StartSend();
					//Sbyte( 0x41 ); //1-15-2 Update View Position --> TODO: Needed whenever crossing block boundaries
					//Svarint( 0 );
					//Svarint( 0 );  //Was not working in 1-15-2
					//DoneSend();

					int nnc = p->next_chunk_to_load;
					//If you want to load more chunks, do it here.
					if( nnc >= CHUNKS_TO_LOAD ) nnc = 0; else nnc++;

					p->next_chunk_to_load = nnc;
					p->next_chunk_to_load_state = 0;
					p->custom_preload_step = 1;
				}
				else
				{
					int k = 0;
					for( k = 0; k < 16; k++ )
					{
//						SblockInternal( k+cx*16, 128, chk+cz*16, 230 ); //Put glass at Y=128 fixes the client if you are going to be doing a lot of updates.
						SblockInternal( k+cx*16, 63, chk+cz*16, BLOCK_GRASS_ID );
					}
				}
			}

			EndSend();
			continue;
		}



		p->update_number++;
		SendStart( playerid );

		if( p->need_to_reply_to_ping )
		{
			StartSend();
			Sbyte( 0x01 );
			Sbuffer( p->playername, 8 );
			DoneSend();
			p->need_to_reply_to_ping = 0;
		}

		//XXX TODO Player list doesn't seem to be sending.
		if( p->need_to_send_playerlist )
		{
			unsigned length = sizeof( pingjson1 ) + sizeof( pingjson2 ) + sizeof( pingjson3 ) + sizeof( pingjson4 ) + 24 + strlen( MOTD_NAME );
			char buffo[length];
			char maxplayers[12];
			char curplayers[12];
			buffo[0] = 0;
			Uint32To10Str( maxplayers, MAX_PLAYERS );
			Uint32To10Str( curplayers, dumbcraft_playercount );
			mstrcatp( buffo, pingjson1, length );
			mstrcat( buffo, MOTD_NAME, length );
			mstrcatp( buffo, pingjson2, length );
			mstrcat( buffo, maxplayers, length );
			mstrcatp( buffo, pingjson3, length );
			mstrcat( buffo, curplayers, length );
			mstrcatp( buffo, pingjson4, length );
			StartSend();
			Sbyte( 0x00 );
			Sstring( buffo, -1 );
			DoneSend();

			p->need_to_send_playerlist = 0;
		}

		//If we didn't finish getting all the broadcast data last time
		//We must do that now.
		if( p->has_logged_on )
		{
			if( p->did_not_clean_out_broadcast_last_time )
				goto now_sending_broadcast;
			else
			{
				//From the game portion
				PlayerUpdate();
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
		//  p->next_chunk_to_load = 1..? for all chunks*16, one per packet (needs to update rows). Then 
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
			if( p->y < 0 ) p->need_to_respawn = 1;
		}

		//I'm worried about things overflowing here, should we consider some mechanism to help prevent this?

		//I used to do things here, it's a useful place to stick things that need to happen every tick..
		//I don't really use it anymore
		if( p->tick_since_update )
		{
			p->tick_since_update = 0;
		}


		if( p->need_to_respawn )
		{
			p->x = (1<<FIXEDPOINT)/2;
			p->y = 100*(1<<FIXEDPOINT);
			p->stance = p->y + (1<<FIXEDPOINT);
			p->z = (1<<FIXEDPOINT)/2;
			p->need_to_send_lookupdate = 1;
			p->need_to_respawn = 0;
		}

		if( p->need_to_send_lookupdate )
		{
			StartSend();
			Sbyte(0x34); //1.16.5 //Player Position And Look (clientbound)
			Sdouble( p->x );
			Sdouble( p->y );
			Sdouble( p->z );
			Sfloat( p->yaw );
			Sfloat( p->pitch );
			Sbyte(0x00); //xyz absolute
			Svarint( 0 );	//Teleport ID
			DoneSend();

			p->need_to_send_lookupdate = 0;
		}

		//We're just logging in!
		if( p->need_to_spawn )
		{
			uint8_t i;

			//Newer versions need not send this, maybe?
			StartSend();
			Sbyte( 0x24 );  //1.16.5 (Join Game)
			Sint( (uint8_t)(playerid + PLAYER_LOGIN_EID_BASE) );
			Sbyte( 0 ); // Is hardcore
			Sbyte( GAMEMODE ); //creative
			Sbyte( 255 ); // Previous gamemode
			Sbyte( 1 ); //World count
			Sstring( "minecraft:overworld", 19 ); // World name

			//Dimension Codec
			//	minecraft:dimension_type
			//	minecraft:worldgen/biome

			//TODO:
			//{
			//	"minecraft:dimension_type": {
			//		type: "minecraft:dimension_type",
			//		value: [
			//			{
			//			    name: "minecraft:overworld",
			//			    id: 0,
			//			    element: {
			//			        piglin_safe: 0b,
			//			        natural: 1b,
			//			        ambient_light: 0.0f,
			//			        has_skylight: 1b,
			//			        bed_works: 1b,
			//			        effects: "minecraft:overworld",
			//				},
			//			},
			//		],
			//	},
			//	"minecraft:worldgen/biome": {
			//		type: "minecraft:worldgen/biome",
			//		value: [
			//			{
			//			    name: "minecraft:ocean",
			//			    id: 0,
			//			    element: {
			//			        precipitation: "rain",
			//			        effects: {
			//			            sky_color: 8103167,
			//			            water_fog_color: 329011,
			//			            fog_color: 12638463,
			//			            water_color: 4159204,
			//			            mood_sound: {
			//			                tick_delay: 6000,
			//			                offset: 2.0d,
			//			                sound: "minecraft:ambient.cave",
			//			                block_search_extent: 8
			//			            }
			//			        },
			//			        depth: -1.0f,
			//			        temperature: 0.5f,
			//			        scale: 0.1f,
			//			        downfall: 0.5f,
			//			        category: "ocean"
			//			    }
			//			},
			//		],
			//	}
			//}

			//Also need dimension

			Sbyte( 0x00 );
			Sbyte( 0x00 );

			Sstring( "minecraft:overworld", 19 ); // World name
			Slong( 0xdeadbeef ); //Hashed seed.
			Sbyte( MAX_PLAYERS );
			Svarint( 8 ); //render distance in chunks.
			Sbyte( 0 ); //Reduce debug info?
			Sbyte( 0 );	//Immediate respawn.

			Sbyte( 0 );	//hard coded debug world.
			Sbyte( 0 );	//is flat
			DoneSend();

			p->need_to_spawn = 0;

			p->next_chunk_to_load = 1;
			p->has_logged_on = 1;
			p->just_spawned = 1;  //For next time round we send to everyone

			//Show us the rest of the players
			for( i = 0; i < MAX_PLAYERS; i++ )
			{
				if( i != playerid && Players[i].active )
				{
					SSpawnPlayer( i );
				}
			}

		}
		if( p->custom_preload_step )
		{
			DoCustomPreloadStep( );
			p->custom_preload_step = 0;
			p->need_to_respawn = 1;
			p->player_is_up_and_running = 1;
			//This is when we checkin to the updates. (after we've sent the map chunk updates)
			p->outcirctail = GetCurrentCircHead();
		}


		//This is triggered when players want to actually join.
		if( p->need_to_login )
		{
			StartSend();
			Sbyte( 0x03 ); //1.16.5 //Set compression threshold
			Svarint( 8192 ); //Arbitrary, so we only hit it when we send chunks.
			DoneSend();

			p->set_compression = 1;

			StartSend();
			Sbyte( 0x02 ); //1.16.5 //Login success
			Sencuuid( playerid + PLAYER_LOGIN_EID_BASE );
			p->need_to_login = 0;
			Sstring( (const char*)p->playername, -1 );
			DoneSend();

			//Do this, it is commented out for other reasons.
			p->need_to_spawn = 1;
		}


		if( p->need_to_send_keepalive )
		{

			StartSend();
			Sbyte( 0x1f ); //1.16.5 Keepalive
			Slong( dumbcraft_tick );
			DoneSend();
			p->need_to_send_keepalive = 0;
		}


		if( p->has_logged_on && !p->doneupdatespeed )
		{
			UpdatePlayerSpeed( p->running?RUNSPEED:WALKSPEED );
			p->doneupdatespeed = 1;
		}

now_sending_broadcast:
		//Apply any broadcast messages ... if we just spawned, then there's nothing to send.
		if( p->player_is_up_and_running )
		{
			p->did_not_clean_out_broadcast_last_time = UnloadCircularBufferOnThisClient( &p->outcirctail );
		}

		EndSend();
	}
	dumbcraft_playercount = localplayercount;
}


//This function should only be called ~10x/second
void TickServer()
{
	dumbcraft_tick++;

#ifdef INCLUDE_ANNOUNCE_UTILS
	if( ( dumbcraft_tick & 0xf ) == 0 )
	{
		SendAnnounce( );
	}
#endif

	//Everything in here should be broadcast to all players.
	StartupBroadcast();

	GameTick();

	for( playerid = 0; playerid < MAX_PLAYERS; playerid++ )
	{
		struct Player * p = &Players[playerid];
		if( !p->active ) continue;

		//Send a keep-alive every so often
		if( ( dumbcraft_tick & 0x2f ) == 0 )
		{
			p->need_to_send_keepalive = 1;
		}

		if( p->just_spawned )
		{
			p->just_spawned = 0;
			SSpawnPlayer( playerid );

			DoneBroadcast();
			p->outcirctail = GetCurrentCircHead(); //If we don't, we'll see ourselves.
			StartupBroadcast();
		}

		if( p->x != p->ox || p->y != p->oy || p->stance != p->os || p->z != p->oz )
		{
			//int16_t diffx = p->x - p->ox;
			//int16_t diffy = p->y - p->oy;
			//int16_t diffz = p->z - p->oz;
			//if( diffx < -127 || diffx > 127 || diffy < -127 || diffy > 127 || diffz < -127 || diffz > 127 )
			{
				StartSend();
				Sbyte( 0x56 );  //1.16.5 (Entity Teleport)
				Svarint( (uint8_t)(playerid + PLAYER_EID_BASE) );
				Sdouble( p->x );
				Sdouble( p->y );
				Sdouble( p->z );
				Sbyte( p->nyaw );
				Sbyte( p->npitch );
				Sbyte( ONGROUND );
				DoneSend();
			}
			/*else
			{
				StartSend();
				Sbyte( 0x26 ); //Updated (Entity Look And Relative Move)
				Svarint( (uint8_t)(playerid + PLAYER_EID_BASE) );
				Sshort( diffx*128 );
				Sshort( diffy*128 );
				Sshort( diffz*128 );
				Sbyte( p->nyaw );
				Sbyte( p->npitch );
				Sbyte( ONGROUND );
				DoneSend();
				p->op = p->pitch; p->ow = p->yaw;
			}*/

			StartSend();
			Sbyte( 0x3a ); //1.16.5 "Entity Head Look"
			Svarint( (uint8_t)(playerid + PLAYER_EID_BASE) );
			Sbyte( p->nyaw );
			DoneSend();

			p->ox = p->x;
			p->oy = p->y;
			p->oz = p->z;
			p->os = p->stance;
		}

		if( p->pitch != p->op || p->yaw != p->ow )
		{

			StartSend();
			/*
			//XXX Was this dropped by 1-15-2
			Sbyte( 0x27 ); //Entity Look
			Svarint( (uint8_t)(playerid + PLAYER_EID_BASE) );
			Sbyte( p->nyaw );
			Sbyte( p->npitch );
			Sbyte( ONGROUND );
			DoneSend();
			*/

			StartSend();
			Sbyte( 0x3A ); //1.16.5 (Entity Head Look)
			Svarint( (uint8_t)(playerid + PLAYER_EID_BASE) );
			Sbyte( p->nyaw );
			DoneSend();

			p->op = p->pitch; p->ow = p->yaw;
		}

		p->tick_since_update = 1;

		PlayerTickUpdate( );
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
//	printf( "Remove: %d\n", playerid );
	Players[playerno].active = 0;
	//todo send 0xff command
}

void DumpRemain()
{
	if( !cmdremain ) return;
//	printf( "(+%d)", cmdremain );
	while( cmdremain-- )
	{
//		printf( "%02x ", Rbyte() );
		Rbyte();
	}

	return;
}

//From user to dumbcraft (you call)
void GotData( uint8_t playerno )
{
#ifdef DEBUG_DUMBCRAFT
	static uint8_t lastcmd;
#endif
	uint8_t i8;
	uint16_t i16;
	playerid = playerno;
	struct Player * p = &Players[playerid];
	uint8_t * chat = 0;
	uint8_t chatlen = 0;
//	uint8_t skip_cmd = 0;

	//This is where we read in a packet from a client.
	//You can send to the broadcast cicular buffer, but you
	// MAY NOT! send back to the sender unless you wait till the end
	//even then that is a prohibitively bad idea!

	while( CanRead() )
	{
		cmdremain = 0xffff;
		cmdremain = Rvarint();

		if( p->set_compression )
		{
			uint8_t compsize = Rvarint();
			if( compsize )
			{
				//Compressed packet. Dump it!		
				DumpRemain();
				continue;
			}
		}

		uint8_t cmd = dcrbyte();

//		printf( "CMD: 0x%02x  REMAIN: %d COMP: %d\n", cmd, cmdremain, p->set_compression );


		if( p->handshake_state == 0 )
		{
			if( cmd == 0 )
			{
				i16 = Rvarint();
				if( i16 != PROTO_VERSION )
				{
#ifdef DEBUG_DUMBCRAFT
					printf("WARNING: wrong version; got: %d; expected %d\n", i16, PROTO_VERSION );
#endif
					//ForcePlayerClose( playerid, 'v' );				
				}
				Rstring( 0, 0 ); //server
				Rshort(); //port
				p->handshake_state = Rvarint();
#ifdef DEBUG_DUMBCRAFT
				printf("Client switched mode to: %d\n", p->handshake_state );
#endif
			}
		}
		else if( p->handshake_state == 1 ) //Status
		{
			switch( cmd )
			{
				case 0x00: //Request
					p->need_to_send_playerlist = 1;					
					break;
				case 0x01: //Ping
					Rbuffer( p->playername, 8 );
					p->need_to_reply_to_ping = 1;
					break;
				default:
#ifdef DEBUG_DUMBCRAFT
					printf( "Unknown status request (%d)\n", cmd );
#endif
					break;
			}
		}
		else if( p->handshake_state == 2 )
		{
			switch( cmd )
			{
			case 0x00:
				Rstring( (char*)p->playername, MAX_PLAYER_NAME-1 );
				p->playername[MAX_PLAYER_NAME-1] = 0;
				p->need_to_login = 1;
				p->handshake_state = 3;
#ifdef DEBUG_DUMBCRAFT
				printf( "Player \"%s\" Login, switching handshake_state.\n", p->playername );
#endif

				break;
			default:
#ifdef DEBUG_DUMBCRAFT
				printf( "Confusing packet for mode 2.\n" );
#endif
				break;
			}
		}


		switch( cmd )
		{
		case 0x00: //Teleport confirm (ignore)
			break;
		//case 0x1A: //Animation for hand waving or other stuff.  (TODO)
		//	break;
		//case 0x14: //Entity Action
		//	break;
		case 0x10: //1.16.5 //Keep alive
			//p->need_to_send_keepalive = 1;
			Rshort();
			//keep alive?
			//p->keepalive_id = Rint();
			break;

		case 0x03: //1.16.5 Handle chat
			i16 = Rvarint();

			chatlen = ((i16)<MAX_CHATLEN)?i16:MAX_CHATLEN;
			chat = alloca( chatlen+1 );
			i8 = 0;

			while( i16-- )
			{
				char c = dcrbyte();
				if( i8 < chatlen )
				{
					chat[i8++] = c;
				}
			}
			//chatlen++;
			chat[i8] = 0;
			break;

		case 0x15: //1.16.5 Player Movement //On-ground, client sends this to an annyoing degree.
			p->onground = dcrbyte();
			break;

		case 0x12: //1.16.5 Player position
			p->x = Rdouble();
			p->y = Rdouble();
			p->z = Rdouble();
			p->onground = dcrbyte();
			break;

		case 0x13: //1.16.5 //Player Position And Rotation
			p->x = Rdouble();
			p->y = Rdouble();
			//p->stance = Rdouble();
			p->z = Rdouble();
		case 0x14: //1.16.5  Player Rotation
			p->yaw = Rfloat();
			p->pitch = Rfloat();
			p->nyaw = p->yaw/45;    //XXX TODO PROBABLY SLOW seems to add <256 bytes, though.  Is there a better way?
			p->npitch = p->pitch/45;//XXX TODO PROBABLY SLOW
			p->onground = dcrbyte();
			break;
		case 0x1B: //1.16.5 player digging.
		{
			uint8_t status = Rvarint(); //action player is taking against block
			uint8_t x, y, z;
			Rposition( &x, &y, &z );
			uint8_t face = dcrbyte(); //which face?
#ifdef NEED_PLAYER_BLOCK_ACTION
			PlayerBlockAction( status, x, y, z, face );
#else
			face = face; status = status; //Ignore warnings
#endif
			break;
		}
		//case 0x2A:
		//{
		//	Players arms swinging.
		//}
#ifdef NEED_PLAYER_CLICK
		case 0x2E: //1.16.5 //Player Block Placement
		{
			uint8_t hand = Rvarint(); //action player is taking against block
			uint8_t x, y, z;
			Rposition( &x, &y, &z );
			uint8_t face = Rvarint(); //which face?

			//Ignore cursor xyz & insideblock
			Rfloat(); Rfloat(); Rfloat(); dcrbyte();

			if( hand == 0 ) PlayerClick( x, y, z, face );
			break;
		}
		case 0x2F: //1.16.5	//"Use Item" / Used when right-clicking an action up in the sky.
		{
			PlayerUse( Rvarint() ); //action player is taking against block
			break;
		}
#endif
#ifdef NEED_SLOT_CHANGE
		case 0x3F:  //1.16.5 Held item change
			PlayerChangeSlot( Rshort() );
			break;
#endif
		case 0x04:  //1.16.5 Client Status
			switch( dcrbyte() )
			{
			case 0x00: //perform respawn.
				p->need_to_respawn = 1;
				break;
				
			default:
				break;
			}
			break;

		case 0x0B://1.16.5 plugin message
			Rstring( 0,0 );
			Rdump( Rshort() );
			break;

		default:
#ifdef DEBUG_DUMBCRAFT
			printf( "UCMD: (LAST) %02x - NOW: %02x\n", lastcmd, cmd );
			printf( "UPKT:\n" );
			DumpRemain();
#endif
			break;
		}

#ifdef DEBUG_DUMBCRAFT
		lastcmd = cmd;
#endif
		DumpRemain();
	}

	if( chatlen )
	{
		uint8_t pll = strlen( (char*)p->playername );

		if( ClientHandleChat( (char*)chat, chatlen ) )
		{
			StartupBroadcast();			
			StartSend();
			Sbyte( 0x0E ); //1.16.5 Chat Message (clientbound)
			Svarint( chatlen + pll + 2 + 10 + 2 );
			Sbuffer( (const uint8_t*)"{\"text\":\"<", 10 );
			Sbuffer( p->playername, pll );
			Sbyte( '>' ) ;
			Sbyte( ' ' ) ;
			Sbuffer( chat, chatlen );
			Sbyte( '"' );
			Sbyte( '}' );
			Sbyte( 0 );
			DoneSend();
			DoneBroadcast();
		}
	}
}

