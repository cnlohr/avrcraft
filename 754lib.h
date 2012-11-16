//Copyright 2012 <>< Charles Lohr, under the MIT-x11 or NewBSD license.  You choose.

#ifndef _754LIB_H
#define _754LIB_H

#include <stdint.h>


#ifdef __AVR__

#else

//For IEEE754-1985 doubles and floats; static library.
// for i386, this must be set.
//#define ENDIAN_SWITCH

#endif

#ifdef ENDIAN_SWITCH
#define ENDI8(x) (7-x)
#define ENDI4(x) (3-x)
#else
#define ENDI8(x) x
#define ENDI4(x) x
#endif


static int16_t DoubleToW( double * d, int8_t shift )
{
	uint8_t * u = (uint8_t*)d;
	uint16_t ret;
	int16_t exponent;

	exponent = (((u[ENDI8(0)]<<8) | u[ENDI8(1)])&0x7ff0) >> 4;
	ret = (u[ENDI8(1)] << 11) | (u[ENDI8(2)] << 3) | (u[ENDI8(3)] >> 5) | 0x8000;

	exponent += shift;
	exponent = 0x40e - exponent;

	if( exponent < 0 )
		ret = 0x7fff;
	else if( exponent > 15 )
		ret = 0;
	else
		ret >>= exponent;

	if( u[ENDI8(0)] & 0x80 )
		return -ret;
	return ret;
}


//As of yet is unwritten.
static void WToDouble( int16_t in, int8_t shift, double * d )
{
	uint8_t i;
	uint16_t exp = 0x80e - shift;
	uint16_t flipin;

	uint8_t * u = (uint8_t*)d;

	if( in < 0 )
	{
		u[ENDI8(0)] = 0x80;
		in = -in;
	}
	else
	{
		u[ENDI8(0)] = 0x00;
	}

	flipin = in;
	for( i = 0; i < 16; i++ )
	{
		if( flipin & 0x8000 ) break;
		exp--;
		flipin<<=1;
	}
	if( i == 16 ) exp = 0;
	flipin<<=1;

	//in is now normalized.
	u[ENDI8(0)] |= (exp>>5);
	u[ENDI8(1)] = (exp<<4) | (flipin>>12);
	u[ENDI8(2)] = (flipin>>4);
	u[ENDI8(3)] = (flipin<<4);
	u[ENDI8(4)] = 0;
	u[ENDI8(5)] = 0;
	u[ENDI8(6)] = 0;
	u[ENDI8(7)] = 0;

}


//Float stuffs

static uint16_t FloatToW( float * d, int8_t shift )
{
	uint16_t ret;
	int16_t exponent;

	uint8_t * u = (uint8_t*)d;

	exponent = ((u[ENDI4(0)]&0x7f)<<1) | (u[ENDI4(1)]>>7);

	ret = (u[ENDI4(1)] << 8) | (u[ENDI4(2)]) | 0x8000;

	exponent += shift;
	exponent = 0x8e - exponent;
	if( exponent < 0 )
		ret = 0x7fff;
	else if( exponent > 15 )
		ret = 0;
	else
		ret >>= exponent;

	if( u[ENDI4(0)] & 0x80 )
		return -ret;
	return ret;
}



//As of yet is unwritten.
static void WToFloat( int16_t in, int8_t shift, float * d )
{
	uint8_t i;
	uint16_t exp = 0x8e - shift;
	uint16_t flipin;

	uint8_t * u = (uint8_t*)d;


	if( in < 0 )
	{
		u[ENDI4(0)] = 0x80;
		in = -in;
	}
	else
	{
		u[ENDI4(0)] = 0x00;
	}

	flipin = in;
	for( i = 0; i < 16; i++ )
	{
		if( flipin & 0x8000 ) break;
		exp--;
		flipin<<=1;
	}
	if( i == 16 ) exp = 0;
	flipin<<=1;

	//in is now normalized.
	u[ENDI4(0)] |= (exp>>1);
	u[ENDI4(1)] = (exp<<7) | (flipin>>9);
	u[ENDI4(2)] = (flipin>>1);
	u[ENDI4(3)] = (flipin<<7);
}

#endif



