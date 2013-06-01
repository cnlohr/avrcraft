//Copyright 2012 <>< Charles Lohr, All Rights Reserved, May be licensed under MIT/x11 or newBSD licenses, you choose.

#include "ntsc.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include "video.h"
#include "ntscfont.h"

#define DO_2X
#define STUPID_FAST_ASM
//#define SLOW_SOME

//XXX TODO: Consider slicing apart the font into separate scan lines.


volatile char framebuffer[NTWIDTH*NTLINES];
unsigned char videobeep;
//unsigned char linescroll = 0; //Used for scrolling down on text.


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
	SPCR |= _BV(SPE) | _BV(MSTR) | _BV(DORD) | _BV(CPHA);// | _BV(SPR0);
#ifdef DO_2X
SPSR |= _BV(SPI2X);
#endif

	if( thisline < 19 || thisline > (NTLINES*8+19) ) //<211
	{
		if( thisline == 19 && videobeep ) videobeep--;
		goto end;
	}
	thisline -= 20;
	charsleft = NTWIDTH;
	
	rline = thisline >> 3;

//	rline += linescroll;
//	if( rline >= NTLINES ) rline -= NTLINES;

	pline = (thisline)&0x7;

	rline *= NTWIDTH;

	SPDR = 0x00;

#ifdef STUPID_FAST_ASM
	asm volatile( "\
\n\tLMainLoop: \
\n\t ld r17, X+ \
\n\t mov r18, r17 \
\n\t andi r18, 0x7f \
\n\t mov r30, r28 \
\n\t mov r31, r29 \
\n\t add r30, r18 \
\n\t adc r31,__zero_reg__ \
\n\t lpm r16, Z \
\n\t sbrs r17, 7 \
\n\t com r16 \
 "
#ifdef SAFETY_ON_ASM_WRITE
"\n\tRetrySend: \
\n\t sts %0, r16 \
\n\t lds r17, %3 \
\n\t sbrc r17, 6 \
\n\t rjmp RetrySend"
#elif defined( SLOW_SOME )
"\n\t nop \n\t nop \
\n\t nop\n\t nop\n\t nop\n\t nop\n\t nop\n\t nop\n\t nop\n\t nop\n\t nop\n\t nop \
\n\t sts %0, r16 \n"
#else
"\n\t sts %0, r16 \n"
#endif
"\n\t dec %2\
\n\t brne LMainLoop \
" : : "i" (&SPDR), "x" (&framebuffer[rline]), "d" (charsleft), "i" (&SPSR), "y" (&font_8x8_data[pline<<7]) : "r30", "r31", "r16", "r17" );
#else

	uint8_t * fbtl = &font_8x8_data[pline<<7];
	do
	{
		unsigned char c = framebuffer[rline++];
		const unsigned char * st = &fbtl[c];
		if( (--charsleft) == 0 ) break;
		c = pgm_read_byte( st );
		/*do
		{
			SPDR = c;
		} while( SPSR & _BV(WCOL ) );*/
		SPDR = c;

	} while( 1 );
#endif
end:
	_delay_us(.4);
	PORTB &= ~_BV(3);
	SPCR &= ~( _BV(SPE) | _BV(MSTR) );
}

