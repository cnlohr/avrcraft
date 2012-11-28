//Copyright 2012 <>< Charles Lohr, All Rights Reserved, May be licensed under MIT/x11 or newBSD licenses, you choose.

#include "ntsc.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>

#include "ntscfont.h"

#define DO_2X
#define STUPID_FAST_ASM
//#define SLOW_SOME
#define WIDTH 40
#define LINES 24

//XXX TODO: Consider slicing apart the font into separate scan lines.


volatile char framebuffer[WIDTH*LINES];

void VSync()
{
}

void VideoLine( )
{
	uint8_t charsleft;
	uint16_t rline;
	uint8_t pline;
	uint8_t thisline = sline;

	int i;
	SPCR &= ~( _BV(SPE) | _BV(MSTR) );
	DDRB |= _BV(3);
	PORTB &= ~_BV(3);
	_delay_us(.5);
//	PORTB |= _BV(3);
//	SPDR = 0x00;
	SPCR |= _BV(SPE) | _BV(MSTR) | _BV(DORD);// | _BV(SPR0);
#ifdef DO_2X
SPSR |= _BV(SPI2X);
#endif

	if( thisline < 19 || thisline > 211 )
	{
		goto end;
	}
	thisline -= 20;
	charsleft = WIDTH;
	
	rline = thisline >> 3;
	pline = (thisline)&0x7;

	rline *= WIDTH;

	SPDR = 0x00;

#ifdef STUPID_FAST_ASM
	asm volatile( "\
\n\tLMainLoop: \
\n\t ld r17, X+ \
\n\t mov r30, r17 \
\n\t clr r31 \
\n\t lsl r30 \
\n\t lsl r30 \
\n\t rol r31 \
\n\t lsl r30 \
\n\t rol r31 \
\n\t or r30, %2 \
\n\t subi r30, lo8(-(%5)) \
\n\t sbci r31, hi8(-(%5)) \
\n\t lpm r16, Z \
\n\t sbrs r17, 7 \
\n\t com r16 "
#ifdef SAFETY_ON_ASM_WRITE
"\n\tRetrySend: \
\n\t sts %0, r16 \
\n\t lds r17, %4 \
\n\t sbrc r17, 6 \
\n\t rjmp RetrySend"
#elif defined( SLOW_SOME )
"\n\t nop \n\t nop \
\n\t nop\n\t nop\n\t nop\n\t nop\n\t nop\n\t nop\n\t nop\n\t nop\n\t nop\n\t nop \
\n\t sts %0, r16 \n"
#else
"\n\t sts %0, r16 \n"
#endif
"\n\t dec %3\
\n\t brne LMainLoop \
" : : "i" (&SPDR), "x" (&framebuffer[rline]), "d" (pline), "d" (charsleft), "i" (&SPSR), "i" (&font_8x8_data) :
 "r30", "r31", "r16", "r17" );
#else
	do
	{
		unsigned char c = framebuffer[rline++];
		const unsigned char * st = &font_8x8_data[ ((c)<<3) | pline ];
		if( (--charsleft) == 0 ) break;
		c = ~pgm_read_byte( st );
		do
		{
			SPDR = c;
		} while( SPSR & _BV(WCOL ) );
	} while( 1 );
#endif

end:
	_delay_us(2);
	PORTB &= ~_BV(3);
	SPCR &= ~( _BV(SPE) | _BV(MSTR) );
}

