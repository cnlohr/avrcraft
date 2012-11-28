//Copyright 2012 <>< Charles Lohr, All Rights Reserved, May be licensed under MIT/x11 or newBSD licenses, you choose.

#include "microsd.h"
#include <avr/io.h>
#include "avr_print.h"
#include <avr/delay.h>

#define FAST_SPI
#define MICROSD_ASM_SPI

//enabling this means you WILL NOT manipulate the port MOSI, MISO or SCK of the device is on while an
//operation could be pending.  It is extremely dangerous unless you are sure interrupts won't mess you up!
//#define MICROSD_ASM_FAST_AND_DANGEROUS
//TODO: Fix void microSPIW(unsigned char ins)'s ASM and write SPID's too.
//XXX IT's all broken.

static unsigned char WaitFor( unsigned char c );  //Return 0 if OK, 1 if bad.
//static unsigned char microSPIRW(unsigned char ins);
void microSPIW(unsigned char ins);
unsigned char microSPIR();
void microSPID();
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

	microSPIW( 0x40 ); microSPIW( 0x00 ); microSPIW( 0x00 );
	microSPIW( 0x00 ); microSPIW( 0x00 ); microSPIW( 0x95 );

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

	microSPIW(0x48);    // CMD8
	microSPIW(0x00);    //
	microSPIW(0x00);
	microSPIW(0x01);    // HVS
	microSPIW(0xAA);    // test pattern
	microSPIW(0x87);    // right checksum for CMD8

	dumpSD( 6 );

sendcmd1:

	reCS();

	microSPIW(0x77);    // CMD55
	microSPIW(0x00);   
	microSPIW(0x00);
	microSPIW(0x00);
	microSPIW(0x00);

	dumpSD( 3 );

	reCS();

	//the 0x40 supposadly tells the card that not just SD, but SDHC is OK!
	microSPIW( 0x69 ); microSPIW( 0x40 ); microSPIW( 0x00 );  //CMD41?
	microSPIW( 0x00 ); microSPIW( 0x00 ); microSPID();
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

	microSPIW( 0x50 ); microSPIW( 0x00 ); microSPIW( 0x00 );  //Use 512-byte block reads.  //CMD16
	microSPIW( 0x02 ); microSPIW( 0x00 ); microSPIW( 0x01 );

	dumpSD(3);

	reCS();

	//Check capacity (we don't know if we are using low or high capacity addressing mode)

	microSPIW( 0x7a ); //CMD58 (read OCR)

	dumpSD( 7 );
	if( microSPIR() & 0x40 ) //TODO Should I make sure it's not 0xff?
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
	microSPID();
	SDCSEN;
	microSPID();

	if( !highcap ) address>>=9;

	microSPIW( 0x40 + 24 );
	microSPIW( address >> 24 );
	microSPIW( address >> 16 );
	microSPIW( address >> 8 );
	microSPIW( address );

	if( WaitFor( 0 ) ) return 1;

	microSPIW( 0xFE ); //start sending block.

	opsleftSD = 512;

	return 0;
}

void pushSDwrite( unsigned char c )
{
	microSPIW( c );
	--opsleftSD;
}

//nonzero indicates failure.
unsigned char endSDwrite()
{
	unsigned char cc;
	unsigned char timeout = 200;

	dumpSD( opsleftSD );

	microSPID(); //CRCA

	//XXX Warning, the 50us wait in this function could be too low.  if it's at 10,
	//you will have write issues.
	do
	{
		_delay_us(50);
		cc = microSPIR();
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
	while( microSPIR() != 0xFF && timeout-- ) _delay_us(50);

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

	microSPIW( 0x51 ); //0x57 ); //was 51
	microSPIW( address>>24 );
	microSPIW( address>>16 );
	microSPIW( address>>8 );
	microSPIW( address );
	microSPID();

	microSPID();
	microSPID();
	status = microSPIR();
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
	return microSPIR();
}

void endSDread()
{
	dumpSD( opsleftSD + 3 );

	SDCSDE;
	microSPID();
}









//Return 0 if OK, 1 if bad.
static unsigned char WaitFor( unsigned char c )
{
	unsigned char i = 250;
	for( ; i; --i )
	{
		if( microSPIR( ) == c ) return 0;
	}
	return 1;
}

void dumpSD( short count )
{
	while( count-- > 0 )
	{
		microSPID();
	}
}

static void reCS()
{
	SDCSDE;
	microSPID();
	SDCSEN;
	microSPID();
}


#ifdef MICROSD_ASM_SPI

unsigned char __attribute__((naked)) microSPIR()
{
#ifdef MICROSD_ASM_FAST_AND_DANGEROUS
asm volatile( "\
\n\tin r18, %0\
\n\tcbr r18, %2\
\n\tmov r19, r18\
\n\tsbr r19, %2\
\n\tclr r24\
\n\tout %0, r18\
\n\tout %0, r19\
\n\tsbic %1, %3\
\n\tori r24, 0x80\
\n\tout %0, r18\
\n\tout %0, r19\
\n\tsbic %1, %3\
\n\tori r24, 0x40\
\n\tout %0, r18\
\n\tout %0, r19\
\n\tsbic %1, %3\
\n\tori r24, 0x20\
\n\tout %0, r18\
\n\tout %0, r19\
\n\tsbic %1, %3\
\n\tori r24, 0x10\
\n\tout %0, r18\
\n\tout %0, r19\
\n\tsbic %1, %3\
\n\tori r24, 0x08\
\n\tout %0, r18\
\n\tout %0, r19\
\n\tsbic %1, %3\
\n\tori r24, 0x04\
\n\tout %0, r18\
\n\tout %0, r19\
\n\tsbic %1, %3\
\n\tori r24, 0x02\
\n\tout %0, r18\
\n\tout %0, r19\
\n\tsbic %1, %3\
\n\tori r24, 0x01\
\n\tout %0, r18\
\n\t ret \n\t" : : "I" (_SFR_IO_ADDR(SDPORT)), "I" (_SFR_IO_ADDR(SDPIN)), "M" (SDCLK), "I" (SDMISOP) );
#else
asm volatile( "\
\n\t clr r24 \
\n\t cbi %0, %2 \
\n\t sbi %0, %2 \
\n\t sbic %1, %3 \
\n\t ori r24, 0x80 \
\n\t cbi %0, %2 \
\n\t sbi %0, %2 \
\n\t sbic %1, %3 \
\n\t ori r24, 0x40 \
\n\t cbi %0, %2 \
\n\t sbi %0, %2 \
\n\t sbic %1, %3 \
\n\t ori r24, 0x20 \
\n\t cbi %0, %2 \
\n\t sbi %0, %2 \
\n\t sbic %1, %3 \
\n\t ori r24, 0x10 \
\n\t cbi %0, %2 \
\n\t sbi %0, %2 \
\n\t sbic %1, %3 \
\n\t ori r24, 0x08 \
\n\t cbi %0, %2 \
\n\t sbi %0, %2 \
\n\t sbic %1, %3 \
\n\t ori r24, 0x04 \
\n\t cbi %0, %2 \
\n\t sbi %0, %2 \
\n\t sbic %1, %3 \
\n\t ori r24, 0x02 \
\n\t cbi %0, %2 \
\n\t sbi %0, %2 \
\n\t sbic %1, %3 \
\n\t ori r24, 0x01 \
\n\t cbi %0, %2 \
\n\t ret \n\t" : : "I" (_SFR_IO_ADDR(SDPORT)), "I" (_SFR_IO_ADDR(SDPIN)), "I" (SDCLKP), "I" (SDMISOP) );
//r24 is automatically return; This is mostly a naked function.
#endif
}

void microSPIW(unsigned char ins)
{
#ifdef MICROSD_ASM_FAST_AND_DANGEROUSx
asm volatile( "\
\
\n\tin r18, %1\
\n\tcbr r18, %3\
\n\tcbr r18, %4\
\n\tmov r20, r18\
\n\tsbr r20, %4\
\
\n\tout %1, r18\
\
\n\tsbrc %0, 7\
\n\tout %1, r20\
\n\tsbi %1, %2\
\n\tout %1, r18\
\
\n\tsbrc %0, 6\
\n\tout %1, r20\
\n\tsbi %1, %2\
\n\tout %1, r18\
\
\n\tsbrc %0, 5\
\n\tout %1, r20\
\n\tsbi %1, %2\
\n\tout %1, r18\
\
\n\tsbrc %0, 4\
\n\tout %1, r20\
\n\tsbi %1, %2\
\n\tout %1, r18\
\
\n\tsbrc %0, 3\
\n\tout %1, r20\
\n\tsbi %1, %2\
\n\tout %1, r18\
\
\n\tsbrc %0, 2\
\n\tout %1, r20\
\n\tsbi %1, %2\
\n\tout %1, r18\
\
\n\tsbrc %0, 1\
\n\tout %1, r20\
\n\tsbi %1, %2\
\n\tout %1, r18\
\
\n\tsbrc %0, 0\
\n\tout %1, r20\
\n\tsbi %1, %2\
\n\tout %1, r18\
\
\n\tret\n\t" : : "d" (ins), "I" (_SFR_IO_ADDR(SDPORT)), "I" (SDCLKP), "M" (SDCLK), "M" (SDMOSI) );
#else
asm volatile( "\
\n\t cbi %1, %2\
\n\t cbi %1, %3\
\n\t sbrc %0, 7\
\n\t sbi %1, %3\
\n\t sbi %1, %2\
\n\t cbi %1, %2\
\n\t\
\n\t cbi %1, %3\
\n\t sbrc %0, 6\
\n\t sbi %1, %3\
\n\t sbi %1, %2\
\n\t cbi %1, %2\
\n\t\
\n\t cbi %1, %3\
\n\t sbrc %0, 5\
\n\t sbi %1, %3\
\n\t sbi %1, %2\
\n\t cbi %1, %2\
\n\t\
\n\t cbi %1, %3\
\n\t sbrc %0, 4\
\n\t sbi %1, %3\
\n\t sbi %1, %2\
\n\t cbi %1, %2\
\n\t\
\n\t cbi %1, %3\
\n\t sbrc %0, 3\
\n\t sbi %1, %3\
\n\t sbi %1, %2\
\n\t cbi %1, %2\
\n\t\
\n\t cbi %1, %3\
\n\t sbrc %0, 2\
\n\t sbi %1, %3\
\n\t sbi %1, %2\
\n\t cbi %1, %2\
\n\t\
\n\t cbi %1, %3\
\n\t sbrc %0, 1\
\n\t sbi %1, %3\
\n\t sbi %1, %2\
\n\t cbi %1, %2\
\n\t\
\n\t cbi %1, %3\
\n\t sbrc %0, 0\
\n\t sbi %1, %3\
\n\t sbi %1, %2\
\n\t cbi %1, %2\
\n\t" : : "r" (ins), "I" (_SFR_IO_ADDR(SDPORT)), "I" (SDCLKP), "I" (SDMOSIP) );
#endif
}

void __attribute__((naked)) microSPID()
{
asm volatile( "\
\n\t cbi %0, %1\
\n\t sbi %0, %2\
\n\t sbi %0, %1\
\n\t cbi %0, %1\
\n\t\
\n\t sbi %0, %1\
\n\t cbi %0, %1\
\n\t\
\n\t sbi %0, %1\
\n\t cbi %0, %1\
\n\t\
\n\t sbi %0, %1\
\n\t cbi %0, %1\
\n\t\
\n\t sbi %0, %1\
\n\t cbi %0, %1\
\n\t\
\n\t sbi %0, %1\
\n\t cbi %0, %1\
\n\t\
\n\t sbi %0, %1\
\n\t cbi %0, %1\
\n\t\
\n\t sbi %0, %1\
\n\t cbi %0, %1\
\n\t ret" : : "I" (_SFR_IO_ADDR(SDPORT)), "I" (SDCLKP), "I" (SDMOSIP) );
}

#else


unsigned char microSPIR()
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

void microSPIW(unsigned char ins)
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

void microSPID()
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



#endif




/*

//SPI operates on rising edge (try to improve performance of this function)
static unsigned char microSPIRW(unsigned char ins)
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
