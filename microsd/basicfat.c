//Copyright 2012 <>< Charles Lohr, All Rights Reserved, May be licensed under MIT/x11 or newBSD licenses, you choose.
//Based off of: http://staff.washington.edu/dittrich/misc/fatgen103.pdf

#include "basicfat.h"
#include "avr_print.h"
#include <alloca.h>

#include <avr/delay.h>

static uint32_t fat_partition_head;
static uint32_t first_data_sector;
static uint32_t first_fat_table_sector;
static uint8_t  cluster512s;  //# of 512-sets per cluster
//static uint32_t lastfatsector;
static struct FileInfo * lastfile;

static uint8_t  internal_8FAT() { return popSDread(); }
static uint32_t internal_32LEFAT();
static uint16_t internal_16LEFAT();

static unsigned char strNCcmp( const char * a, const char * b );

unsigned char openFAT()
{
	static struct FileInfo dummy;
	uint32_t BPB_TotSec32;
	uint32_t BPB_FATSz32;
	uint16_t BPB_BytesPerSec;
	uint8_t  BPB_SecPerClus;
	uint16_t BPB_RsvdSecCnt;
	uint8_t  BPB_NumFATs;
	uint8_t  BPB_RootEntCnt;
	uint8_t  IsFATOK;

	fat_partition_head = 0;
	dummy.clusterno = 0;
	dummy.byteno = 0;
	dummy.sectorno = 0;
	lastfile = &dummy;

	//We may do this twice, so we need to figure out if we're reading a partition table, or if it's an unpartitioned card.
try_readfat:

	IsFATOK = 1;

	if( startSDread( fat_partition_head ) )
	{
		sendstr( "Failed to read SD card\n" );
		return 4;
	}

	dumpSD( 11 ); //dump jump and description

	//XXX Consider doing sanity checks here for if the FAT is OK.
	BPB_BytesPerSec = internal_16LEFAT();
	BPB_SecPerClus  = internal_8FAT();
	BPB_RsvdSecCnt  = internal_16LEFAT();
	BPB_NumFATs     = internal_8FAT();
	BPB_RootEntCnt  = internal_16LEFAT();

	dumpSD( 13 ); //we don't care about any of these values.

	BPB_TotSec32    = internal_32LEFAT();
	BPB_FATSz32     = internal_32LEFAT();

	dumpSD( 42 ); //we don't care about this, either.

	//46 41 54 33 32 20 20 20 a.k.a. "FAT
	
	if( internal_32LEFAT() != 0x33544146 )
		IsFATOK = 0;

	if( internal_32LEFAT() != 0x20202032 )
		IsFATOK = 0;

	if( !IsFATOK )
	{
		if( fat_partition_head )
		{
			//Failed to find a valid FAT.
			endSDread();
			sendstr( "Could not find valid FAT32 entry.\n" );
			return 1;
		}

		//currently at byte 90
		//If we're actually a MBR, not a FAT, we will need to read out the partition table.

		dumpSD(0x164 + 8); //Jump to 0x1BE + 8 (we don't care about anything but the location info)
		
		fat_partition_head = internal_32LEFAT();
		sendstr( "Re-reading at: " );
		sendhex4( fat_partition_head );
		sendchr( '\n' );
		endSDread();
		if( fat_partition_head )
			goto try_readfat;
		sendstr( "Failure.  Cannot find first partition.\n" );
		return 3;
	}

	endSDread();

	//We have it!

	first_data_sector = BPB_RsvdSecCnt;
	while( BPB_NumFATs-- )
		first_data_sector += BPB_FATSz32;

	cluster512s = BPB_SecPerClus;

	first_fat_table_sector = BPB_RsvdSecCnt;

	sendstr( "Foundsector: " );
	sendhex4( (int)(first_data_sector&0xFFFF) );
	sendchr( '\n' );

	return 0;
}


uint32_t FindClusterFileInDir( const char * fname, uint32_t cluster, int16_t fileid, uint32_t * filelen )
{
	//Consider not using longfname here and just abort if we see something wrong.

	char  *  longfname;//[MAX_LONG_FILENAME];
	uint8_t i;
	uint8_t haslongfilename = 0;
	uint16_t entry;
	struct FileInfo dummy;

	dummy.clusterno = cluster;
	dummy.sectorno = 0;
	dummy.byteno = 0;

	StartReadFAT( &dummy );

	entry = 0;

	if( fileid >= 0 )
		longfname = fname;
	else
		longfname = alloca( MAX_LONG_FILENAME );

	do
	{
		uint8_t currententry[32];
		for( i = 0; i < 32; i++ )
		{
			currententry[i] = read8FAT();
		}

		//what are we dealing with?
		if( currententry[0] == 0xE5 ) continue;
		if( currententry[0] == 0 ) { endSDread(); return 0xffffffff; }
		//we're not implementing kanji.

		//Is this a long file name?
		if( currententry[0x0B] == 0x0f )
		{
			uint8_t entry = currententry[0] & 0x1f;

			//Anything wrong?
			if( entry < 1 || entry > 0x14 ) continue;

			entry--;
			entry *= 13;

			//filename too long.
			if( entry + 12 >= MAX_LONG_FILENAME ) continue;

			longfname[entry+0] = currententry[1];
			longfname[entry+1] = currententry[3];
			longfname[entry+2] = currententry[5];
			longfname[entry+3] = currententry[7];
			longfname[entry+4] = currententry[9];
			longfname[entry+5] = currententry[14];
			longfname[entry+6] = currententry[16];
			longfname[entry+7] = currententry[18];
			longfname[entry+8] = currententry[20];
			longfname[entry+9] = currententry[22];
			longfname[entry+10] = currententry[24];
			longfname[entry+11] = currententry[28];
			longfname[entry+12] = currententry[30];
			haslongfilename = 1;
			continue;
		}

		if( !haslongfilename )
		{
			uint8_t j = 0;

			for( i = 0; i < 8; i++ )
				if( ' ' == (longfname[j++] = currententry[i]) ) break;
			longfname[j-1] = '.';
			for( ; i < 11; i++ )
				if( ' ' == (longfname[j++] = currententry[i]) ) break;
			longfname[j-1] = 0;

			//XXX: Special: ignore any time we do a .. or .
			if( longfname[0] == '.' ) continue;
		}

		haslongfilename = 0;

		if( fileid == entry || ( fileid == -1 && strNCcmp( longfname, fname ) == 0 ) )
		{
			uint32_t ret = 0;
			ret = (uint32_t)currententry[26] | ((uint32_t)currententry[27] << 8) | ((uint32_t)currententry[20] << 16) | ((uint32_t)currententry[21]<<24);
			if( filelen )
			{
				*filelen = (uint32_t)currententry[28] | ((uint32_t)currententry[29]<<8) |
						((uint32_t)currententry[30]<<16) | ((uint32_t)currententry[31]<<24)|
						((currententry[11] & 0x10)?0xFFFFFFFF:0); ;
			}
			endSDread();

			return ret;
		}

		longfname[0] = 0;
		entry++;
	} while( 1 );

	endSDread();

	return 0xffffffff;
}


uint32_t ClusterToSector( int32_t cluster )
{
	return ( cluster - 2 ) * cluster512s + first_data_sector;
}

unsigned char StartReadFAT( struct FileInfo * file )
{
	uint16_t i;

	lastfile = file;
	
	uint8_t ret = startSDread( ClusterToSector( file->clusterno) + fat_partition_head + file->sectorno );
	if( ret )
	{
		sendstr( "General fault starting card read.\n" );
	}

	for( i = file->byteno; i; --i )  
	{
		popSDread();
	}

	return ret;
}

unsigned char StartReadFAT_SA( struct FileInfo * file )
{
	uint16_t i;

	lastfile = file;
	
	uint8_t ret = startSDread( ClusterToSector( file->clusterno) + fat_partition_head + file->sectorno );
	if( ret )
	{
		sendstr( "General fault starting card read.\n" );
	}


	return ret;
}

void FATAdvanceSector()
{
	lastfile->byteno = 0;
	lastfile->sectorno++;
	if( lastfile->sectorno == cluster512s )
	{
		uint32_t fatsector;
		//Advance cluster now. We will be reading out of the first FAT.
		lastfile->sectorno = 0;

		//Experimentally, our FAT starts at byte 0x4000 (or sector 20) why?
		fatsector = first_fat_table_sector + (lastfile->clusterno>>(9-2));//(ClusterToSector( 0 ) + fat_partition_head + (lastfile->clusterno>>(9-2)));
/*
		sendchr( 0 );
		sendstr( "Shift: " );
		sendhex4( fatsector>>16 );
		sendhex4( fatsector );
		sendchr( ' ' ); 
		sendhex4( first_fat_table_sector );
		sendchr( '+' );
		sendhex4( (lastfile->clusterno>>(9-2)) );
		sendchr( '@' );
		sendhex4(  (lastfile->clusterno<<2) & 0x1ff );
		sendchr( '\n' );
*/
		uint8_t ret = startSDread( fatsector + fat_partition_head );
		if( ret )
		{
			sendstr( "General fault starting card read for sector advancement.\n" );
			return;
		}

		dumpSD( (lastfile->clusterno<<2) & 0x1ff );
		lastfile->clusterno = internal_32LEFAT();

		endSDread();

		//XXX TODO: Check for end-of-chain here.

	}
	
}

void EndReadFAT()
{
	endSDread();
/*	if( lastfile->byteno >= 512 )
	{
		sendstr( "\nADVANCE IN FINAL\n" );
		AdvanceSector();
	}*/
	lastfile = 0;
}

void InitFileStructure( struct FileInfo * f, uint32_t cluster )
{
	f->clusterno = cluster;
	f->byteno = 0;
	f->sectorno = 0;
}

uint8_t  read8FAT()
{
	uint8_t ret = popSDread();

	if( lastfile )
		lastfile->byteno++;
/*
sendchr( 0 );
sendchr( '$' );
sendhex4( opsleftSD );*/


	if( opsleftSD <= 0 )
	{
		uint16_t i;
		endSDread();
		FATAdvanceSector();
		StartReadFAT( lastfile );
	}

	return ret;
}

static uint16_t internal_16LEFAT()
{
	uint16_t ret = popSDread();
	ret |= ((uint16_t)popSDread())<<8;
	return ret;
}

uint16_t read16LEFAT()
{
	uint16_t ret = read8FAT();
	ret |= ((uint16_t)read8FAT())<<8;
	return ret;
}

static uint32_t internal_32LEFAT()
{
	uint32_t ret = popSDread();
	ret |= ((uint32_t)popSDread())<<8;
	ret |= ((uint32_t)popSDread())<<16;
	ret |= ((uint32_t)popSDread())<<24;
	return ret;
}

uint32_t read32LEFAT()
{
	uint32_t ret = read8FAT();
	ret |= ((uint32_t)read8FAT())<<8;
	ret |= ((uint32_t)read8FAT())<<16;
	ret |= ((uint32_t)read8FAT())<<24;
	return ret;
}


static unsigned char strNCcmp( const char * a, const char * b )
{
	do
	{
		char ra = (*a >= 'a' && *a <= 'z')?(*a - 'a' + 'A'):*a;
		char rb = (*b >= 'a' && *b <= 'z')?(*b - 'a' + 'A'):*b;
		if( ra < rb ) return -1;
		else if( ra > rb ) return 1;
		else if( !ra ) return 0;
		a++; b++;
	} while( 1 );
}
