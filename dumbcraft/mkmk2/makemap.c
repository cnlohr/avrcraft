#include <stdio.h>
#include <malloc.h>

#include <stdint.h>

int MakeVarint( uint8_t * dat, int num )
{
	int r = 0;
cont:
	dat[r] = num & 0x7f;
	fprintf( stderr, "%d %x\n", num, dat[r] );
	if( num > 0x7f )
	{
		dat[r++] |= 0x80;
		num >>= 7;
		goto cont;
	}

	return r+1;
}


uint32_t PushBits( uint8_t * mapdata, uint32_t maplen, uint32_t data, int bits )
{
	static uint32_t v;
	static uint8_t pl;

	if( bits == 0 )
	{
		if( pl )
		{
			//Flush out
			uint32_t * mo = (uint32_t*) &mapdata[maplen];
			*mo = v;
			maplen += 4;			
		}
		pl = 0; v = 0;
		return maplen;
	}

	int px = (32-bits-pl);
	if( px > 0 )
		v |= data<<px;
	else
		v |= data>>(-px);
	//fprintf( stderr, "%08x %d %d  (%d)\n", v, pl, bits, 31-bits-pl );
	pl += bits;

	if( pl >= 32 )
	{
		uint32_t * mo = (uint32_t*)&mapdata[maplen];
		*mo = v;
		pl-=32;
		v = data<<(32-bits-pl);
		//fprintf( stderr, "+%08x %d %d  (%d)\n", v, pl, bits, 32-bits-pl );
		//Whatever.  Something's wrong.  Don't know what yet.
		maplen += 4;
	}

	return maplen;
}


int main()
{
	uint8_t * mapdata = malloc( 1000000 );
	int maplen = 0;
	int cell = 0;
	int r = 0;
	int x, y;

	mapdata[maplen++] = 16; //# of bits per block.  NOTE THE PACKET START (SEE BELOW) this dos revent the palette from existing.
	mapdata[maplen++] = 0; //No palette.

	maplen += MakeVarint(&mapdata[maplen],(16*16*16/2));

	for( y = 0; y < 16; y++ )
	for( x = 0; x < 16; x++ )
	{
		for( cell = 0; cell < 16; cell++ ) //block
		{
			mapdata[maplen++] = 0x04;
			mapdata[maplen++] = 0x00;
		}
	}

/*
	for( y = 0; y < 16; y++ )
	for( x = 0; x < 16; x++ )
	{
		for( cell = 0; cell < 8; cell++ ) //other stuff?
		{
			mapdata[maplen++] = 0x00;
			mapdata[maplen++] = 0x00;
		}
	}
*/

	//Block light
	for( cell = 0; cell < 16*16*16/2; cell++ )
	{
		mapdata[maplen++] = 0x00;
	}

	for( cell = 0; cell < 16*16*16/2; cell++ )
	{
		mapdata[maplen++] = 0x00;
	}

	for( cell = 0; cell < 16*16; cell++ ) //biome
		mapdata[maplen++] = 0x00;

	uint8_t * buffer = malloc( 1000000 );
	int buffpl = 0;
	buffer[buffpl++] = 0x22; //https://wiki.vg/Protocol#Chunk_Data
	buffer[buffpl++] = 0;	buffer[buffpl++] = 0;	buffer[buffpl++] = 0;	buffer[buffpl++] = 0; //Chunk X
	buffer[buffpl++] = 0;	buffer[buffpl++] = 0;	buffer[buffpl++] = 0;	buffer[buffpl++] = 0; //Chunk Z
	buffer[buffpl++] = 1; //ground-up (will need biome info)
	buffer[buffpl++] = 0x08; //bit mask - only one chunk shall be loaded. This _should_ be a varint.

	buffpl += MakeVarint( &buffer[buffpl], maplen );

	fwrite( buffer, buffpl, 1, stdout );
	fwrite( mapdata, maplen, 1, stdout );

	//Zero following for NBT Tags
	buffpl = 0;
	buffer[buffpl++] = 0x00;
	fwrite( buffer, buffpl, 1, stdout );

	return 0;
}

