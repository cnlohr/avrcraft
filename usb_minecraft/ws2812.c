#include "ws2812.h"

void SendBufferWS( uint8_t * buffer, int count )
{
	int i;
	uint8_t mask, v;
	for( i = 0; i < count; i++ )
	{
		SEND_WS( (*buffer++) );
		SEND_WS( (*buffer++) );
		SEND_WS( (*buffer++) );
	}
}

void SetManyWS( uint8_t r, uint8_t g, uint8_t b, int count )
{
	int i;
	uint8_t mask, v;
	for( i = 0; i < count; i++ )
	{
		SEND_WS( g );
		SEND_WS( r );
		SEND_WS( b );
	}
}

