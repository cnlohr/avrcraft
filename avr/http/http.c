#include "http.h"
#include <tcp.h>
#include <avr_print.h>

#ifdef INCLUDE_HTTP_SERVER

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
					HTTPCustomCallback( i );
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

static void HTTPHandleInternalCallback( uint8_t i )
{
	struct HTTPConnection * h = &HTTPConnections[i];
//	sendstr( "Internal Handle HTTP\n" );
}

static void InternalStartHTTP( uint8_t id )
{
	struct HTTPConnection * h = &HTTPConnections[id];
//	sendstr( "Internal Start HTTP\n" );
	h->is_dynamic = 1;
	HTTPCustomStart( id );
}


#endif
