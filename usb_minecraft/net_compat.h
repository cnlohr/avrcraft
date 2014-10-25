#ifndef _ENC424JCOMPAT_H
#define _ENC424JCOMPAT_H

//enc424j600 compat lib.
//We're using the IP library from AVRCraft... but we're using our own PHY+MAC

//TODO: Should probably use Timer1 (since we can't use Timer0)
//to make sure we send media detect pulses at the right points.
//Also, as a user, you cannot use the USI or Timer0

#include <stdint.h>
#include "eth_config.h"

void HandleUSB();

uint8_t IsSendBufferFree();

//You must define 'mymac'
extern unsigned char MyMAC[6];
extern unsigned char ETbuffer[ETBUFFERSIZE];
extern unsigned short ETsendplace;
//For telling where the current transaction started.
extern uint16_t sendbaseaddress;
extern unsigned short ETchecksum;

//return 0 if OK, otherwise nonzero.
int8_t et_init( const unsigned char * macaddy );

int8_t et_xmitpacket( uint16_t start, uint16_t len );

//This waits for 8ms, sends an autoneg notice, then waits for 8 more ms.
//Ordinarily this would if packets were processed and still ned processing, but
//that doesn't make sense for this driver.  Do not put this in a loop unto itself.
unsigned short et_recvpack();

//You have to write this! (Or the underlying IP core must)
void et_receivecallback( uint16_t packetlen );

//Finish up any reading. 							//CLOSURE
static inline void et_finish_callback_now() { }

//Raw, on-wire pops. (assuming already in read)
void et_popblob( uint8_t * data, uint8_t len );
void et_dumpbytes( uint8_t len );
uint16_t et_pop16();
uint8_t et_pop8();

//Raw, on-wire push. (assuming already in write)
void et_pushpgmstr( const char * msg );
void et_pushstr( const char * msg );
void et_pushblob( const uint8_t * data, uint8_t len );
void et_pushpgmblob( const uint8_t * data, uint8_t len );
static inline void et_push8( uint8_t d ) { if (ETsendplace < ETBUFFERSIZE ) ETbuffer[ETsendplace++] = d; }
static inline void et_pushzeroes( uint8_t nrzeroes ) { while( nrzeroes-- ) et_push8(0); }
void et_push16( uint16_t v );

//Start a new send.									//OPENING
static inline void et_startsend( uint16_t start ) { sendbaseaddress = ETsendplace = start; }

static inline uint16_t et_get_write_length() { return ETsendplace - sendbaseaddress + 6; } //XXX: Tricky - throw in extra 6 here because dstmac is included.

//End sending (calls xmitpacket with correct flags)	//CLOSURE
static inline void et_endsend() { et_xmitpacket( sendbaseaddress, ETsendplace - sendbaseaddress ); }

//Deselects the chip, can halt operation.
static inline void et_stopop() { }

//Start a checksum
void et_start_checksum( uint16_t start, uint16_t len );

//Get the results of a checksum (little endian)
static inline uint16_t et_get_checksum() { return ETchecksum; }

//Modify a word of memory (little endian)
static inline void et_alter_word( uint16_t address, uint16_t val ) { ETbuffer[address] = val>>8; ETbuffer[address+1] = val & 0xff; }

//Copy from one part of the enc to the other.
//Warning range_end is INCLUSIVE! You will likely want to subtract one.
//Double-warning.  On some MACs, range is ignored.
//void et_copy_memory( uint16_t to, uint16_t from, uint16_t length, uint16_t range_start, uint16_t range_end );
#define et_copy_memory COPY_NOT_AVAILABLE!


#define EERXRDPTL 1
#define EEGPWRPTL 2
//Low-level access (not available, except for above )
void et_write_ctrl_reg16( uint8_t addy, uint16_t value );
uint16_t et_read_ctrl_reg16( uint8_t addy );




#endif

/*
     Modification of "avrcraft" IP Stack.
    Copyright (C) 2014 <>< Charles Lohr
		CRC Code from: http://www.hackersdelight.org/hdcodetxt/crc.c.txt

    Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"),
	to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense,
	and/or sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
	in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
	CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
	TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
