//Copyright 2012 <>< Charles Lohr, All Rights Reserved, May be licensed under MIT/x11 or newBSD licenses, you choose.

//Basic FAT32 filesystem app
//It should be able to sense if it is talking to a partition table or not.
//If it is, it can only use the first partition.

//This can only use FAT32.

#ifndef _BASICFAT_H
#define _BASICFAT_H

#include "microsd.h"

//This functionality uses ~24 bytes of static ram.

//Don't make this too big, otherwise you'll blow your stack.
//The function it's in requires an additional ~50 bytes of stack already.
#define MAX_LONG_FILENAME 40

#define ROOT_CLUSTER 2

//This is only good for reading 512's at a time.
struct FileInfo
{
	uint32_t clusterno;
	uint16_t byteno:9;
	uint8_t  sectorno:7;
};

//returns 0 for success, nonzero if failure.
unsigned char openFAT();

//RETURN DATA: sector #, 0 if empty file, -1 if can't find or end of file list.
//if fileid >= 0, then the mode is it returns the entry #n.  it will write the long filename into fname if this is the case.
//if fileid == -1, then this is considered a "find this file" feature.
//You must make fname as big as MAX_LONG_FILENAME
//pass in cluster #2 if you want the root.
//this function ignores checksums on long filenames.
//filelen is optional. IF it is returning a folder *filelen == 0xFFFFFFFF
uint32_t FindClusterFileInDir( const char * fname, uint32_t cluster, int16_t fileid, uint32_t * filelen );

void InitFileStructure( struct FileInfo * f, uint32_t cluster );

//Convert a specified cluster to a sector.
uint32_t ClusterToSector( int32_t cluster );

//begin a read
unsigned char StartReadFAT( struct FileInfo * fi );

//Sector aligned, you must read exactly and only one sector using the SD card functions, not FAT functions
//This function is used for performance purposes when you know you'll be reading exactly 512 bytes of ram.
//You must call AdvanceSector when done.
unsigned char StartReadFAT_SA( struct FileInfo * fi );  
void FATAdvanceSector();  //Only call this if you are using ReadFAT_SA


uint8_t  read8FAT();
uint16_t read16LEFAT();
uint32_t read32LEFAT();

void EndReadFAT();



#endif

