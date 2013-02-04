 /* General demo program, Copyleft 2008 Charles Lohr */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>

#include "ntsc.h"
#include <stdio.h>
#include "video.h"

void SetupClock()
{
	CLKPR = _BV( CLKPCE );
	CLKPR = 0; //No divide
}

volatile extern char framebuffer[];

int main() {
	short tf = 0;

	cli();

	SetupNTSC();
	SetupClock();
	sei();

	for( tf = 0; tf < 48*24; tf++ )
	{
		framebuffer[tf] = tf;
	}

	while(1)
	{
		if( (tf&0xfff) == 0 ) videobeep=3;
		sprintf( framebuffer, "%d", tf++ );
	}
}


