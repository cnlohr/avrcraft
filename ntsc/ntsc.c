//Copyright 2011 <>< Charles Lohr, under the MIT/x11 License.

//http://www.kolumbus.fi/pami1/video/pal_ntsc.html

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include "ntsc.h"
/*

//NOTE: This system is for FIRST FIELD ONLY! (i.e. 240p)

//Fs = 3.579545375 MHz
//h =                  63.55555 us (officially)
//h = 229/3.579545375  63.97461 us (us)

//Control frames  (sline = 0, cline = 1...etc)

//1-6 (DOUBLE TIME) (4.7us / 59.3us) OR (2.3us / 29.7us in reality)
//7-12 backwards
//13-18 forwards
//then, move on!

//1, 2, 3 = (low for  2.3us, high for 29.5us) (TWICE per line)
//4, 5, 6 = (low for 27.1us, high for  4.7us) (TWICE per line)
//7, 8, 9 = (low for  2.3us, high for 29.5us) (TWICE per line)

//Frames 10..19 are freebies.

//20...262 (1...243) are REAL frames (sline)   (cline = 0)
*/


unsigned char cline;
unsigned char sline;
unsigned char frame;

#define FREQ_REL (((float)F_CPU)/((float)28636363))

//In order to detect off-clock quanta, our TMR1 must be divisible by 2.
//I think this is wrong.
//#define INTVEL( x ) ( ((int)((int)x/(int)4))*4)
#define INTVEL( x ) ((int)x)


//Enable corrective timing to reduce jiter from the AVR clock quanta - be careful, this has some overhead associated with it.
#define CORRECTIVE_TIMING

#ifdef CORRECTIVE_TIMING
#define EXACT_TCNT_FOR_SYNC (INTVEL(110*FREQ_REL) + 0)
#else
#define EXACT_TCNT_FOR_SYNC (INTVEL(126*FREQ_REL) + 0)
#endif

//Triggered on new line
ISR(TIMER1_COMPA_vect )
{
	//NTSC_PORT = SYNC_LEVEL;
	MAKE_NTSC_SYNC;

	if( cline )
	{

		cline++;

		//We were ending an even frame...
		if( cline == 2 )
		{
			OCR1B = 66*FREQ_REL; // back to regular sync.
		}

		if( cline == 8 )
		{
			//Go high 27.1us later.
			OCR1B = 774*FREQ_REL;  //was 776
		}
		else if( cline == 14 )
		{
			//Go high 2.3us later
			OCR1B = 64*FREQ_REL; //was 66
		}
#ifdef INTERLACE
		else if( cline == 19 && (frame & 1) )
		{
			//Slow the clock back down.
			OCR1A = INTVEL(1831*FREQ_REL); //WAS 1832
			OCR1B = EXACT_TCNT_FOR_SYNC; //Actually /should/ be 134 but we need to preempt it.
		}
		else if( cline == 18 && !(frame & 1) )
		{
			//Slow the clock back down.
			OCR1A = INTVEL(1831*FREQ_REL); //WAS 1832
			OCR1B = EXACT_TCNT_FOR_SYNC; //Actually /should/ be 134 but we need to preempt it.
//			cline += 4;
		}
#else
		else if( cline == 20 )
		{
			//Slow the clock back down.
			OCR1A = INTVEL(1831*FREQ_REL); //WAS 1832
			OCR1B = EXACT_TCNT_FOR_SYNC; //Actually /should/ be 134 but we need to preempt it.
		}
#endif
		else if( cline == 30 )
		{
			cline = 0;
			sline = 1;
		}
	}
	else
	{
		//Handle rendering of line.

		//Is this right?
		if( sline == 242 )
		{
#ifdef INTERLACE
			if( frame & 1 )
			{
				//Ending an odd frame
				cline = 2;
				sline = 0;
				OCR1A = 1830*FREQ_REL/2;  //First bunch of lines are super quick on sync.  (WAS 1832/2)
				OCR1B = 66*FREQ_REL;	 //With upshot quickly afterward. (2.3us) (was 66)
			}
			else
			{
				//Ending an even frame
				cline = 1;
				sline = 0;
				OCR1A = 1830*FREQ_REL/2;  //First bunch of lines are super quick on sync.  (WAS 1832/2)
				OCR1B = 66*FREQ_REL;	 //With upshot quickly afterward. (2.3us) (was 66)
			}
#else
			cline = 2;
			sline = 0;
			OCR1A = 1830*FREQ_REL/2;  //First bunch of lines are super quick on sync.  (WAS 1832/2)
			OCR1B = 66*FREQ_REL;	 //With upshot quickly afterward. (2.3us) (was 66)
#endif

			frame++;
		}
		else
		{
			sline++;
		}
	}
}

const uint8_t nops[4] = { 1, 1, 1, 1 };

//Partway through line  (note: only called in Vblank)
ISR(TIMER1_COMPB_vect )
{
	//XXX: TODO: Improve clock stabilization algorithm 
//	volatile uint8_t t = nops[TCNT1L];
//	do { } while( --t );

#ifdef CORRECTIVE_TIMING 

//Timing based on TCNT1L (%0)
/*
	asm voltile( "\
\n\t	push r16 \
\n\t	push r17 \
\n\t	lds r16, %2 \
\n\t    cpi r16, %3 \
\n\t    brne lblout \
\n\t	lds r16, %0 \
\n\t    andi r16, 3 \
\n\t    dec r16 \
\n\t    breq lbl1a \
\n\t    dec r16 \
\n\t    breq lbl1b \
\n\t    dec r16 \
\n\t    breq lbl1c \
\n\t    nop \n nop \nnop \n nop \n  \
\n\t    rjmp lblout \
\n\t lbl1a: \
\n\t    nop \n nop \nnop \n nop \n nop \n nop \n  \
\n\t    rjmp lblout \
\n\t lbl1b: \
\n\t    nop \n nop \n nop \n nop \n nop \n nop \n nop \n  \
\n\t    rjmp lblout \
\n\t lbl1c: \
\n\t    nop \n nop \n nop \n nop \n nop \n nop \n   \
\n\t lblout:\
\n\t	pop r17 \
\n\t	pop r16 \
	" : : "i" (&TCNT1L), "i" ((uint8_t)EXACT_TCNT_FOR_SYNC + 56), "i" (&OCR1B), "i" ((uint8_t)EXACT_TCNT_FOR_SYNC) );
*/

#define TIMING_OFFSET 1

	asm volatile( "\
\n\t	push r16 \
\n\t	push r17 \
\n\t    push r30 \
\n\t    push r31 \
\n\t	lds r16, %0 \
\n\t    add r16, %1 \
\n\t    subi r16, -2 \
\n\t    andi r16, 3 \
\n\t    call toloop /*For some reason we can't load Z with the labels??) */ \
\n\t toloop: \
\n\t    pop r31 \
\n\t    pop r30  \
\n\t    adiw r30, (localwaits - toloop - 5) \
\n\t    add r30, r16 \
\n\t    adc r31, __zero_reg__ \
\n\t    ijmp \
\n\t localwaits: \
\n\t    nop \n nop \n nop \n nop \n \
\n\t lblout: \
\n\t    pop r31 \
\n\t    pop r30 \
\n\t	pop r17 \
\n\t	pop r16 \
\n\t	" : : "i" (&TCNT1L), "i" ((uint8_t)TIMING_OFFSET), "i" (&OCR1B), "i" ((uint8_t)EXACT_TCNT_FOR_SYNC) );
/*
\n\t    add r30, r16 \
\n\t    adc r31, __zero_reg__ \
\n\t	lds r16, %2 \
\n\t    cpi r16, %3 \
\n\t    brne lblout \
\n\t    ijmp  \


*/

#endif

	MAKE_NTSC_BLANK;

	_delay_us(4.7*FREQ_REL);
//	TCNT2 = 0;
//	OCR2B = 1;  //1 makes it the most vivid.
//	DDRD |= _BV(3*FREQ_REL);
//	_delay_us(2.5*FREQ_REL);
//	DDRD &= ~_BV(3*FREQ_REL);
//	NTSC_PORT = BLANK_LEVEL;
//	_delay_us(2*FREQ_REL);


	if( cline )
	{
		if( cline >= 20 || (cline & 1 ) )
		{
			VSync();
		}
	}
	else
	{
		VideoLine( );
		VSync();
	}
}




void SetupNTSC()
{
	NTSC_SETUP;

	//Timers
	TCNT1 = 0;
	OCR1A = 1832*FREQ_REL;	//Setup line timer 1 (note this is for each line)
	OCR1B = 500*FREQ_REL;
	TCCR1A = 0;	//No PWMs.
	TCCR1B = _BV(CS10) | _BV(WGM12);  // no prescaling
	TIMSK1 = _BV(OCIE1A) | _BV(OCIE1B); //Enable overflow mark

}


