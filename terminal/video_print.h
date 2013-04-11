#ifndef _VIDEO_PRINT_H
#define _VIDEO_PRINT_H


#define setup_spi videotermsetup



void sendchr( char c );

#define sendstr( s ) { unsigned char rcnt; \
	for( rcnt = 0; s[rcnt] != 0; rcnt++ ) \
		sendchr( s[rcnt] ); }

void sendhex1( unsigned char i );
void sendhex2( unsigned char i );
void sendhex4( unsigned int i );
void videotermsetup( void );


#endif

