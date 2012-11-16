#include <stdio.h>

int main( int argc, char ** argv )
{
	int i, j;
	for( i = 0; i < 8; i++ )
	{
		for( j = 0; j < 4096; j++ )
			putchar( (i<4)?2:0 );

		for( j = 0; j < 2048; j++ )
			putchar ( 0 ); //meta

		for( j = 0; j < 2048; j++ )
			putchar ( 0x00 ); //light

		for( j = 0; j < 2048; j++ )
			putchar ( 0xff ); //sky light

		for( j = 0; j < 2048; j++ )
			putchar ( 0x00 ); //add
	}
	for( j = 0; j < 256; j++ )
		putchar ( 1 ); //biome

	return 0;
/*
	if( argc != 2 )
	{
		fprintf( stderr, "Usage: [tool] [blocktype]\n" );
		return -1;
	}
	int x, y, z, i, j;
	for( i = 0; i < 1; i++ )
	{
		for( j = 0; j < 4096; j++ )
			putchar( atoi( argv[1] ) );

		for( j = 0; j < 2048; j++ )
			putchar ( 0 ); //meta

		for( j = 0; j < 2048; j++ )
			putchar ( 0x00 ); //light

		for( j = 0; j < 2048; j++ )
			putchar ( 0xff ); //sky light

		for( j = 0; j < 2048; j++ )
			putchar ( 0x00 ); //add
	}

//	for( j = 0; j < 256; j++ )
//		putchar ( 0 ); //biome
*/
}
