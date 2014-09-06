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


int main()
{
	uint8_t * mapdata = malloc( 1000000 );
	int maplen = 0;
	int cell = 0;
	int r = 0;
	int x, y;

/*
	for( cell = 0; cell < 16*16*16/2; cell++ )
	{
		mapdata[maplen++] = cell;
		mapdata[maplen++] = 0x20;
		mapdata[maplen++] = 0x02;
		mapdata[maplen++] = 0x00;
		mapdata[maplen++] = 0x0f;
	}

	for( cell = 0; cell < 16*16; cell++ )
	{
		mapdata[maplen++] = 1;
	}
*/



	for( y = 0; y < 16; y++ )
	for( x = 0; x < 16; x++ )
	{
		for( cell = 0; cell < 16; cell++ ) //block
		{
			mapdata[maplen++] = 0x20;//(x!=9)?(0x20):0;
			mapdata[maplen++] = 0x00;
		}
	}


	for( y = 0; y < 16; y++ )
	for( x = 0; x < 16; x++ )
	{
		for( cell = 0; cell < 8; cell++ ) //other stuff?
		{
			mapdata[maplen++] = 0x00;
			mapdata[maplen++] = 0x00;
		}
	}

	for( cell = 0; cell < 16*16; cell++ )
	{
		mapdata[maplen++] = 1;
	}


/*
	int x, y;
	for( y = 0; y < 16; y++ )
	for( x = 0; x < 16; x++ )
	{
		for( cell = 0; cell < 16; cell++ ) //block
			mapdata[maplen++] = cell&0x03;

		for( cell = 0; cell < 16/2; cell++ ) //meta
			mapdata[maplen++] = 0;

		for( cell = 0; cell < 16/2; cell++ ) //light
			mapdata[maplen++] = 0;

		for( cell = 0; cell < 16/2; cell++ ) //skylight
			mapdata[maplen++] = 0;

		for( cell = 0; cell < 16/2; cell++ ) //add
			mapdata[maplen++] = 0;
	}
*/
	for( cell = 0; cell < 16*16; cell++ ) //biome
		mapdata[maplen++] = 0x00;

	uint8_t * buffer = malloc( 1000000 );
	int buffpl = 0;
	buffer[buffpl++] = 0x21; //Packet
	buffer[buffpl++] = 0;	buffer[buffpl++] = 0;	buffer[buffpl++] = 0;	buffer[buffpl++] = 0; //Chunk X
	buffer[buffpl++] = 0;	buffer[buffpl++] = 0;	buffer[buffpl++] = 0;	buffer[buffpl++] = 0; //Chunk Z
	buffer[buffpl++] = 1; //ground-up
	buffer[buffpl++] = 0x00; //bit mask
	buffer[buffpl++] = 0x08; //bit mask

	buffpl += MakeVarint( &buffer[buffpl], maplen );

	fwrite( buffer, buffpl, 1, stdout );
	fwrite( mapdata, maplen, 1, stdout );

	return 0;
}

