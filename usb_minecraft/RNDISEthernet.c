/*
             LUFA Library
     Copyright (C) Dean Camera, 2014.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2014  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Main source file for the RNDISEthernet demo. This file contains the main tasks of the demo and
 *  is responsible for the initial application hardware configuration.
 */

#include "RNDISEthernet.h"
#include "eth_config.h"
#include "myRNDIS.h"
#include "net_compat.h"
#include "avr_print.h"
#include "iparpetc.h"
#include "tcp_awful.h"
#include "dumbconfig.h"
#include <dumbcraft.h>


#define POP et_pop8()
#define POP16 et_pop16()
#define PUSH(x) et_push8(x)
#define PUSH16(x) et_push16(x)

int8_t lastconnection = -1; //for dumbcraft
uint16_t bytespushed; //for dumbcraft
char my_server_name[3] = { 'h', 'i', 0 };


unsigned char MyMAC[6] = { 0x02, 0x00, 0x02, 0x00, 0x02, 0x00 };
unsigned char MyIP[4]  = { 192, 168, 55, 243 };
unsigned char MyMask[4] = { 255, 255, 255, 0 };

void HandleUDP( uint16_t len )
{
	uint8_t mask;
	uint8_t v;
	/*uint16_t epcs = */et_pop16(); //Checksum
	len -= 8; //remove header.

	if( localport == 13313 )
	{
		//printf( "%d pack\n", len );
	}
}



uint8_t TCPReceiveSyn( uint16_t portno )
{
	if( portno == MINECRAFT_PORT )  //Must bump these up by 8... 
	{
		uint8_t ret = GetFreeConnection();//MyGetFreeConnection(HTTP_CONNECTIONS+1, TCP_SOCKETS );
		if( ret == 0 ) return 0;
		sendstr( "Conn attempt: " );
		sendhex2( ret );
		sendchr( '\n' );
		AddPlayer( ret - HTTP_CONNECTIONS - 1 );
		
		return ret;
	}
#ifndef NO_HTTP
	else if( portno == 80 )
	{
		uint8_t ret = MyGetFreeConnection(1, HTTP_CONNECTIONS+1 );
//		sendchr( 0 ); sendchr( 'x' ); sendhex2( ret );
		HTTPInit( ret-1, ret );
		return ret;
	}
#endif
	return 0;
}

void TCPConnectionClosing( uint8_t conn )
{
	if( conn >= HTTP_CONNECTIONS )
	{
		sendstr( "LOSC\n" );
		RemovePlayer( conn - HTTP_CONNECTIONS - 1 );
	}
#ifndef NO_HTTP
	else
	{
		curhttp = &HTTPConnections[conn-1];
		HTTPClose(  );
	}
#endif
}


///////////////////////////////////////DUMBCRAFT AREA//////////////////////////////////////////

uint8_t disable;

uint8_t CanSend( uint8_t playerno ) //DUMBCRAFT
{
	return TCPCanSend( playerno + HTTP_CONNECTIONS + 1 );
}

void SendStart( uint8_t playerno )  //DUMBCRAFT
{
	bytespushed = 0;
	lastconnection = playerno + HTTP_CONNECTIONS + 1;
	disable = 0;
}

uint8_t incirc = 0;
uint8_t circ_buffer_at = 0;
uint8_t circ_buffer[CIRC_BUFFER_SIZE];

uint16_t GetCurrentCircHead()
{
	return circ_buffer_at;
}

uint8_t UnloadCircularBufferOnThisClient( uint16_t * whence )
{
	//TRICKY: we're in the middle of a transaction now.
	//We need to back out, copy the memory, then pretend nothing happened.
	uint16_t w = *whence;
	uint16_t togomaxB = (circ_buffer_at - w)&((CIRC_BUFFER_SIZE)-1);

	if( togomaxB == 0 ) return 0;
//	printf( "(%d-%d)", circ_buffer_at, *whence );
//	printf( "-%d-", togomaxB );
	while ( togomaxB-- )
	{
		uint8_t cc = circ_buffer[*(whence)];
		PUSH( cc );
//XXX XXX XXX TODO Re-enable sending here.
//		PUSH( '\x05' );
//		sendchr (' ' );
//		sendhex2( cc );
		(*whence)++;
		if( *whence >= CIRC_BUFFER_SIZE ) *whence = 0;
	}

	return 0;
}

void StartupBroadcast()
{
	incirc = 1;
}

void DoneBroadcast()
{
	incirc = 0;
}

void extSbyte( uint8_t byte ) //DUMBCRAFT
{
	if( !incirc )
	{
		if( bytespushed == 0 )
		{
			TCPs[lastconnection].sendtype = ACKBIT | PSHBIT;
			StartTCPWrite( lastconnection );
		}
		bytespushed++;
		PUSH( byte );
	}
	else
	{
		circ_buffer[circ_buffer_at] = byte;
//		sendchr( ':');
//		sendhex2( byte );
		circ_buffer_at++;
		if( circ_buffer_at>= CIRC_BUFFER_SIZE) circ_buffer_at = 0;
	}
}

void EndSend( )  //DUMBCRAFT
{
	if( bytespushed == 0 )
	{
	}
	else
	{
		EndTCPWrite( lastconnection );
	}
}

void ForcePlayerClose( uint8_t playerno, uint8_t reason ) //DUMBCRAFT
{
	sendstr( "FPC\n" );
	RequestClosure( playerno + HTTP_CONNECTIONS );
	RemovePlayer( playerno );	
}


uint16_t totaldatalen;
uint16_t readsofar;

uint8_t Rbyte()  //DUMBCRAFT
{
	if( readsofar++ > totaldatalen ) return 0;
	return POP;
}

uint8_t CanRead()  //DUMBCRAFT
{
	return readsofar < totaldatalen;
}

uint8_t TCPReceiveData( uint8_t connection, uint16_t totallen ) 
{
	if( connection > HTTP_CONNECTIONS ) //DUMBCRAFT
	{
		totaldatalen = totallen;
		readsofar = 0;
		GotData( connection -  HTTP_CONNECTIONS - 1 );
		return 0; //Do this if we didn't send an ack.
	}
#ifndef NO_HTTP
	else
	{
		HTTPGotData( connection-1, totallen );
		return 0;
	}
#endif
	return 0;
}
















/** Main program entry point. This routine configures the hardware required by the application, then
 *  enters a loop to run the application tasks in sequence.
 */
int main(void)
{
	uint8_t cyc;
	SetupHardware();

	/* Webserver Initialization */
		setup_spi();

	DDRD |= _BV(6);
	PORTD &= ~_BV(6);
	PORTD = 0;

	GlobalInterruptEnable();

	//sendstr( "Boot" );

	et_init( 0 );

	InitTCP();
	InitDumbcraft();


	for (;;)
	{
		RNDIS_Task();
		USB_USBTask();
		_delay_us(200);
		UpdateServer();
		cyc++;
		if( (cyc & 0x7f) == 0 )
		{
			struct Player * p = &Players[0];
//			sendhex2( p->npitch );
//			 = 1;
//			if( p->active )

			PORTD |= _BV(6);
			TickServer();
			TickTCP();
			PORTD &= ~_BV(6);
		}
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Hardware Initialization */

	USB_Init();
	Serial_CreateStream(NULL);
}

/** Event handler for the USB_Connect event. This indicates that the device is enumerating via the status LEDs and
 *  starts the library USB task to begin the enumeration and USB management process.
 */
void EVENT_USB_Device_Connect(void)
{
	/* Indicate USB enumerating */
//	LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the USB_Disconnect event. This indicates that the device is no longer connected to a host via
 *  the status LEDs and stops all the relevant tasks.
 */
void EVENT_USB_Device_Disconnect(void)
{
	/* Indicate USB not ready */
//	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the USB_ConfigurationChanged event. This is fired when the host sets the current configuration
 *  of the USB device after enumeration, and configures the RNDIS device endpoints and starts the relevant tasks.
 */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	/* Setup RNDIS Data Endpoints */
	ConfigSuccess &= Endpoint_ConfigureEndpoint(CDC_TX_EPADDR, EP_TYPE_BULK, CDC_TXRX_EPSIZE, 1);
	ConfigSuccess &= Endpoint_ConfigureEndpoint(CDC_RX_EPADDR, EP_TYPE_BULK, CDC_TXRX_EPSIZE, 1);
	ConfigSuccess &= Endpoint_ConfigureEndpoint(CDC_NOTIFICATION_EPADDR, EP_TYPE_INTERRUPT, CDC_NOTIFICATION_EPSIZE, 1);

	/* Indicate endpoint configuration success or failure */
//	LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the USB_ControlRequest event. This is used to catch and process control requests sent to
 *  the device from the USB host before passing along unhandled control requests to the library for processing
 *  internally.
 */
void EVENT_USB_Device_ControlRequest(void)
{
	/* Process RNDIS class commands */
	switch (USB_ControlRequest.bRequest)
	{
		case RNDIS_REQ_SendEncapsulatedCommand:
			if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				Endpoint_ClearSETUP();

				/* Read in the RNDIS message into the message buffer */
				Endpoint_Read_Control_Stream_LE(RNDISMessageBuffer, USB_ControlRequest.wLength);
				Endpoint_ClearIN();

				/* Process the RNDIS message */
				ProcessRNDISControlMessage();
			}

			break;
		case RNDIS_REQ_GetEncapsulatedResponse:
			if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				/* Check if a response to the last message is ready */
				if (!(MessageHeader->MessageLength))
				{
					/* Set the response to a single 0x00 byte to indicate that no response is ready */
					RNDISMessageBuffer[0] = 0;
					MessageHeader->MessageLength = 1;
				}

				Endpoint_ClearSETUP();

				/* Write the message response data to the endpoint */
				Endpoint_Write_Control_Stream_LE(RNDISMessageBuffer, MessageHeader->MessageLength);
				Endpoint_ClearOUT();

				/* Reset the message header once again after transmission */
				MessageHeader->MessageLength = 0;
			}

			break;
	}
}

/** Task to manage the sending and receiving of encapsulated RNDIS data and notifications. This removes the RNDIS
 *  wrapper from received Ethernet frames and places them in the FrameIN global buffer, or adds the RNDIS wrapper
 *  to a frame in the FrameOUT global before sending the buffer contents to the host.
 */

void RNDIS_Task(void)
{
	Endpoint_SelectEndpoint(CDC_NOTIFICATION_EPADDR);

	if (USB_DeviceState != DEVICE_STATE_Configured)
	  return;

	HandleUSB();
}



