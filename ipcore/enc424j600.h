//Copyright 2012 Charles Lohr under the MIT/x11, newBSD, LGPL or GPL licenses.  You choose.

#ifndef _enc424j600_H
#define _enc424j600_H

#include "eth_config.h"
#include "enc424j600_regs.h"
#include <avr/io.h>

//SPI Comms details

#ifdef ASM_SPI
#define espiR espiR_asm
#define espiW espiW_asm
#endif

#ifndef ASSEMBLY

uint8_t espiR();
void espiW( uint8_t i );

//Memory configuration
//The ENC424j600 has 0x5FFF (24kB) bytes of memory
//We have to make good use of it.

//RX Buffer goes from here to the end of ram): This is already device default
#define RX_BUFFER_SIZE   (3264)
#define RX_BUFFER_START  (0x6000 - RX_BUFFER_SIZE)



#if RX_BUFFER_SIZE < MAX_FRAMELEN + 20
#error RX_BUFFER_SIZE not big enough.
#endif




//return 0 if OK, otherwise nonzero.
int8_t enc424j600_init( const unsigned char * macaddy );

//return 0 if OK, otherwise nonzero.
//Packets are stored anywhere in the enc424j600's memory.
//Packets expect to be in the following format:
// [dest mac x6] [protocol (80 00)] [data]
int8_t enc424j600_xmitpacket( uint16_t start, uint16_t len );

//returns 0 if no packet. Otherwise nonzero.
unsigned short enc424j600_recvpack();

//You have to write this!
void enc28j60_receivecallback( uint16_t packetlen );


//Finish up any reading. 							//CLOSURE
void enc424j600_finish_callback_now();

//Raw, on-wire pops. (assuming already in read)
void enc424j600_popblob( uint8_t * data, uint8_t len );
void enc424j600_dumpbytes( uint8_t len );
uint16_t enc424j600_pop16();
#define enc424j600_pop8  espiR

//Raw, on-wire push. (assuming already in write)
void enc424j600_pushblob( const uint8_t * data, uint8_t len );
#define enc424j600_push8  espiW
void enc424j600_push16( uint16_t v);

//XXX: Todo: see if we can find a faster way of invoking espiR.


//Start a new send.									//OPENING
void enc424j600_startsend( uint16_t start );

uint16_t enc424j600_get_write_length();

//End sending (calls xmitpacket with correct flags)	//CLOSURE
void enc424j600_endsend();

//Deselects the chip, can halt operation.			//CLOSURE
void enc424j600_stopop();

//Start a checksum
void enc424j600_start_checksum( uint16_t start, uint16_t len );

//Get the results of a checksum (little endian)
uint16_t enc424j600_get_checksum();

//Modify a word of memory (little endian)
void enc424j600_alter_word( uint16_t address, uint16_t val );



//User must provide this:
void enc424j600_receivecallback( uint16_t receivedbytecount );
#endif
#endif

