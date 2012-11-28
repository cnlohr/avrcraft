//Copyright 2012 Charles Lohr under the MIT/x11, newBSD, LGPL or GPL licenses.  You choose.


#include "enc424j600.h"
#include "enc424j600_regs.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "avr_print.h"

uint8_t termcallbackearly;
static uint16_t sendbaseaddress;

static void standarddelay()
{
	_delay_us(15);
}

//big endian
uint16_t enc424j600_pop16()
{
	uint16_t ret;
	ret = espiR() << 8;
	return espiR() | ret;
}

void enc424j600_push16( uint16_t v)
{
	espiW( v >> 8 );
	espiW( v & 0xFF );
}

//little endian
uint16_t enc424j600_pop16LE()
{
	uint16_t ret;
	ret = espiR();
	return (espiR() << 8) | ret;
}

void enc424j600_push16LE( uint16_t v)
{
	espiW( v & 0xFF );
	espiW( v >> 8 );
}


void enc424j600_popblob( uint8_t * data, uint8_t len )
{
	while( len-- )
	{
		*(data++) = enc424j600_pop8();
	}
}

void enc424j600_pushblob( const uint8_t * data, uint8_t len )
{
	while( len-- )
	{
		enc424j600_push8( *(data++) );
	}
}

void enc424j600_dumpbytes( uint8_t len )
{
	while( len-- )
	{
		enc424j600_pop8();
	}
}


void enc424j600_write_ctrl_reg16( uint8_t addy, uint16_t value )
{
	ETCSPORT &= ~ETCS;
	espiW( EWCRU );
	espiW( addy );
	enc424j600_push16LE( value );
	ETCSPORT |= ETCS;	
}

//This requires the proper bank to be selected.
static uint8_t enc_read_ctrl_reg8_common( uint8_t addy )
{
	uint8_t ret;
	ETCSPORT &= ~ETCS;
	espiW( ERCR | addy );
	ret = espiR();
	ETCSPORT |= ETCS;
	return ret;
}

/*

static uint8_t enc_read_ctrl_reg8( uint8_t addy )
{
	uint8_t ret;
	ETCSPORT &= ~ETCS;
	espiW( ERCRU );
	espiW( addy );
	ret = espiR();
	ETCSPORT |= ETCS;
	return ret;
}*/

uint16_t enc424j600_read_ctrl_reg16( uint8_t addy )
{
	uint16_t ret;
	ETCSPORT &= ~ETCS;
	espiW( ERCRU );
	espiW( addy );
	ret = enc424j600_pop16LE();
	ETCSPORT |= ETCS;
	return ret;
}

static void enc_oneshot( uint8_t cmd )
{
	ETCSPORT &= ~ETCS;
	espiW( cmd );
	ETCSPORT |= ETCS;
}

uint16_t NextPacketPointer = RX_BUFFER_START;


int8_t enc424j600_init( const unsigned char * macaddy )
{
	unsigned char i = 0;

	ETPORT &= ~ETSCK;
	ETDDR &= ~( ETSO | ETINT );
	ETDDR |= ( ETSCK | ETSI	);
	ETCSDDR |= ETCS;
	ETCSPORT |= ETCS;

	standarddelay();

	enc424j600_write_ctrl_reg16( EEUDASTL, 0x1234 );
	if( enc424j600_read_ctrl_reg16( EEUDASTL ) != 0x1234 )
		return -1;

/*
	//Section 8.1 of datasheet.
	do
	{
		enc424j600_write_ctrl_reg16( EEUDASTL, 0x1234 );
		i++;
		if( i > 250 )
		{
			//Error. Cannot initialize.
			return -1;
		}
	} while( enc424j600_read_ctrl_reg16( EEUDASTL ) != 0x1234 );
*/
	//Instead of ECONing, we do it this way.
	enc_oneshot( ESSETETHRST );

	standarddelay();
	standarddelay();

	//the datasheet says to do this, but if I do, sometimes from a cold boot, it doesn't work.
//	if( enc424j600_read_ctrl_reg16( EEUDASTL ) )
//	{
//		return -2;
//	}

	//EECON2, add on "txmac" so we can save that time internally.
	enc424j600_write_ctrl_reg16( EECON2L, 0xEB00 );
	enc424j600_write_ctrl_reg16( EERXSTL, RX_BUFFER_START );
	enc424j600_write_ctrl_reg16( EMAMXFLL, MAX_FRAMELEN );
	//Must have RX tail here otherwise we will have difficulty moving our buffer along.
	enc424j600_write_ctrl_reg16( EERXTAILL, 0x5FFE );

	//DMA Copy-from logic uses read-area wrapping logic.
	enc424j600_write_ctrl_reg16( EEUDASTL, RX_BUFFER_START ); 

	*((uint16_t*)(&MyMAC[0])) = enc424j600_read_ctrl_reg16( EMAADR1L );
	*((uint16_t*)(&MyMAC[2])) = enc424j600_read_ctrl_reg16( EMAADR2L );
	*((uint16_t*)(&MyMAC[4])) = enc424j600_read_ctrl_reg16( EMAADR3L );

	//Enable RX
	enc_oneshot( ESENABLERX );

	return 0;
}

void enc424j600_startsend( uint16_t baseaddress)
{
	//Start at beginning of scratch.
	sendbaseaddress = baseaddress;
	enc424j600_write_ctrl_reg16( EEGPWRPTL, baseaddress );
	ETCSPORT &= ~ETCS;
	espiW( EWGPDATA );
	//Send away!
}

void enc424j600_endsend( )
{
	uint16_t i;
	uint16_t es;
	ETCSPORT |= ETCS;
	es = enc424j600_read_ctrl_reg16( EEGPWRPTL ) - sendbaseaddress;

	if( enc424j600_xmitpacket( sendbaseaddress, es ) )
	{
		sendstr( "send fail.\n" );
	}
}

uint16_t enc424j600_get_write_length()
{
	return enc424j600_read_ctrl_reg16( EEGPWRPTL ) + 6 - sendbaseaddress;
}

int8_t enc424j600_xmitpacket( uint16_t start, uint16_t len )
{
	uint8_t i = 0;

	//Wait for previous packet to complete.
	//ECON1 is in the common banks, so we can get info on it more quickly.
	while( enc_read_ctrl_reg8_common( EECON1L ) & _BV(1) )
	{
		i++;
		if( i > 250 )
		{
			//Consider clearing the packet in transmission, here.
			//Done by clearing TXRTS
			return -1;
		}
		//_delay_us(1);
		standarddelay();
	}

	enc424j600_write_ctrl_reg16( EETXSTL, start );
	enc424j600_write_ctrl_reg16( EETXLENL, len );

	//Don't worry, by default this part inserts the padding and crc.

	enc_oneshot( ESSETTXRTS );

	return 0;
}

void enc424j600_finish_callback_now()
{
	unsigned short nextpos;
	ETCSPORT |= ETCS;
	enc_oneshot( ESSETPKTDEC );
	//XXX BUG: NextPacketPointer-2 may be less than in the RX area
	if( NextPacketPointer == RX_BUFFER_START )
		nextpos = 0x5FFE;
	else
		nextpos = NextPacketPointer - 2;
	enc424j600_write_ctrl_reg16( EERXTAILL, nextpos );
	termcallbackearly = 1;
}

void enc424j600_stopop()
{
	ETCSPORT |= ETCS;
}

unsigned short enc424j600_recvpack()
{
	uint16_t receivedbytecount;
	enc_oneshot( ESENABLERX );
	termcallbackearly = 0;

	//if ERXTAIL == ERXHEAD this is BAD! (it thinks it's full)
	//ERXTAIL should be 2 less than ERXHEAD
	//Note: you don't read the two bytes immediately following ERXTAIL, they are dummy.

/*
	sendchr( '\n' );
	sendhex4( enc424j600_read_ctrl_reg16( EERXHEADL ) );
	sendchr( ':' );
	sendhex4( enc424j600_read_ctrl_reg16( EERXTAILL ) );
	sendchr( '\n' );
	*/

	//NextPacketPointer contains the start address of ERXST initially
	if( !enc_read_ctrl_reg8_common( EESTATL ) )
	{
		return 0;
	}

//	sendchr( '.' );
	//Configure ERXDATA for reading.
	enc424j600_write_ctrl_reg16( EERXRDPTL, NextPacketPointer );

	//Start reading!!!

	ETCSPORT &= ~ETCS;
	espiW( ERRXDATA );

	NextPacketPointer = enc424j600_pop16LE();

	//Read the status vector
	receivedbytecount = enc424j600_pop16LE();

	if( enc424j600_pop16LE() & _BV( 7 ) )
	{
		enc424j600_pop16LE(); 
		//Good packet.

		//Dest mac.
		enc424j600_receivecallback( receivedbytecount );
	}

	if( !termcallbackearly )
	{
		enc424j600_finish_callback_now();
	}

	return 0;
}

void enc424j600_wait_for_dma()
{
	uint8_t i = 0;
	//wait for previous DMA operation to complete
	while( ( enc_read_ctrl_reg8_common( EECON1L ) & _BV(5) )  &&  (i++ < 250 ) ) standarddelay();
}


//Start a checksum
void enc424j600_start_checksum( uint16_t start, uint16_t len )
{
	uint8_t i = 0;
	enc424j600_wait_for_dma();
	enc424j600_write_ctrl_reg16( EEDMASTL, start + sendbaseaddress );
	enc424j600_write_ctrl_reg16( EEDMALENL, len );
	enc_oneshot( ESDMACKSUM );
}

//Get the results of a checksum
uint16_t enc424j600_get_checksum()
{
	uint16_t ret;
	uint8_t i = 0;
	enc424j600_wait_for_dma();
	ret = enc424j600_read_ctrl_reg16( EEDMACSL );
	return (ret >> 8 ) | ( ( ret & 0xff ) << 8 );
}

//Modify a word of memory (based off of offset from start sending)
void enc424j600_alter_word( uint16_t address, uint16_t val )
{
	enc424j600_write_ctrl_reg16( EEUDAWRPTL, address + sendbaseaddress );
	ETCSPORT &= ~ETCS;
	espiW( EWUDADATA );
	espiW( val >> 8 );
	espiW( val & 0xFF );
	ETCSPORT |= ETCS;
}


void enc424j600_copy_memory( uint16_t to, uint16_t from, uint16_t length )
{
	enc424j600_wait_for_dma();
	enc424j600_write_ctrl_reg16( EEDMASTL, from );
	enc424j600_write_ctrl_reg16( EEDMADSTL, to );
	enc424j600_write_ctrl_reg16( EEDMALENL, length );
	enc_oneshot( ESDMACOPY ); //XXX TODO: Should we be purposefully /not/ calculating checksum?  Does it even matter?
}


//NOTE: What about read/write PHY? I don't think we need it.





//Code graveyard
/*


//This was used to iterate over a configuration array, if we had more than
//six configuration items it would produce smaller code, but we only have four.

//This is used for the loop iteration for configuration.
//It's less expensive when you have more than six values, but
//we don't so, we just hardcode it.
const unsigned char enc_config_data[] PROGMEM = {

	//EECON2, add on "txmac" so we can save that time internally.
	EECON2L,   0x00, 0xEB,

	//XXX TODO Why does this need to be -1
	EERXSTL, (RX_BUFFER_START & 0xFF), RX_BUFFER_START >> 8,
	EMAMXFLL, MAX_FRAMELEN & 0xFF, MAX_FRAMELEN >> 8,


	//Must have RX tail here otherwise we will have difficulty moving our buffer along.
	EERXTAILL, 0xFE, 0x5F,

//	EERXHEADL, RX_BUFFER_START & 0xFF, RX_BUFFER_START >> 8,


	0xFF, 0xFF, 0xff
};



//Here is one option, a loop to iterate through values.  Unless you exceed six variables, it's cheaper to just
//enc424j600_write_ctrl_reg16(...) your stuff.
	i = 0;
	do
	{
		//I haven't the foggiest idea why this needs to be volatile.
		volatile uint16_t val;
		uint8_t targ;
		targ = pgm_read_byte( enc_config_data + (i++) );
		val = pgm_read_word( enc_config_data + i ); i+=2;
		if( targ == 0xFF ) break;
		enc424j600_write_ctrl_reg16( targ, val );
	} while(1);
*/
