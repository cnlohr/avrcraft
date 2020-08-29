#include "dumbutils.h"
#include "dumbcraft.h"
#include <util10.h>
#include <string.h>

void UpdateSlot( uint8_t window, uint8_t slot, uint8_t count, uint16_t id, uint8_t damage )
{
	StartSend();
	Sbyte( 0x17 );  //Updated (But rest of packet might be wrong)
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
	Sbyte( 0x0F ); //UPDATED
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
	Sbyte( 0x00 );  //1.15.2 Spawn Entity
	Svarint( eid );
//	Suuid( eid );
	Svarint( type );
	Sdouble( x );
	Sdouble( y );
	Sdouble( z );
	Sbyte( 0 );
	Sbyte( 0 );
//	Sbyte( 0 );
	Sint( 0 ); //data
	Sshort( 0 );
	Sshort( 0 );
	Sshort( 0 );
	DoneSend();
}

void EntityUpdatePos( uint16_t entity, uint16_t x, uint16_t y, uint16_t z, uint8_t yaw, uint8_t pitch )
{
	StartSend();
/*
	Sbyte( 0x26 ); //UPDATED, MAYBE!
	Svarint( entity );
	Sshort( x );
	Sshort( y );
	Sshort( z );
	Sfloat( yaw );
	Sfloat( pitch );
	Svarint( 1 ); //On ground
	DoneSend();
*/
	Sbyte( 0x2A ); //1.15.2 Entity Position and Rotation
	Svarint( entity );
	Sdouble( x );
	Sdouble( y );
	Sdouble( z );
	Sbyte( yaw );
	Sbyte( pitch );
	Svarint( 1 );
	DoneSend();
}


//Update a sign at a specific location with a string and a numerical value.
void SignUp( uint8_t x, uint8_t y, uint8_t z, const char* st, uint8_t val )
{
	char stmp1[5];
	char stmp2[5];

	Uint8To10Str( stmp1, val );
	Uint8To16Str( stmp2+2, val );
	stmp2[0] = '0'; stmp2[1] = 'x';
	SignTextUp( x, y, z, stmp1, stmp2 );
}

void InternalSendPosition (uint8_t x, uint8_t y, uint8_t z )
{
	//uint32_t slp = ((uint32_t)x & 0x3FFFFFF) << 38 | ((uint32_t)y & 0xFFF) << 26 | ((uint32_t)z & 0x3FFFFFF);
	Sbyte( 0x00 ); //bits 56...
	Sbyte( 0x00 ); //bits 48... (Would be x>>10...) but we're limited in size.
	Sbyte( (x>>2) ); //bits 40...
	Sbyte( ((x & 0x03)<<6) | ((y>>6)&0x3f) ); //bits 32...  XXX Not sure if Y is right.
	Sbyte( (y & 0x3f)<<2 ); //bits 24...
	Sbyte( 0x00 ); //bits 16...
	Sbyte( 0x00 ); //bits 8 ...
	Sbyte( z ); //bits 0 ...
}

void SendNBTString( const char * str )
{
	unsigned short sl = strlen( str );
	Sshort( sl );
	Sbuffer( (const uint8_t*)str, sl );
}

void SignTextUp( uint8_t x, uint8_t y, uint8_t z, const char * line1, const char * line2 )
{
	printf( "TODO: SIGN TEXT UP\n" );

#if 0
	int len1 = strlen( line1 );
	int len2 = strlen( line2 );

	//Big thanks in this section goes to Na "Sodium" from #mcdevs on freenode IRC.

	StartSend();
	Sbyte( 0x09 ); //[UPDATED]  (Update entity)
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
#endif
}


//Update a block at a given x, y, z (good for 0..255 in each dimension)
void SblockInternal( uint8_t x, uint8_t y, uint8_t z, uint8_t bt, uint8_t meta )
{
	uint16_t tblockmeta = (bt<<4) | meta;

	printf( "BC: %d %d %d\n", x, y, z );

	StartSend();
	Sbyte(0x0C);  //1.15.2  "Block Change"
	InternalSendPosition( x, y, z );
	Svarint( tblockmeta ); //block type
	DoneSend();
}
