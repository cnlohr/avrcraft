#include "http.h"
#include <tcp.h>
#include <avr_print.h>
#include <basicfat.h>
#include <string.h>
#include <util10.h>
#include <avr/pgmspace.h>

#ifdef INCLUDE_HTTP_SERVER

//Before moving the HTTP over here aone 16328
//After moving it was 16400... What a price we pay for prettyness of code.
//hmm >:|
//260/895
//
//102/895
//But the size went up to 16,410

#define POP enc424j600_pop8()

static void HTTPHandleInternalCallback( uint8_t i );
static void InternalStartHTTP( uint8_t id );

void HTTPInit( uint8_t id )
{
	struct HTTPConnection * h = &HTTPConnections[id];
	h->state = TCP_STATE_WAIT_METHOD;
	h->state_deets = 0;
	h->is_dynamic = 0;
	h->clusterno = 0;
	h->bytesleft = 0;
	h->timeout = 0;

	h->is404 = 0;
	h->isdone = 0;
	h->isfirst = 0;
}

void HTTPClose( uint8_t id )
{
	struct HTTPConnection * h = &HTTPConnections[id];

	h->state = TCP_STATE_NONE;
	RequestClosure( id );
}


void HTTPGotData( uint8_t id, uint16_t len )
{
	uint8_t c;
	struct HTTPConnection * h = &HTTPConnections[id];
	h->timeout = 0;

	while( len-- )
	{
		c = POP;
	//	sendhex2( h->state ); sendchr( ' ' );
		switch( h->state )
		{
		case TCP_STATE_WAIT_METHOD:
			if( c == ' ' )
			{
				h->state = TCP_STATE_WAIT_PATH;
				h->state_deets = 0;
			}
			break;
		case TCP_STATE_WAIT_PATH:
			h->pathbuffer[h->state_deets++] = c;
			if( h->state_deets == MAX_PATHLEN )
			{
				h->state_deets--;
			}
			
			if( c == ' ' )
			{
				h->state = TCP_STATE_WAIT_PROTO; 
				h->pathbuffer[h->state_deets-1] = 0;
				h->state_deets = 0;
			}
			break;
		case TCP_STATE_WAIT_PROTO:
			if( c == '\n' )
			{
				h->state = TCP_STATE_WAIT_FLAG;
			}
			break;
		case TCP_STATE_WAIT_FLAG:
			if( c == '\n' )
			{
				h->state = TCP_STATE_DATA_XFER;
				InternalStartHTTP( id );
			}
			else if( c != '\r' )
			{
				h->state = TCP_STATE_WAIT_INFLAG;
			}
			break;
		case TCP_STATE_WAIT_INFLAG:
			if( c == '\n' )
			{
				h->state = TCP_STATE_WAIT_FLAG;
				h->state_deets = 0;
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
		struct HTTPConnection * h = &HTTPConnections[i];
		switch( h->state )
		{
		case TCP_STATE_DATA_XFER:
			if( TCPCanSend( i ) )
			{
				if( h->is_dynamic )
				{
				//	HTTPCustomCallback( i );
				}
				else
				{
					HTTPHandleInternalCallback( i );
				}
			}
			break;
		case TCP_WAIT_CLOSE:
			if( TCPCanSend( i ) )
				HTTPClose( i );
			break;
		default:
			if( h->timeout++ > HTTP_SERVER_TIMEOUT )
				HTTPClose( i );
		}
	}

}


static void PushStr( const char * msg )
{
	uint8_t r;

	do
	{
		r = pgm_read_byte(msg++);
		if( !r ) break;
		enc424j600_push8( r );
	} while( 1 );

/*
	do
	{
		r = *(msg++);
		if( !r ) break;
		enc424j600_push8( r );
	} while( 1 );
*/
}

#define PSTRx PSTR

struct FileInfo filedescriptors[HTTP_CONNECTIONS];

static void HTTPHandleInternalCallback( uint8_t conn )
{
	uint16_t i, bytestoread;
	struct HTTPConnection * h = &HTTPConnections[conn];

	if( h->isdone )
	{
		HTTPClose( conn );
		return;
	}
	if( h->is404 )
	{
		TCPs[conn].sendtype = ACKBIT | PSHBIT;
		StartTCPWrite( conn );
		PushStr( PSTRx("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\nFile not found.") );
		EndTCPWrite( conn );
		h->isdone = 1;
		return;
	}
	if( h->isfirst )
	{
		char stto[10];
		uint8_t slen = strlen( h->pathbuffer );
		const char * k;
		TCPs[conn].sendtype = ACKBIT | PSHBIT;
		StartTCPWrite( conn );
		//TODO: Content Length?  MIME-Type?
		PushStr( PSTRx("HTTP/1.1 200 Ok\r\nConnection: close\r\nContent-Length: ") );
		Uint32To10Str( stto, h->bytesleft );
		enc424j600_pushblob( stto, strlen( stto ) );
		PushStr( PSTRx( "\r\nContent-Type: " ) );
		//Content-Type?
		while( slen && ( h->pathbuffer[--slen] != '.' ) );
		k = &h->pathbuffer[slen+1];
		if( strcmp( k, "mp3" ) == 0 )
		{
			PushStr( PSTRx( "audio/mpeg3" ) );
		}
		else
		{
			PushStr( PSTRx( "text/html" ) );
		}

		PushStr( PSTRx( "\r\n\r\n" ) );
		EndTCPWrite( conn );
		h->isfirst = 0;
		return;
	}

	StartTCPWrite( conn );
	StartReadFAT( &filedescriptors[conn] );

	bytestoread = ((h->bytesleft)>512)?512:h->bytesleft;
	for( i = 0; i < bytestoread; i++ )
	{
		enc424j600_push8( read8FAT() );	
	}

	EndReadFAT();
	EndTCPWrite( conn );

	h->bytesleft -= bytestoread;

	if( !h->bytesleft )
		h->isdone = 1;

}

static void InternalStartHTTP( uint8_t conn )
{
	uint8_t i;
	struct HTTPConnection * h = &HTTPConnections[conn];
	const char * path = &h->pathbuffer[0];

	if( h->pathbuffer[0] == '/' )
		path++;

	h->clusterno = FindClusterFileInDir( path, ROOT_CLUSTER, -1, &h->bytesleft );
	h->bytessofar = 0;

	sendchr( 0 );
	sendstr( "Getting: " );
	sendstr( path );
	sendhex4( h->clusterno );
	sendchr( '\n' );

	if( h->clusterno < 0 )
	{
		h->is_dynamic = 1;
//		HTTPCustomStart( conn );
	}
	else
	{
		InitFileStructure( &filedescriptors[conn], h->clusterno );
		h->isfirst = 1;
		h->isdone = 0;
		h->is404 = 0;
		h->is_dynamic = 0;
	}

//	sendstr( "Internal Start HTTP\n" );
}


#endif
