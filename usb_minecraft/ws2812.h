#ifndef _WS2812_H
#define _WS2812_H

#include <avr/io.h>
#include <stdint.h>


#ifndef NOOP
#define NOOP asm volatile("nop" ::)
#endif

//Yuck, let's just WS this sucker.
#define SEND_WS( var ) \
				DDRD |= _BV(3); \
				mask = 0x80; \
				v = var; \
				while( mask ) \
				{ \
					if( mask & v )  \
					{ \
						PORTD |= _BV(3);  mask>>=1;   \
						NOOP; NOOP; NOOP; NOOP; \
						NOOP; NOOP; NOOP; NOOP; \
						PORTD &= ~_BV(3);\
					} \
					else \
					{ \
						PORTD |= _BV(3); NOOP; \
						PORTD &= ~_BV(3); \
						mask>>=1; \
						NOOP; NOOP; NOOP; NOOP; \
					} \
					\
				}

void SetManyWS( uint8_t r, uint8_t g, uint8_t b, int count );
void SendBufferWS( uint8_t * buffer, int count );


#endif

