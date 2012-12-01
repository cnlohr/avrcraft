#include "http.h"
#include <tcp.h>
#include <avr_print.h>
#include <basicfat.h>
#include <string.h>
#include <util10.h>
#include <avr/pgmspace.h>

#ifdef INCLUDE_HTTP_SERVER

//Extra ~120 bytes of code, but ~30% overall performance boost!
#define FAST_SECTOR_TRANSFER

#define POP enc424j600_pop8()
static void InternalStartHTTP( );
struct HTTPConnection * curhttp;

void HTTPInit( uint8_t id, uint8_t socket )
{
	curhttp = &HTTPConnections[id];
	memset( curhttp, 0, sizeof( struct HTTPConnection ) );
	curhttp->state = TCP_STATE_WAIT_METHOD;
	curhttp->socket = socket;
}

void HTTPClose( )
{
	curhttp->state = TCP_STATE_NONE;
	RequestClosure( curhttp->socket );
}


void HTTPGotData( uint8_t id, uint16_t len )
{
	uint8_t c;
	curhttp = &HTTPConnections[id];
	curhttp->timeout = 0;

	while( len-- )
	{
		c = POP;
	//	sendhex2( h->state ); sendchr( ' ' );
		switch( curhttp->state )
		{
		case TCP_STATE_WAIT_METHOD:
			if( c == ' ' )
			{
				curhttp->state = TCP_STATE_WAIT_PATH;
				curhttp->state_deets = 0;
			}
			break;
		case TCP_STATE_WAIT_PATH:
			curhttp->pathbuffer[curhttp->state_deets++] = c;
			if( curhttp->state_deets == MAX_PATHLEN )
			{
				curhttp->state_deets--;
			}
			
			if( c == ' ' )
			{
				curhttp->state = TCP_STATE_WAIT_PROTO; 
				curhttp->pathbuffer[curhttp->state_deets-1] = 0;
				curhttp->state_deets = 0;
			}
			break;
		case TCP_STATE_WAIT_PROTO:
			if( c == '\n' )
			{
				curhttp->state = TCP_STATE_WAIT_FLAG;
			}
			break;
		case TCP_STATE_WAIT_FLAG:
			if( c == '\n' )
			{
				curhttp->state = TCP_STATE_DATA_XFER;
				InternalStartHTTP( );
			}
			else if( c != '\r' )
			{
				curhttp->state = TCP_STATE_WAIT_INFLAG;
			}
			break;
		case TCP_STATE_WAIT_INFLAG:
			if( c == '\n' )
			{
				curhttp->state = TCP_STATE_WAIT_FLAG;
				curhttp->state_deets = 0;
			}
			break;
		case TCP_STATE_DATA_XFER:
			//Ignore any further data?
			len = 0;
			break;
		case TCP_WAIT_CLOSE:
			HTTPClose( id );
			break;
		default:
			break;
		};
	}
}

void HTTPTick()
{
	uint8_t i;
	for( i = 0; i < HTTP_CONNECTIONS; i++ )
	{
		curhttp = &HTTPConnections[i];
		switch( curhttp->state )
		{
		case TCP_STATE_DATA_XFER:
			if( TCPCanSend( curhttp->socket ) )
			{
				if( curhttp->is_dynamic )
				{
					HTTPCustomCallback( );
				}
				else
				{
					HTTPHandleInternalCallback( );
				}
			}
			break;
		case TCP_WAIT_CLOSE:
			if( TCPCanSend( curhttp->socket ) )
				HTTPClose( );
			break;
		default:
			if( curhttp->timeout++ > HTTP_SERVER_TIMEOUT )
				HTTPClose( i );
		}
	}

}


void PushPGMStr( const char * msg )
{
	uint8_t r;

	do
	{
		r = pgm_read_byte(msg++);
		if( !r ) break;
		enc424j600_push8( r );
	} while( 1 );
}

#define PSTRx PSTR

void HTTPHandleInternalCallback( )
{
	uint16_t i, bytestoread;

	if( curhttp->isdone )
	{
		HTTPClose( );
		return;
	}
	if( curhttp->is404 )
	{
		TCPs[curhttp->socket].sendtype = ACKBIT | PSHBIT;
		StartTCPWrite( curhttp->socket );
		PushPGMStr( PSTRx("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\nFile not found.") );
		EndTCPWrite( curhttp->socket );
		curhttp->isdone = 1;
		return;
	}
	if( curhttp->isfirst )
	{
		char stto[10];
		uint8_t slen = strlen( curhttp->pathbuffer );
		const char * k;
		TCPs[curhttp->socket].sendtype = ACKBIT | PSHBIT;
		StartTCPWrite( curhttp->socket );
		//TODO: Content Length?  MIME-Type?
		PushPGMStr( PSTRx("HTTP/1.1 200 Ok\r\nConnection: close") );

		if( curhttp->bytesleft != 0xffffffff )
		{
			PushPGMStr( PSTRx("\r\nContent-Length: ") );
			Uint32To10Str( stto, curhttp->bytesleft );
			enc424j600_pushblob( stto, strlen( stto ) );
		}
		PushPGMStr( PSTRx( "\r\nContent-Type: " ) );
		//Content-Type?
		while( slen && ( curhttp->pathbuffer[--slen] != '.' ) );
		k = &curhttp->pathbuffer[slen+1];
		if( strcmp( k, "mp3" ) == 0 )
		{
			PushPGMStr( PSTRx( "audio/mpeg3" ) );
		}
		else
		{
			PushPGMStr( PSTRx( "text/html" ) );
		}

		PushPGMStr( PSTRx( "\r\n\r\n" ) );
		EndTCPWrite( curhttp->socket );
		curhttp->isfirst = 0;
		return;
	}

	StartTCPWrite( curhttp->socket );
#ifdef FAST_SECTOR_TRANSFER
	StartReadFAT_SA( &curhttp->data.filedescriptor );

	bytestoread = ((curhttp->bytesleft)>512)?512:curhttp->bytesleft;
	for( i = 0; i < bytestoread; i++ )
	{
		enc424j600_push8( popSDread() );	
	}

	endSDread();
	FATAdvanceSector();
#else
	StartReadFAT( &curhttp->data.filedescriptor );

	bytestoread = ((curhttp->bytesleft)>512)?512:curhttp->bytesleft;
	for( i = 0; i < bytestoread; i++ )
	{
		enc424j600_push8( read8FAT() );	
	}

	EndReadFAT();
#endif

	EndTCPWrite( curhttp->socket );

	curhttp->bytesleft -= bytestoread;

	if( !curhttp->bytesleft )
		curhttp->isdone = 1;

}

static void InternalStartHTTP( )
{
	int32_t clusterno;
	uint8_t i;
	const char * path = &curhttp->pathbuffer[0];

	if( curhttp->pathbuffer[0] == '/' )
		path++;

	if( path[0] == 'd' && path[1] == '/' )
	{
		curhttp->is_dynamic = 1;
		curhttp->isfirst = 1;
		curhttp->isdone = 0;
		curhttp->is404 = 0;
		HTTPCustomStart();
		return;
	}


	clusterno = FindClusterFileInDir( path, ROOT_CLUSTER, -1, &curhttp->bytesleft );
	curhttp->bytessofar = 0;

	sendchr( 0 );
	sendstr( "Getting: " );
	sendstr( path );
	sendhex4( clusterno );
	sendchr( '\n' );

	if( clusterno < 0 )
	{
		sendstr( "CLUSTERNO BAD\n" );
		curhttp->is404 = 1;
		curhttp->isfirst = 1;
		curhttp->isdone = 0;
		curhttp->is_dynamic = 0;
	}
	else
	{
		InitFileStructure( &curhttp->data.filedescriptor, clusterno );
		curhttp->isfirst = 1;
		curhttp->isdone = 0;
		curhttp->is404 = 0;
		curhttp->is_dynamic = 0;
	}

//	sendstr( "Internal Start HTTP\n" );
}


#endif
