#include "dumbutils.h"
#include "dumbcraft.h"
#include <util10.h>
#include <string.h>

void UpdateSlot( uint8_t window, uint8_t slot, uint8_t count, uint16_t id, uint8_t damage )
{
	StartSend();
	Sbyte( 0x15 );  //1.16.5 "Set Slot" (But rest of packet might be wrong)
	Sbyte( window ); 
	Sshort( slot );

	//Slot data
	Sbyte( 1 ); //present
	Svarint( id );
	Sbyte( count );
	Sbyte( 0x00 );  //NBT (could have other properties)
	DoneSend();
}

void GPChat( const char * text )
{
	uint16_t chatlen = strlen( text );
	StartSend();
	Sbyte( 0x0E ); //1.16.5 Chat Message (clientbound)
	Svarint( chatlen + 11 );
	Sbuffer( (const uint8_t*)"{\"text\":\"", 9 );
	Sbuffer( (const uint8_t*)text, chatlen );
	Sbyte( '"' );
	Sbyte( '}' );
	DoneSend();
}



void SpawnEntity( uint16_t eid, uint8_t type, uint16_t x, uint16_t y, uint16_t z )
{
	StartSend();
	Sbyte( 0x02 );  //1.16.5 Spawn Living Entity
	Svarint( eid );
	Sencuuid( eid );
	Svarint( type );
	Sdouble( x );
	Sdouble( y );
	Sdouble( z );
	Sbyte( 0 );
	Sbyte( 0 );
	Sbyte( 0 ); //head pitch
	Sshort( 0 );
	Sshort( 0 );
	Sshort( 0 );
	DoneSend();
}

void EntityUpdatePos( uint16_t entity, uint16_t x, uint16_t y, uint16_t z, uint8_t yaw, uint8_t pitch )
{
	StartSend();
	Sbyte( 0x28 ); //1.16.5 Entity Position and Rotation
	Svarint( entity );
	Sdouble( x );
	Sdouble( y );
	Sdouble( z );
	Sbyte( yaw );
	Sbyte( pitch );
	Svarint( 1 ); //on ground
	DoneSend();
}

//Update a sign at a specific location with a string and a numerical value.
void SignUp( uint8_t x, uint8_t y, uint8_t z, const char* st, uint8_t val )
{
	char stmp[16];
	Uint8To10Str( stmp+5, val );
	Uint8To16Str( stmp+2, val );
	stmp[0] = '0'; stmp[1] = 'x'; stmp[4] = ':';
	SignTextUp( x, y, z, st, stmp );
}

void InternalSendPosition (uint8_t x, uint8_t y, uint8_t z )
{
	//NOTE: Could be switched to 16_t for x and z.

	//uint32_t slp = ((uint32_t)x & 0x3FFFFFF) << 38 | ((uint32_t)y & 0xFFF) << 26 | ((uint32_t)z & 0x3FFFFFF);
/* 1.11
	Sbyte( 0x00 ); //bits 56...
	Sbyte( 0x00 ); //bits 48... (Would be x>>10...) but we're limited in size.
	Sbyte( (x>>2) ); //bits 40...
	Sbyte( ((x & 0x03)<<6) | ((y>>6)&0x3f) ); //bits 32...  XXX Not sure if Y is right.
	Sbyte( (y & 0x3f)<<2 ); //bits 24...
	Sbyte( 0x00 ); //bits 16...
	Sbyte( 0x00 ); //bits 8 ...
	Sbyte( z ); //bits 0 ...
*/

	//1.15.2: ((x & 0x3FFFFFF) << 38) | ((z & 0x3 FF FF FF) << 12) | (y & 0xFFF)

#if 0
	Sbyte( 0x00 ); //bits 56...
	Sbyte( 0x00 ); //bits 48... (Would be x>>10...) but we're limited in size.
	Sbyte( (x>>2) ); //bits 40...
	Sbyte( ((x & 0x03)<<6) | ((y>>6)&0x3f) ); //bits 32...  XXX Not sure if Y is right.
	Sbyte( (y & 0x3f)<<2 ); //bits 24...
	Sbyte( 0x00 ); //bits 16...
#endif

	Sbyte( 0 );
	Sbyte( x>>10 );
	Sbyte( x>>2 );
	Sbyte( (x & 3)<<6 );

	Sbyte( z >> 12 );
	Sbyte( z >> 4 );
	Sbyte( (z & 0x0f)<<4 );
	Sbyte( y );
}

void SendNBTString( const char * str )
{
	unsigned short sl = strlen( str );
	Sshort( sl );
	Sbuffer( (const uint8_t*)str, sl );
}

void SignTextUp( uint8_t x, uint8_t y, uint8_t z, const char * line1, const char * line2 )
{
	int len1 = strlen( line1 );
	int len2 = strlen( line2 );

	//Big thanks in this section goes to Na "Sodium" from #mcdevs on freenode IRC.

	StartSend();
	Sbyte( 0x09 ); //[1.16.5]  (Block entity data)
	InternalSendPosition( x, y, z );
	Sbyte( 9 ); // "Set text on sign"

	Sbyte( 0x0a );
	Sshort( 0 ); //No name for compound

	Sbyte(1);	SendNBTString( "x" ); Sbyte(x);
	Sbyte(1);	SendNBTString( "y" ); Sbyte(y);
	Sbyte(1);	SendNBTString( "z" ); Sbyte(z);

	Sbyte( 8 );
	SendNBTString( "Text1" );
	Sshort( 11+ len1+14 );
	Sbuffer( (const uint8_t*)"{\"bold\":\"true\",\"text\":\"", 9+14 ); //http://minecraft.gamepedia.com/Commands#Raw_JSON_Text 
	Sbuffer( (const uint8_t*)line1, len1 );
	Sbuffer( (const uint8_t*)"\"}", 2 );
	
	Sbyte( 8 );
	SendNBTString( "Text3" );
	Sshort( 11+ len2 );
	Sbuffer( (const uint8_t*)"{\"text\":\"", 9 );
	Sbuffer( (const uint8_t*)line2, len2 );
	Sbuffer( (const uint8_t*)"\"}", 2 );

	Sbyte( 0x00 );	 //Compound end.

	DoneSend();
}


//Update a block at a given x, y, z (good for 0..255 in each dimension)
void SblockInternal( uint8_t x, uint8_t y, uint8_t z, uint16_t blockid )
{
	StartSend();
	Sbyte(0x0B);  //1.16.5  "Block Change"
	InternalSendPosition( x, y, z );
	Svarint( blockid ); //block type
	DoneSend();
}
