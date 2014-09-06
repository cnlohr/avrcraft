#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int MakeVarint( uint8_t * dat, int num )
{
	int r = 0;
cont:
	dat[r] = num & 0x7f;
	if( num > 0x7f )
	{
		dat[r++] |= 0x80;
		num >>= 7;
		goto cont;
	}

	return r+1;
}

int main( int argc, char ** argv )
{
	if( argc != 2 )
	{
		fprintf( stderr, "error: usage: [compacketize] [original, uncompressed size] < compressed file.\n" );
		return -1;
	}
	unsigned char * stbuff = malloc( 10000000 );
	int c;
	int ct = 0;
	while( (c = getchar()) != EOF )
	{
		stbuff[ct++] = c;
	}

	// ct  = size (compressed)
	// 

	//TODO Make varint.
	char varintout[8];
	char varinttop[8];
	int rlen = MakeVarint( varintout, atoi(argv[1]) );

	int plen = rlen + ct;
	int rplen = MakeVarint( varinttop, plen );
	fprintf( stderr, "RP: %d RL: %d CT: %d\n", rplen, rlen, ct );
	fwrite( varinttop, 1, rplen, stdout );
	fwrite( varintout, 1, rlen, stdout );
	fwrite( stbuff, 1, ct, stdout );
	
	return 0;
}

