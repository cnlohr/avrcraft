#include "dumbutils.h"
#include "dumbcraft.h"
#include <util10.h>
#include <string.h>

void UpdateSlot( uint8_t window, uint8_t slot, uint8_t count, uint16_t id, uint8_t damage )
{
	StartSend();
	Sbyte( 0x2F );
	Sbyte( window ); 
	Sshort( slot );
	Sshort( id );
	Sbyte( count );
	Sbyte( damage );
	Sbyte( 0x00 ); //??????
	Sbyte( 0xff );
	Sbyte( 0xff ); 
	DoneSend();
}

void GPChat( const char * text )
{
	uint16_t chatlen = strlen( text );
	StartSend();
	Sbyte( 0x02 );
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
	Sbyte( 0x14 );
	Sint( eid );
	DoneSend();

	StartSend();
	Sbyte( 0x0F  );
	Svarint( eid );
	Sbyte( type );
	Sint( x );
	Sint( y );
	Sint( z );
	Sbyte( 0 );
	Sbyte( 0 );
	Sbyte( 0 );
	Sshort( 0 );
	Sshort( 0 );
	Sshort( 0 );
	Sbyte( 0x00 ); //Byte, Index 0
	Sbyte( 0x00 ); //no parameters.
	Sbyte( 127 );
	DoneSend();
}

void EntityUpdatePos( uint16_t entity, uint16_t x, uint16_t y, uint16_t z )
{
	StartSend();
	Sbyte( 0x18 );
	Sint( entity );
	Sint( x );
	Sint( y );
	Sint( z );
	Sbyte( 0 );
	Sbyte( 0 );
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

void SignTextUp( uint8_t x, uint8_t y, uint8_t z, const char * line1, const char * line2 )
{
	StartSend();
	Sbyte( 0x33 ); //sign update
	Sint( x ); Sshort( y ); Sint( z );
	Sstring( line1, -1 );
	Sstring( "", 0 );
	Sstring( line2, -1 );
	Sstring( "", 0 );
	DoneSend();
}


//Update a block at a given x, y, z (good for 0..255 in each dimension)
void SblockInternal( uint8_t x, uint8_t y, uint8_t z, uint8_t bt, uint8_t meta )
{
	StartSend();
	Sbyte(0x23);  //NEW
	Sint( x ); //x
	Sbyte( y ); //y
	Sint( z ); //z
	Svarint( bt ); //block type
	Sbyte( meta ); //metadata
	DoneSend();
}
