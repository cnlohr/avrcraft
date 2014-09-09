#include "util10.h"
#include <string.h>
#include <stdlib.h>

void Uint32To10Str( char * sto, uint32_t indata )
{
	uint32_t place = 1000000000;
	uint8_t pl = 0;
	uint8_t ct = 0;

	while( place )
	{
		while( indata >= place )
		{
			ct++;
			indata -= place;
		}
		if( pl || ct )
		{
			sto[pl++] = ct + '0';
		}
		place /= 10;
		ct = 0;
	}
	if( pl == 0 ) sto[pl++] = '0';
	sto[pl] = 0;
}

//TODO: Consider de-loopifying this some.
void Uint8To10Str( char * str, uint8_t val )
{
	uint8_t count = 0;
	while( val >= 100 ) { val -= 100; count++; }
	str[0] = '0' + count;
	count = 0;
	while( val >= 10 ) { val -= 10; count++; }
	str[1] = '0' + count;
	count = 0;
	while( val >= 1 ) { val -= 1; count++; }
	str[2] = '0' + count;
	str[3] = 0;
}

void Uint8To16Str( char * str, uint8_t val )
{
	uint8_t high = val>>4;
	uint8_t low  = val & 0x0f;
	str[0] = (high>9)?('a'-10+high):(high+'0');
	str[1] = (low>9)?('a'-10+low):(low+'0');
	str[2] = 0;
}


uint16_t StrTo16Uint( char * str )
{
	uint16_t ret = 0;
	uint8_t yet = 0;
	char c;
	while( (c = (*(str++))) )
	{
		if( c >= '0' && c <= '9' )
		{
			yet = 1;
			ret = ret * 10 + (c - '0');
		}
		else if( yet )
		{
			//Chars in the middle of the number.
			return ret;
		}
	}
	return ret;
}

