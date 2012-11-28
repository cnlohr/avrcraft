//Standard header definitions, no Copyright applies.
//It is in the public domain.
//Transcribed manually from datasheet by Charles Lohr

#ifndef _ENC28J60_REGS_H
#define _ENC28J60_REGS_H

//SPI Commands 

//Single-byte commands
#define ESB0SEL      0b11000000  //Select bank 0
#define ESB1SEL      0b11000010
#define ESB2SEL      0b11000100
#define ESB3SEL      0b11000110  //Select bank 3
#define ESSETETHRST  0b11001010  //Issue system reset
#define ESFCDISABLE  0b11100000  //Disable flow control
#define ESFCSINGLE   0b11100010  //Transmit one pause frame
#define ESFCMULTIPLE 0b11100100  //Enables flow control with periodic pause frames
#define ESFCCLEAR    0b11100110  //Terminates flow control with a final pause frame
#define ESSETPKTDEC  0b11001100  //Decrements PKTCNT by setting PKTDEC
#define ESDMASTOP    0b11010010  //Stop DMA transfer
#define ESDMACKSUM   0b11011000  //Start DMA checksum (with seed)
#define ESDMACKSUMS  0b11011010  //Start DMA checksum (with seed)
#define ESDMACOPY    0b11011100  //Start DMA copy (and checksum)
#define ESDMACOPYS   0b11011110  //Start a DMA copy+checksum+seed
#define ESSETTXRTS   0b11010100  //Sets TXRTS (sends ethernet packet)
#define ESENABLERX   0b11101000  //Enables RX (sets RXEN)
#define ESDISABLERX  0b11101010  //Enables RX (sets RXEN)
#define ESSETEIE     0b11101100  //Enable ethernet interrupts by setting INT
#define ESCLREIE     0b11101110  //Disable Ethernet interrupts.

//Read the selected bank.  (returns bank #) (two-byte command)
#define EDRBSEL      0b11001000

//Three-byte commands
#define EWGPRDPT    0b01100000  //Write general purpose register
#define ERGPRDPT    0b01100010  //Read general purpose register
#define EWRXRDPT    0b01100100  //Write Receive Buffer Read Pointer
#define ERRXRDPT    0b01100110  //Read receive buffer read pointer
#define EWIDARDPT   0b01101000  //Write user-defined area read pointer
#define ERUDARDPT   0b01101010  //Read user-defined area read pointer
#define EWGPWRPT    0b01101100  //Write general purpose buffer write pointer
#define ERGPWRPT    0b01101110  //Read general purpose buffer write pointer
#define EWRXWRPT    0b01110000  //Write receive buffer write pointer
#define ERRXWRPT    0b01110100  //Read Receive Buffer Write Pointer
#define EWUDAWRPT   0b01110100  //Write User-defined area write pointer
#define ERUDAWRPT   0b01110110  //read user defined area write pointer

//Common commands
#define ERCR	0x00	//Read Control Register
#define EWCR	0x40	//Write Control Register
#define EBFS	0x80	//Bit Field Set
#define EBFC	0xa0	//Bit Field Clear

#define ERCRU   0x20    //Read control register unbanked
#define EWCRU   0x22    //Write control register unbanked
#define EBFSU   0x24    //Set Bitfield Unbanked
#define EBFCU   0x26    //Clear bitfield unbanked

#define ERGPDATA  0b00101000 //Read EGPDATA
#define EWGPDATA  0b00101010 //Write EGPDATA
#define ERRXDATA  0b00101100 //Read ERXDATA
#define EWRXDATA  0b00101110 //Write ERXDATA
#define ERUDADATA 0b00110000 //Read EUDADATA
#define EWUDADATA 0b00110010 //Write EUDADATA



//PHY Registers
#define EPHCON1  0x00
#define EPHSTAT  0x01
#define EPHANA   0x04
#define EPHANLPA 0x05
#define EPHANE   0x06
#define EPHCON2  0x11
#define EPHSTAT2 0x1B
#define EPHSTAT3 0x1F



//Bank 1
#define EETXSTL     (0x00 | 0x00)
#define EETXSTH     (0x01 | 0x00)
#define EETXLENL    (0x02 | 0x00)
#define EETXLENH    (0x03 | 0x00)
#define EERXSTL     (0x04 | 0x00)
#define EERXSTH     (0x05 | 0x00)
#define EERXTAILL   (0x06 | 0x00)
#define EERXTAILH   (0x07 | 0x00)
#define EERXHEADL   (0x08 | 0x00)
#define EERXHEADH   (0x09 | 0x00)
#define EEDMASTL    (0x0A | 0x00)
#define EEDMASTH    (0x0B | 0x00)
#define EEDMALENL   (0x0C | 0x00)
#define EEDMALENH   (0x0D | 0x00)
#define EEDMADSTL   (0x0E | 0x00)
#define EEDMADSTH   (0x0F | 0x00)
#define EEDMACSL    (0x10 | 0x00)
#define EEDMACSH    (0x11 | 0x00)
#define EETXSTATL   (0x12 | 0x00)
#define EETXSTATH   (0x13 | 0x00)
#define EETXWIREL   (0x14 | 0x00)
#define EETXWIREH   (0x15 | 0x00)

//Common registers
#define EEUDASTL    (0x16 | 0x00)
#define EEUDASTH    (0x17 | 0x00)
#define EEUDANDL    (0x18 | 0x00)
#define EEUDANDH    (0x19 | 0x00)
#define EESTATL     (0x1A | 0x00)
#define EESTATH     (0x1B | 0x00)
#define EEIRL       (0x1C | 0x00)
#define EEIRH       (0x1D | 0x00)
#define EECON1L     (0x1E | 0x00)
#define EECON1H     (0x1F | 0x00)


//Bank 2
#define EEHT1L      (0x00 | 0x20)
#define EEHT1H      (0x01 | 0x20)
#define EEHT2L      (0x02 | 0x20)
#define EEHT2H      (0x03 | 0x20)
#define EEHT3L      (0x04 | 0x20)
#define EEHT3H      (0x05 | 0x20)
#define EEHT4L      (0x06 | 0x20)
#define EEHT4H      (0x07 | 0x20)
#define EEPMM1L     (0x08 | 0x20)
#define EEPMM1H     (0x09 | 0x20)
#define EEPMM2L     (0x0A | 0x20)
#define EEPMM2H     (0x0B | 0x20)
#define EEPMM3L     (0x0C | 0x20)
#define EEPMM3H     (0x0D | 0x20)
#define EEPMM4L     (0x0E | 0x20)
#define EEPMM4H     (0x0F | 0x20)
#define EEPMCSL     (0x10 | 0x20)
#define EEPMCSH     (0x11 | 0x20)
#define EEPMOL      (0x12 | 0x20)
#define EEPMOH      (0x13 | 0x20)
#define EERXFCONL   (0x14 | 0x20)
#define EERXFCONH   (0x15 | 0x20)

//Bank 3
#define EMACON1L    (0x00 | 0x40)
#define EMACON1H    (0x01 | 0x40)
#define EMACON2L    (0x02 | 0x40)
#define EMACON2H    (0x03 | 0x40)
#define EMABBIPGL   (0x04 | 0x40)
#define EMABBIPGH   (0x05 | 0x40)
#define EMAIPGL     (0x06 | 0x40)
#define EMAIPGH     (0x07 | 0x40)
#define EMACLCONL   (0x08 | 0x40)
#define EMACLCONH   (0x09 | 0x40)
#define EMAMXFLL    (0x0A | 0x40)
#define EMAMXFLH    (0x0B | 0x40)
//...
#define EMICMDL     (0x12 | 0x40)
#define EMICMDH     (0x13 | 0x40)
#define EMIREGADRL  (0x14 | 0x40)
#define EMIREGADRH  (0x15 | 0x40)

//Bank 4
#define EMAADR3L    (0x00 | 0x60)
#define EMAADR3H    (0x01 | 0x60)
#define EMAADR2L    (0x02 | 0x60)
#define EMAADR2H    (0x03 | 0x60)
#define EMAADR1L    (0x04 | 0x60)
#define EMAADR1H    (0x05 | 0x60)
#define EMIWRL      (0x06 | 0x60)
#define EMIWRH      (0x07 | 0x60)
#define EMIRDL      (0x08 | 0x60)
#define EMIRDH      (0x09 | 0x60)
#define EMISTATL    (0x0A | 0x60)
#define EMISTATH    (0x0B | 0x60)
#define EEPAUSL     (0x0C | 0x60)
#define EEPAUSH     (0x0D | 0x60)
#define EECON2L     (0x0E | 0x60)
#define EECON2H     (0x0F | 0x60)
#define EERXWML     (0x10 | 0x60)
#define EERXWMH     (0x11 | 0x60)
#define EEIEL       (0x12 | 0x60)
#define EEIEH       (0x13 | 0x60)
#define EEIDLEDL    (0x14 | 0x60)
#define EEIDLEDH    (0x15 | 0x60)

//Unbanked
#define EEGPDATA    (0x80)
#define EERXDATA    (0x82)
#define EEUDADATA   (0x84)
#define EEGPRDPTL   (0x86)
#define EEGPRDPTH   (0x87)
#define EEGPWRPTL   (0x88)
#define EEGPWRPTH   (0x89)
#define EERXRDPTL   (0x8A)
#define EERXRDPTH   (0x8B)
#define EERXWRPTL   (0x8C)
#define EERXWRPTH   (0x8D)
#define EEUDARDPTL  (0x8E)
#define EEUDARDPTH  (0x8F)
#define EEUDAWRPTL  (0x90)
#define EEUDAWRPTH  (0x91)



/*

//PHLCON

#define EPHCON1     0x00
#define EPHSTAT1    0x01
#define EPHID1      0x02
#define EPHID2      0x03
#define EPHCON2     0x10	
#define EPHSTAT2    0x11
#define EPHIE       0x12
#define EPHIR       0x13
#define EPHLCON    0x14

#define PHCON2_HDLDIS    0x0100

//Other controls

#define ENEEDDUMMYFLAG 0x80


#define EERDPTL		(0x00|0x00)
#define EERDPTH		(0x01|0x00)
#define EEWRPTL		(0x02|0x00)
#define EEWRPTH		(0x03|0x00)
#define EETXSTL		(0x04|0x00)
#define EETXSTH		(0x05|0x00)
#define EETXNDL		(0x06|0x00)
#define EETXNDH		(0x07|0x00)
#define EERXSTL		(0x08|0x00)
#define EERXSTH		(0x09|0x00)
#define EERXNDL		(0x0A|0x00)
#define EERXNDH		(0x0B|0x00)
#define EERXRDPTL	(0x0C|0x00)
#define EERXRDPTH	(0x0D|0x00)
#define EERXWRPTL	(0x0E|0x00)
#define EERXWRPTH	(0x0F|0x00)
#define EEDMASTL	(0x10|0x00)
#define EEDMASTH	(0x11|0x00)
#define EEDMANDL	(0x12|0x00)
#define EEDMANDH	(0x13|0x00)
#define EEDMADSTL	(0x14|0x00)
#define EEDMADSTH	(0x15|0x00)
#define EEDMACSL	(0x16|0x00)
#define EEDMACSH	(0x17|0x00)

//NOTE: Global Registers
#define EEIE		(0x1B|0x00)
#define EEIR		(0x1C|0x00)
#define EESTAT		(0x1D|0x00)
#define EECON2		(0x1E|0x00)
#define EECON1		(0x1F|0x00)

#define EETH0		(0x00|0x20)
#define EETH1		(0x01|0x20)
#define EETH2		(0x02|0x20)
#define EETH3		(0x03|0x20)
#define EETH4		(0x04|0x20)
#define EETH5		(0x05|0x20)
#define EETH6		(0x06|0x20)
#define EETH7		(0x07|0x20)
#define EEPMM0		(0x08|0x20)
#define EEPMM1		(0x09|0x20)
#define EEPMM2		(0x0a|0x20)
#define EEPMM3		(0x0b|0x20)
#define EEPMM4		(0x0c|0x20)
#define EEPMM5		(0x0d|0x20)
#define EEPMM6		(0x0e|0x20)
#define EEPMM7		(0x0f|0x20)
#define EEPMCSL		(0x10|0x20)
#define EEPMCSH		(0x11|0x20)
#define EEPMOL		(0x14|0x20)
#define EEPMOH		(0x15|0x20)
#define EERXFCON	(0x18|0x20)
#define EEPKTCNT	(0x19|0x20)

#define EMACON1		(0x00|0x40|ENEEDDUMMYFLAG)
#define EMACON2		(0x01|0x40|ENEEDDUMMYFLAG)
#define EMACON3		(0x02|0x40|ENEEDDUMMYFLAG)
#define EMACON4		(0x03|0x40|ENEEDDUMMYFLAG)
#define EMABBIPG	(0x04|0x40|ENEEDDUMMYFLAG)
#define EMAIPGL		(0x06|0x40|ENEEDDUMMYFLAG)
#define EMAIPGH		(0x07|0x40|ENEEDDUMMYFLAG)
#define EMACLCON1	(0x08|0x40|ENEEDDUMMYFLAG)
#define EMACLCON2	(0x09|0x40|ENEEDDUMMYFLAG)
#define EMAMXFLL	(0x0a|0x40|ENEEDDUMMYFLAG)
#define EMAMXFLH	(0x0b|0x40|ENEEDDUMMYFLAG)
#define EMICMD		(0x12|0x40|ENEEDDUMMYFLAG)
#define EMIREGADR	(0x14|0x40|ENEEDDUMMYFLAG)
#define EMIWRL		(0x16|0x40|ENEEDDUMMYFLAG)
#define EMIWRH		(0x17|0x40|ENEEDDUMMYFLAG)
#define EMIRDL		(0x18|0x40|ENEEDDUMMYFLAG)
#define EMIRDH		(0x19|0x40|ENEEDDUMMYFLAG)

#define EMAADR5		(0x00|0x60|ENEEDDUMMYFLAG)
#define EMAADR6		(0x01|0x60|ENEEDDUMMYFLAG)
#define EMAADR3		(0x02|0x60|ENEEDDUMMYFLAG)
#define EMAADR4		(0x03|0x60|ENEEDDUMMYFLAG)
#define EMAADR1		(0x04|0x60|ENEEDDUMMYFLAG)
#define EMAADR2		(0x05|0x60|ENEEDDUMMYFLAG)
#define EEBSTSD		(0x06|0x60)
#define EEBSTCON	(0x07|0x60)
#define EEBSTCSL	(0x08|0x60)
#define EEBSTCSH	(0x09|0x60)
#define EMISTAT		(0x0a|0x60|ENEEDDUMMYFLAG)
#define EEREVID		(0x12|0x60)
#define EECOCON		(0x15|0x60)
#define EEFLOCON	(0x17|0x60)
#define EEPAUSL		(0x18|0x60)
#define EEPAUSH		(0x19|0x60)

#define EBANK(x) ((x&0x60)>>5)
#define EREG(x)  (x&0x1F)
#define ENEEDDUMMY(x) (x&0x80)

//For EIE
#define EINTIE	0x80
#define EPKTIE	0x40
#define EDMAIE	0x20
#define ELINKIE	0x10
#define ETXIE	0x08
#define ETXERIE	0x02
#define ERXERIE	0x01

//For EIR
#define EPKTIF	0x40
#define EDMAIF	0x20
#define ELINKIF	0x10
#define ETXIF	0x08
#define ETXERIF	0x02
#define ERXERIF	0x01

//For ESTAT
#define EINT	0x80
#define EBUFFER	0x40
#define ELATECOL	0x10
#define ERXBUSY	0x04
#define ETXABRT	0x02
#define ECLKRDY	0x01

//For ECON2
#define EAUTOINC	0x80
#define EPKTDEC	0x40
#define EPWRSV	0x20
#define EVRPS	0x08

//For ECON1
#define ETXRST	0x80
#define ERXRST	0x40
#define EDMAST	0x20
#define ECSUMEN	0x10
#define ETXRTS	0x08
#define ERXEN	0x04
#define EBSEL1	0x02
#define EBSEL0	0x01

//For ERXFCON
#define EUCEN	0x80
#define EANDOR	0x40
#define ECRCEN	0x20
#define EPMEN	0x10
#define EMPEN	0x08
#define EHTEN	0x04
#define EMCEN	0x02
#define EBCEN	0x01

//For MACON1
#define ETXPAUS	0x08
#define ERXPAUS	0x04
#define EPASSALL	0x02
#define EMARXEN	0x01

//FOR MACON3
#define EPADCFG2	0x80
#define EPADCFG1	0x40
#define EPADCFG0	0x20
#define ETXCRCEN	0x10
#define EPHDREN	0x08
#define EHFRMEN	0x04
#define EFRMLNEN	0x02
#define EFULDPX	0x01

//For MACON4
#define EDEFER	0x04
#define EBPEN	0x02
#define ENOBKOFF	0x01

//For MICMD
#define EMIISCAN	0x02
#define EMIIRD	0x01

//For EBSTCON
#define EPSV2	0x80
#define EPSV1	0x40
#define EPSV0	0x20
#define EPSEL	0x10
#define ETMSEL1	0x08
#define ETMSEL0	0x04
#define ETME	0x02
#define EBISTST	0x01

//For MISTAT
#define ENVALID	0x04
#define ESCAN	0x02
#define EBUSY	0x01

//For ECOCON
#define ECOCON2	0x04
#define ECOCON1	0x02
#define ECOCON0 0x01

//For EFLOCON
#define EFULDPXS	0x04
#define EFCEN1	0x02
#define EFCEN0	0x01

*/
#endif

