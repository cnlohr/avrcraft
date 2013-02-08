#include "mfs.h"
#include <avr/pgmspace.h>

extern const unsigned char PROGMEM mfsfat[];
extern const unsigned char PROGMEM mfsdata[];
//const unsigned char * readptr;

uint16_t * offset;

int8_t MFSOpenFile( const char * fname, struct MFSFileInfo * mfi )
{
	const char * fatptr = mfsfat;
	uint8_t match;
	uint8_t i;
	char c;

	while( 1 )
	{
		i = 0;
		match = 1;
		while( c = pgm_read_byte( fatptr++ ) )
		{
			if( c != fname[i++] )
			{
				match = 0;
			}
		}
		if( fname[i] )
		{
			match = 0;
		}

		//No filename (end of FAT)
		if( i == 0 )
		{
			return -1;
		}

		fatptr += 2; //Skip reserved word.
		mfi->offset = pgm_read_byte( fatptr++ ) << 8;
		mfi->offset |= pgm_read_byte( fatptr++ );
		mfi->filelen = pgm_read_byte( fatptr++ ) << 8;
		mfi->filelen |= pgm_read_byte( fatptr++ );

		if( match )
		{
			return 0;
		}
	}
}

void MFSStartReadFile( struct MFSFileInfo * f )
{
//	readptr = &mfsdata[f->offset];
	offset = &f->offset;
}

uint8_t MFSRead8()
{	
	return pgm_read_word( &mfsdata[(*offset)++]);
}


