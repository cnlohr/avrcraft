//Copyright 2012 <>< Charles Lohr, All Rights Reserved, May be licensed under MIT/x11 or newBSD licenses, you choose.

#include "microsd.h"
#include <avr/io.h>
#include "avr_print.h"
#include <avr/delay.h>

#define FAST_SPI

static unsigned char WaitFor( unsigned char c );  //Return 0 if OK, 1 if bad.
//static unsigned char spiRW(unsigned char ins);
static void spiW(unsigned char ins);
static unsigned char spiR();
static void spiD();
static void reCS();

short    		opsleftSD;
static unsigned char	highcap;

unsigned char initSD()
{
///	unsigned char i = 0;
	unsigned char tries = 0;
restart:
	SDPORT &= ~SDCLK;
	SDPORT |= SDMOSI;
	SDDDR |= SDCLK | SDMOSI;
	SDDDR &= ~SDMISO;
	SDCSDE;
	dumpSD(10);
	_delay_ms(1);
	SDCSEN;
	_delay_us(50);

	spiW( 0x40 ); spiW( 0x00 ); spiW( 0x00 );
	spiW( 0x00 ); spiW( 0x00 ); spiW( 0x95 );

	if( WaitFor( 1 ) )
	{
		//Up to three tries to get sync'd.
		SDCSDE;
		if( tries++ < 10 ) 
			goto restart;
		sendstr( "Failure finding SD card.\n" );
		SDCSDE;
		return 1;
	}


	//Card is present!

	reCS();
	//XXX Yucky work-around, we have to tell the host we support high-capacity cards.

	spiW(0x48);    // CMD8
	spiW(0x00);    //
	spiW(0x00);
	spiW(0x01);    // HVS
	spiW(0xAA);    // test pattern
	spiW(0x87);    // right checksum for CMD8

	dumpSD( 6 );

sendcmd1:

	reCS();

	spiW(0x77);    // CMD55
	spiW(0x00);   
	spiW(0x00);
	spiW(0x00);
	spiW(0x00);

	dumpSD( 3 );

	reCS();

	//the 0x40 supposadly tells the card that not just SD, but SDHC is OK!
	spiW( 0x69 ); spiW( 0x40 ); spiW( 0x00 );  //CMD41?
	spiW( 0x00 ); spiW( 0x00 ); spiD();
	//sendstr( "CK" );

	if( WaitFor( 0 ) )
	{
		//Up to three tries to get sync'd.
		if( tries++ < 100 ) 
			goto sendcmd1;
		sendstr( "No init.\n" );
		SDCSDE;
		return 1;
	}
	
	//Card is basically  set up!
	reCS();

	spiW( 0x50 ); spiW( 0x00 ); spiW( 0x00 );  //Use 512-byte block reads.  //CMD16
	spiW( 0x02 ); spiW( 0x00 ); spiW( 0x01 );

	dumpSD(3);

	reCS();

	//Check capacity (we don't know if we are using low or high capacity addressing mode)

	spiW( 0x7a ); //CMD58 (read OCR)

	dumpSD( 7 );
	if( spiR() & 0x40 ) //TODO Should I make sure it's not 0xff?
	{
		highcap = 1;
	}
	else
	{
		highcap = 0;
	}

	dumpSD(3);

	SDCSDE;

	sendstr( "Card OK.\n" );

	return 0;
}






void testReadBlock()
{
	unsigned short i;
	unsigned short j;


	for( j = 1024+10000; j < 1280+10000; j++ )
	{
		unsigned char sdr, exp;

		startSDwrite( j );
		for( i = 0; i < 52; i++ )
			pushSDwrite( j + 0xaa );
		endSDwrite();


		startSDread( j );
		sendchr( 0 );

		sdr = popSDread();
		exp = j + 0xaa;
	
		if( sdr != exp )
		{
			sendhex2( sdr );
			sendchr( '.' );
			sendhex2( exp );
		}
		sendchr( ' ' );
		endSDread();
	}
}








unsigned char startSDwrite( uint32_t address )
{
	SDCSDE;
	spiD();
	SDCSEN;
	spiD();

	if( !highcap ) address>>=9;

	spiW( 0x40 + 24 );
	spiW( address >> 24 );
	spiW( address >> 16 );
	spiW( address >> 8 );
	spiW( address );

	if( WaitFor( 0 ) ) return 1;

	spiW( 0xFE ); //start sending block.

	opsleftSD = 512;

	return 0;
}

void pushSDwrite( unsigned char c )
{
	spiW( c );
	--opsleftSD;
}

//nonzero indicates failure.
unsigned char endSDwrite()
{
	unsigned char cc;
	unsigned char timeout = 200;

	dumpSD( opsleftSD );

	spiD(); //CRCA

	//XXX Warning, the 50us wait in this function could be too low.  if it's at 10,
	//you will have write issues.
	do
	{
		_delay_us(50);
		cc = spiR();
	}
	while ( --timeout && cc == 0xff );

	timeout = 200;

	if( (cc & 0x0f) != 0x05 )
	{
		dumpSD(3);
		SDCSDE;
		sendstr( "write failed.\n" );
		return 2;
	}

	//Wait for read to complete.
	while( spiR() != 0xFF && timeout-- ) _delay_us(50);

	SDCSDE;
	return 0;
}

//returns nonzero if failure.
unsigned char startSDread( uint32_t address )
{
	unsigned char retries = 250;
	unsigned char status;
	reCS();

	if( !highcap ) address<<=9;

	spiW( 0x51 ); //0x57 ); //was 51
	spiW( address>>24 );
	spiW( address>>16 );
	spiW( address>>8 );
	spiW( address );
	spiD();

	spiD();
	spiD();
	status = spiR();
/*
	sendchr( 0 ); sendchr( '$' ); sendhex2( status ); sendchr( '$' );
	sendhex2( address>>24 );
	sendhex2( address>>16 );
	sendhex2( address>>8 );
	sendhex2( address );
*/
	opsleftSD = 512;

	while( retries-- )
	{
		if( !WaitFor( 0xFE ) )
			return 0;
		_delay_ms(1);
	}

	endSDread();
	return 1;
}

unsigned char popSDread()
{
	--opsleftSD;
	return spiR();
}

void endSDread()
{
	dumpSD( opsleftSD + 3 );

	SDCSDE;
	spiD();
}









//Return 0 if OK, 1 if bad.
static unsigned char WaitFor( unsigned char c )
{
	unsigned char i = 250;
	for( ; i; --i )
	{
		if( spiR( ) == c ) return 0;
	}
	return 1;
}

void dumpSD( short count )
{
	while( count-- > 0 )
	{
		spiD();
	}
}

static void reCS()
{
	SDCSDE;
	spiD();
	SDCSEN;
	spiD();
}



static unsigned char spiR()
{
	unsigned char ret = 0;
#ifndef FAST_SPI
	unsigned char i;

	SDPORT |= SDMOSI;

	for( i = 0; i < 8; i++ )
	{
		SDPORT &= ~SDCLK;
		ret <<= 1;
		if( SDPIN & SDMISO )
			ret |= 0x01;
		SDPORT |= SDCLK;
	}
	SDPORT &= ~SDCLK;

	return ret;
#else
	SDPORT |= SDMOSI;

	SDPORT &= ~SDCLK;
	SDPORT |= SDCLK;
	if( SDPIN & SDMISO ) ret |= 0x80;
	SDPORT &= ~SDCLK;
	SDPORT |= SDCLK;
	if( SDPIN & SDMISO ) ret |= 0x40;
	SDPORT &= ~SDCLK;
	SDPORT |= SDCLK;
	if( SDPIN & SDMISO ) ret |= 0x20;
	SDPORT &= ~SDCLK;
	SDPORT |= SDCLK;
	if( SDPIN & SDMISO ) ret |= 0x10;
	SDPORT &= ~SDCLK;
	SDPORT |= SDCLK;
	if( SDPIN & SDMISO ) ret |= 0x08;
	SDPORT &= ~SDCLK;
	SDPORT |= SDCLK;
	if( SDPIN & SDMISO ) ret |= 0x04;
	SDPORT &= ~SDCLK;
	SDPORT |= SDCLK;
	if( SDPIN & SDMISO ) ret |= 0x02;
	SDPORT &= ~SDCLK;
	SDPORT |= SDCLK;
	if( SDPIN & SDMISO ) ret |= 0x01;
	SDPORT |= SDCLK;
	SDPORT &= ~SDCLK;
	return ret;
#endif
}


static void spiW(unsigned char ins)
{
#if defined( FAST_SPI )
	SDPORT &= ~SDCLK;
	if( ins & 0x80 ) SDPORT |= SDMOSI; else SDPORT &= ~SDMOSI;
	SDPORT |= SDCLK;
	SDPORT &= ~SDCLK;
	if( ins & 0x40 ) SDPORT |= SDMOSI; else SDPORT &= ~SDMOSI;
	SDPORT |= SDCLK;
	SDPORT &= ~SDCLK;
	if( ins & 0x20 ) SDPORT |= SDMOSI; else SDPORT &= ~SDMOSI;
	SDPORT |= SDCLK;
	SDPORT &= ~SDCLK;
	if( ins & 0x10 ) SDPORT |= SDMOSI; else SDPORT &= ~SDMOSI;
	SDPORT |= SDCLK;
	SDPORT &= ~SDCLK;
	if( ins & 0x08 ) SDPORT |= SDMOSI; else SDPORT &= ~SDMOSI;
	SDPORT |= SDCLK;
	SDPORT &= ~SDCLK;
	if( ins & 0x04 ) SDPORT |= SDMOSI; else SDPORT &= ~SDMOSI;
	SDPORT |= SDCLK;
	SDPORT &= ~SDCLK;
	if( ins & 0x02 ) SDPORT |= SDMOSI; else SDPORT &= ~SDMOSI;
	SDPORT |= SDCLK;
	SDPORT &= ~SDCLK;
	if( ins & 0x01 ) SDPORT |= SDMOSI; else SDPORT &= ~SDMOSI;
	SDPORT |= SDCLK;
	SDPORT &= ~SDCLK;
#else
	unsigned char i;
	SDPORT &= ~SDCLK;
	for( i = 0; i < 8; i++ )
	{
		SDPORT &= ~SDCLK;
		if( ins & 0x80 )
			SDPORT |= SDMOSI;
		else
			SDPORT &= ~SDMOSI;
		ins <<= 1;
		SDPORT |= SDCLK;
	}
	SDPORT &= ~SDCLK;
#endif
}

static void spiD()
{
	unsigned char i;
	SDPORT |= SDMOSI;
	for( i = 0; i < 8; )
	{
		SDPORT &= ~SDCLK;
		i++;
		SDPORT |= SDCLK;
	}
	SDPORT &= ~SDCLK;
}




/*

//SPI operates on rising edge (try to improve performance of this function)
static unsigned char spiRW(unsigned char ins)
{

	unsigned char ret = 0;

	unsigned char i;
	SDPORT &= ~SDCLK;
	for( i = 0; i < 8; i++ )
	{
		ret <<= 1;
		if( ins & 0x80 )
			SDPORT |= SDMOSI;
		else
			SDPORT &= ~SDMOSI;

		SDPORT |= SDCLK;
		if( SDPIN & SDMISO )
			ret |= 1;

		ins <<= 1;
		SDPORT &= ~SDCLK;
	}

	return ret;
}

*/
