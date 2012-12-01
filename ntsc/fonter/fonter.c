#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>

int main()
{
	int ch = 0;
	int line = 0;
	unsigned char * chdata;

	char version[1024];
	char comment[1024];
	int width, height, depth;

	FILE * f = fopen( "font.ppm", "rb" );
	if( !f )
	{
		fprintf( stderr, "Error: could not open font.ppm\n" );
		return -1;
	}

	fscanf( f, "%1023s\n", version );
	//fscanf( f, "%1023s\n", comment );
	for( ch = 0; ch < 1023; ch++ )
	{
		char c = fgetc( f );
		if( c == EOF )
		{
			fprintf( stderr, "Unexpected EOF.\n" );
			exit (-1 );
		}
		else if( c == '\n' )
		{
			break;
		}
		else
			comment[ch] = c;
	}
	comment[ch] = 0;
	fscanf( f, "%d %d\n", &width, &height );
	fscanf( f, "%d\n", &depth );
	chdata = malloc( width * height * 3 );
	fread( chdata, width * height * 3, 1, f );
	fclose( f );

	if( ( version[0] != 'P' || version[1] != '6' ) || width != 1024 || height != 8 || depth != 255 )
	{
		fprintf( stderr, "error: wrong parameters (Expecting PPM P6, COMMENT, 1024, 8, 255)\n" );
		fprintf( stderr, "  Got: '%s' '%s' %d %d %d\n", version, comment, width, height, depth );
		return -1;
	}

	printf( "//From http://overcode.yak.net/12\n" );
	printf( "#include <avr/pgmspace.h>\n\n" );
	printf( "const unsigned char font_8x8_data[] PROGMEM = {\n" );

	for( line = 0; line < 8; line++ )
	{

		printf( "\n\t" );

		for( ch = 0; ch < 128; ch++ )
		{
			int px = 0;
			int out;
			out = 0;

			unsigned char * lc = &chdata[(line * width + ch * 8) * 3];

			for( px = 0; px < 8; px++ )
			{
				out |= (1<<px)* ( (lc[px*3]<128)?1:0 );
			}

			printf( "0x%02x, ", out );
			if( (ch & 0x0f) == 0x0f ) printf( "\n\t" );
		}
//		if( ch >= 32 )
//			printf( "// '%c'\n", ch );
//		else
//			printf( "// %d\n", ch );
	}
	printf( "};\n" );
	return 0;
}
