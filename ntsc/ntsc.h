//Copyright 2011 <>< Charles Lohr, under the MIT/x11 License.

#ifndef _NTSC_H
#define _NTSC_H

void SetupNTSC();

//Called per video line
void VideoLine();

//Called at VSYNC rate (possibly for some sort of sound system)
void VSync();


extern unsigned char cline;
extern unsigned char sline;
extern unsigned char frame;

#define NT_NOOP asm volatile("nop" ::)

#define MAKE_NTSC_SYNC  { DDRB |= _BV(4); SPCR &= ~_BV(MSTR); }
#define MAKE_NTSC_BLANK {  DDRB &= ~_BV(4); SPCR |= _BV(MSTR); }

#define  NTSC_SETUP \
{ \
	DDRB |= _BV(5) | _BV(3); \
	PORTB &= ~_BV(2); \
	DDRB |= _BV(2); \
	DDRB &= ~_BV(4); \
	PORTB &= ~_BV(4); \
	SPCR = _BV(SPE) | _BV(MSTR); \
	 \
}

//Optional?
//#define INTERLACE

#endif
