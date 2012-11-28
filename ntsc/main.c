 /* General demo program, Copyleft 2008 Charles Lohr */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>

#include "ntsc.h"
#include <stdio.h>


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

	while(1)
	{
		sprintf( framebuffer, "%d", tf++ );
	}
}


