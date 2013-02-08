#ifndef _MFS_H
#define _MFS_H

#include <stdint.h>

//Multi-purpose PROGMEM-based FS (for AVRs)

struct MFSFileInfo
{
	uint16_t filelen;
	uint16_t offset;
};



//Returns 0 on succses.
//Returns size of file if non-empty
//If positive, populates mfi.
//Returns -1 if can't find file or reached end of file list.
int8_t MFSOpenFile( const char * fname, struct MFSFileInfo * mfi );
void MFSStartReadFile( struct MFSFileInfo * f );
uint8_t MFSRead8();
void MFSDoneRead();



#endif


