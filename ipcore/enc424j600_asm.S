// Copyright 2012 Charles Lohr under the MIT/x11, newBSD, LGPL or GPL licenses.  You choose.

//XXX TODO In SPIW, should we be setting ETSI low on start in r18??

#define _SFR_ASM_COMPAT 1
#define __SFR_OFFSET 0

#include <avr/io.h>

#define ASSEMBLY
#include "enc424j600.h"

#ifdef ASM_SPI

.global espiR_asm
espiR_asm:

//Return value in R24

#ifdef ASM_REALLY_FAST_SPI
	in r18, ETPORT
	andi r18, ~(ETSCK)
	mov r19, r18
	ori r19, ETSCK

	clr r24
	out ETPORT, r18
	out ETPORT, r19
	sbic ETPIN, ETSOP
	ori r24, 0x80
	out ETPORT, r18
	out ETPORT, r19
	sbic ETPIN, ETSOP
	ori r24, 0x40
	out ETPORT, r18
	out ETPORT, r19
	sbic ETPIN, ETSOP
	ori r24, 0x20
	out ETPORT, r18
	out ETPORT, r19
	sbic ETPIN, ETSOP
	ori r24, 0x10
	out ETPORT, r18
	out ETPORT, r19
	sbic ETPIN, ETSOP
	ori r24, 0x08
	out ETPORT, r18
	out ETPORT, r19
	sbic ETPIN, ETSOP
	ori r24, 0x04
	out ETPORT, r18
	out ETPORT, r19
	sbic ETPIN, ETSOP
	ori r24, 0x02
	out ETPORT, r18
	out ETPORT, r19
	sbic ETPIN, ETSOP
	ori r24, 0x01
	out ETPORT, r18


	ret
#else
	clr r24              // 1 cycle 
	cbi ETPORT, ETSCKP   // 2 cycles
	sbi ETPORT, ETSCKP   // 2 cycles
	sbic ETPIN, ETSOP    // 1 cycle
	ori r24, 0x80        // 1 cycle
	cbi ETPORT, ETSCKP
	sbi ETPORT, ETSCKP
	sbic ETPIN, ETSOP
	ori r24, 0x40
	cbi ETPORT, ETSCKP
	sbi ETPORT, ETSCKP
	sbic ETPIN, ETSOP
	ori r24, 0x20
	cbi ETPORT, ETSCKP
	sbi ETPORT, ETSCKP
	sbic ETPIN, ETSOP
	ori r24, 0x10
	cbi ETPORT, ETSCKP
	sbi ETPORT, ETSCKP
	sbic ETPIN, ETSOP
	ori r24, 0x08
	cbi ETPORT, ETSCKP
	sbi ETPORT, ETSCKP
	sbic ETPIN, ETSOP
	ori r24, 0x04
	cbi ETPORT, ETSCKP
	sbi ETPORT, ETSCKP
	sbic ETPIN, ETSOP
	ori r24, 0x02
	cbi ETPORT, ETSCKP
	sbi ETPORT, ETSCKP
	sbic ETPIN, ETSOP
	ori r24, 0x01
	cbi ETPORT, ETSCKP
	ret
#endif

.global espiW_asm
espiW_asm:
#ifdef ASM_REALLY_FAST_SPI


	in r18, ETPORT
	andi r18, ~(ETSCK)
	mov r20, r18
	ori r20, ETSI

	out ETPORT, r18     //SCK is low

	sbrc r24, 7
	out ETPORT, r20     //Set SI high
	sbi ETPORT, ETSCKP  //Set SCK high
	out ETPORT, r18     //Set SCK Low

	sbrc r24, 6
	out ETPORT, r20
	sbi ETPORT, ETSCKP
	out ETPORT, r18

	sbrc r24, 5
	out ETPORT, r20
	sbi ETPORT, ETSCKP
	out ETPORT, r18

	sbrc r24, 4
	out ETPORT, r20
	sbi ETPORT, ETSCKP
	out ETPORT, r18

	sbrc r24, 3
	out ETPORT, r20
	sbi ETPORT, ETSCKP
	out ETPORT, r18

	sbrc r24, 2
	out ETPORT, r20
	sbi ETPORT, ETSCKP
	out ETPORT, r18

	sbrc r24, 1
	out ETPORT, r20
	sbi ETPORT, ETSCKP
	out ETPORT, r18

	sbrc r24, 0
	out ETPORT, r20
	sbi ETPORT, ETSCKP
	out ETPORT, r18

	ret
#else
	cbi ETPORT, ETSCKP
	cbi ETPORT, ETSIP
	sbrc r24, 7
	sbi ETPORT, ETSIP
	sbi ETPORT, ETSCKP
	cbi ETPORT, ETSCKP

	cbi ETPORT, ETSIP
	sbrc r24, 6
	sbi ETPORT, ETSIP
	sbi ETPORT, ETSCKP
	cbi ETPORT, ETSCKP

	cbi ETPORT, ETSIP
	sbrc r24, 5
	sbi ETPORT, ETSIP
	sbi ETPORT, ETSCKP
	cbi ETPORT, ETSCKP

	cbi ETPORT, ETSIP
	sbrc r24, 4
	sbi ETPORT, ETSIP
	sbi ETPORT, ETSCKP
	cbi ETPORT, ETSCKP

	cbi ETPORT, ETSIP
	sbrc r24, 3
	sbi ETPORT, ETSIP
	sbi ETPORT, ETSCKP
	cbi ETPORT, ETSCKP

	cbi ETPORT, ETSIP
	sbrc r24, 2
	sbi ETPORT, ETSIP
	sbi ETPORT, ETSCKP
	cbi ETPORT, ETSCKP

	cbi ETPORT, ETSIP
	sbrc r24, 1
	sbi ETPORT, ETSIP
	sbi ETPORT, ETSCKP
	cbi ETPORT, ETSCKP

	cbi ETPORT, ETSIP
	sbrc r24, 0
	sbi ETPORT, ETSIP
	sbi ETPORT, ETSCKP
	cbi ETPORT, ETSCKP

	ret
#endif

#endif
