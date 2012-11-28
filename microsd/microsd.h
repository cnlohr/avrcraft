//Copyright 2012 <>< Charles Lohr, All Rights Reserved, May be licensed under MIT/x11 or newBSD licenses, you choose.

#ifndef _MICROSD_H
#define _MICROSD_H

//Uses ~4 bytes of static ram

#include <stdint.h>

#define SDMISOP 5
#define SDCLKP  6
#define SDMOSIP 7

#define SDMISO _BV( SDMISOP )
#define SDCLK  _BV( SDCLKP )
#define SDMOSI _BV( SDMOSIP )
#define SDPORT PORTD
#define SDPIN  PIND
#define SDDDR  DDRD

#define SDCSEN { DDRB |= _BV(0); PORTB &= ~_BV(0); }
#define SDCSDE { DDRB |= _BV(0); PORTB |= _BV(0); }

//returns nonzero if failure.
unsigned char initSD();

//returns nonzero if failure.
unsigned char startSDwrite( uint32_t address );
void pushSDwrite( unsigned char c );
//nonzero indicates failure
unsigned char endSDwrite();

//returns nonzero if failure.
unsigned char startSDread( uint32_t address );
unsigned char popSDread();
void endSDread();

//Send a specified number of 0xFF's bytes.
void dumpSD( short count );

extern short opsleftSD;

void testReadBlock();

#endif

