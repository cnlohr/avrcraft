#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

//Format:
//
// mfsfat[] = { 'f', 'i', 'l', 'e', '1', '.', 't', 'x', 't', 0, [reserved: 0], 
//   [reserved: 0], [start high], [start low], [length high], [length low], 'f', 'i', 'l', ...
//
// mfsdata[] = {...}
//
//
//Restrictions:  FAT cannot exceed 65536 bytes.
//               FS Data Size cannot exceed 65536 bytes.

unsigned char mfsfat[65536];
unsigned char mfsdata[65536];

unsigned short fatpointer = 0;
unsigned short datapointer = 0;

int main( int argc, char ** argv )
{
	int i;
	DIR           *d;
	struct dirent *dir;

	if( argc != 3 )
	{
		fprintf( stderr, "Error: [tool] [directory to pack] [output packed .c file]\n" );
		return -1;
	}

	d = opendir( argv[1] );

	if (!d)
	{
		fprintf( stderr, "Error: cannot open folder for packing.\n" );
		return -2;
	}

	while ((dir = readdir(d)) != NULL)
    {
		if( dir->d_type & DT_REG )
		{
			char thisfile[1024];
			struct stat buf;
			int dlen = strlen( dir->d_name );
			int sprret = snprintf( thisfile, 1023, "%s/%s", argv[1], dir->d_name );

			if( sprret > 1023 || sprret < 1 )
			{
				fprintf( stderr, "Error processing \"%s\" (snprintf)\n", dir->d_name );
				continue;
			}

			int statret = stat( thisfile, &buf );

			if( statret )
			{
				fprintf( stderr, "Error processing \"%s\" (stat)\n", dir->d_name );
				continue;
			}

		
			memcpy( &mfsfat[fatpointer], dir->d_name, dlen );
			fatpointer+= dlen;
			mfsfat[fatpointer++] = 0;

			mfsfat[fatpointer++] = 0; //Reserved
			mfsfat[fatpointer++] = 0;

			mfsfat[fatpointer++] = datapointer >> 8;
			mfsfat[fatpointer++] = datapointer & 0xFF;

			if( buf.st_size + datapointer > 65535 )
			{
				fprintf( stderr, "Error: no space left.\n" );
				return -1;
			}

			mfsfat[fatpointer++] = (buf.st_size) >> 8;
			mfsfat[fatpointer++] = (buf.st_size) & 0xFF;
			
			if( buf.st_size )
			{
				FILE * f = fopen( thisfile, "rb" );
				if( !f )
				{
					fprintf( stderr, "Error: cannot open \"%s\" for reading.\n", dir->d_name );
					return -9;
				}
				fread( &mfsdata[datapointer], 1, buf.st_size, f );
				fclose( f );
				datapointer += buf.st_size;
			}
		}
    }

    closedir(d);

	mfsfat[fatpointer++] = 0;

	FILE * f = fopen( argv[2], "w" );

	if( !f || ferror( f ) )
	{
		fprintf( stderr, "Error: cannot open \"%s\" for writing.\n", argv[2] );
	}

	fprintf( f, "#include <avr/pgmspace.h>\n\n" );
	fprintf( f, "const unsigned char PROGMEM mfsfat[] = {\n\t" );
	for( i = 0; i < fatpointer; i++ )
	{
		if( i && (!(i%16)) )
		{
			fprintf( f, "\n\t" );
		}
		fprintf( f, "0x%02x, ", mfsfat[i] );
	}
	fprintf( f, "\n};\n\nconst unsigned char PROGMEM mfsdata[] = {\n\t" );
	for( i = 0; i < datapointer; i++ )
	{
		if( i && (!(i%16)) )
		{
			fprintf( f, "\n\t" );
		}
		fprintf( f, "0x%02x, ", mfsdata[i] );
	}
	fprintf( f, "\n};\n\n" );
	fclose( f );

	return 0;
}


