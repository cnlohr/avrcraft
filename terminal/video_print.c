//Copyright 2013 <>< Charles Lohr, under the MIT/x11 License.

#include <stdio.h>
#include "video_print.h"
#include <video.h>

uint8_t pfcol;
uint8_t pfrow;
uint8_t pfattrib;

uint8_t pfesc = 0;

uint8_t savecol, saverow, saveattrib;

uint8_t pfmark;
uint8_t num1;
uint8_t num2;

void ProcessEsc2( char c )
{
	int8_t i;
	uint16_t j;

	uint8_t isnumeric = (c>='0'&&c<='9');
	uint8_t num = c - '0';

	if( isnumeric )
	{
		if( pfmark == 0 )
		{
			num1 = num1*10 + num;
		}
		else if( pfmark == 1 )
		{
			num2 = num2*10 + num;
		}
		else
			pfesc = 0;
	}
	else
	{
		int killpf = 1;
		switch( c )
		{
		case ';': pfmark++; break;
		case 'A': //Up
			i = num1 - pfrow;
			if( i > 0 ) pfrow = i;
			else        pfrow = 0;
			break;
		case 'B': 
			i = num1 + pfrow;
			if( i < NTLINES ) pfrow = i;
			else              pfrow = NTLINES-1;
			break;
		case 'D': //Up
			i = num1 - pfcol;
			if( i > 0 ) pfcol = i;
			else        pfcol = 0;
			break;
		case 'C': 
			i = num1 + pfrow;
			if( i < NTWIDTH ) pfcol = i;
			else              pfcol = NTWIDTH-1;
			break;
		case 'H': case 'f':
			if( num1 < NTLINES ) pfrow = num1; else pfrow = NTLINES-1;
			if( num2 < NTWIDTH ) pfcol = num2; else pfcol = NTWIDTH-1;
			break;
		case 'm':
			pfattrib = num1;
			break;
		case 'K':
			switch( num1 )
			{
			case 1:
				for( i = 0; i <= pfcol; i++ )
					framebuffer[NTWIDTH*pfrow + i] = 0;
				break;
			case 2:
				for( i = 0; i < NTWIDTH; i++ )
					framebuffer[NTWIDTH*pfrow + i] = 0;
				break;
			default:
			case 0:
				for( i = pfcol; i < NTWIDTH; i++ )
					framebuffer[NTWIDTH*pfrow + i] = 0;
				break;
			}
		case 'J':
			switch( num1 )
			{
			case 1:
				for( j = NTWIDTH*pfrow + pfcol; j < NTWIDTH*NTLINES; j++ )
					framebuffer[j] = 0;
				break;
			case 2:
				for( j = 0; j < NTWIDTH*NTLINES; j++ )
					framebuffer[j] = 0;
				break;
			default:
			case 0:
				for( j = 0; j <= NTWIDTH*pfrow + pfcol; j++ )
					framebuffer[j] = 0;
				break;
			}
		default:
			break;
		}

		if( killpf ) 
			pfesc = 0;
	}
	
}

void sendchr( char c )
{
	//[
	switch( pfesc )
	{
	case 1:
		pfesc = 0;
		switch( c )
		{
			case '(': pfesc = 3; break;
			case '[': pfesc = 2; pfmark = 0; num1 = 0; num2 = 0; break;
			case 'D': if( pfrow < NTLINES-1 ) pfrow++; break;
			case 'M': if( pfrow > 0 ) pfrow--; break;
			case 'E': pfcol = NTWIDTH; break;
			case '7': savecol = pfcol; saverow = pfrow; saveattrib = pfattrib; break;
			case '8': pfcol = savecol; pfrow = saverow; pfattrib = saveattrib; break;
		}
		return;
	case 2:
		ProcessEsc2( c );
		return;
	case 3:
		pfesc = 0;
		return;
	default:
	case 0: break;
	}

	if( c == '\n' ) 
	{
		pfcol = NTWIDTH;
	}
	else if( c == 27 )
	{
		pfesc = 1;
		return;
	}
	else 
	{
		framebuffer[NTWIDTH*pfrow + pfcol] = c | ((pfattrib==7)?0x80:0);
		pfcol++;
	}

	if( pfcol >= NTWIDTH )
	{
		pfrow++;
		pfcol = 0;

		//Need to scroll?
		if( pfrow >= NTLINES )
		{
			uint16_t by;

			for( by = 0; by < NTWIDTH * (NTLINES-1); by++ )
				framebuffer[by] = framebuffer[by+NTWIDTH];

			pfrow--;

			for( ; by < NTWIDTH * NTLINES; by++ )
				framebuffer[by] = 0;
			
		}
	}
	if( pfrow >= NTLINES ) pfrow = 0;
}

void sendhex1( unsigned char i )
{
	sendchr( (i<10)?(i+'0'):(i+'A'-10) );
}
void sendhex2( unsigned char i )
{
	sendhex1( i>>4 );
	sendhex1( i&0x0f );
}
void sendhex4( unsigned int i )
{
	sendhex2( i>>8 );
	sendhex2( i&0xFF);
}


static int SPIPutCharInternal(char c, FILE *stream)
{
	sendchr( c );
	return 0;
}

static FILE mystdout = FDEV_SETUP_STREAM( SPIPutCharInternal, NULL, _FDEV_SETUP_WRITE );

void videotermsetup( void )
{
	uint16_t tf;

	pfrow = 0;
	pfcol = 0;
	
	for( tf = 0; tf < NTWIDTH*NTLINES; tf++ )
	{
		framebuffer[tf] = 0;
	}

	sendstr( "Booting.\n" );
}

