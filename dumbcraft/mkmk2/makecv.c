#include <stdio.h>

int main()
{
	int byteno = 0;
	//FILE * f = fopen( "map.gz", "rb" );

	printf( "uint8_t compeddata[] PROGMEM = {\n\t" );

	while( !feof( stdin ) )
	{
		int c = getc( stdin );
		if( c == EOF ) break;
		printf( "0x%02x, ", (unsigned char)c );
		byteno++;
		if( byteno == 16 )
		{
			byteno = 0;
			printf( "\n\t" );
		}
	}
	printf( "};\n" );
	return 0;
}
