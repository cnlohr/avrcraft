#include "net_compat.h"
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <string.h>
#include <LUFA/Drivers/USB/USB.h>
#include "myRNDIS.h"
#include "Descriptors.h"


unsigned char ETbuffer[ETBUFFERSIZE];
unsigned short ETsendplace;
uint16_t sendbaseaddress;
unsigned short ETchecksum;
uint16_t FrameInLength;
unsigned short FrameInRemain;
unsigned short FrameOutLength;
unsigned short FrameOutStart;

uint8_t IsSendBufferFree(){ return FrameOutLength == 0; }
//For sending packets:
// Ptr: &ETbuffer[FrameOutStart] Len: FrameOutLength

void HandleUSB()
{

	/* Check if a message response is ready for the host */
	if (Endpoint_IsINReady() && ResponseReady)
	{
		USB_Request_Header_t Notification = (USB_Request_Header_t)
			{
				.bmRequestType = (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE),
				.bRequest      = RNDIS_NOTIF_ResponseAvailable,
				.wValue        = 0,
				.wIndex        = 0,
				.wLength       = 0,
			};

		/* Indicate that a message response is ready for the host */
		Endpoint_Write_Stream_LE(&Notification, sizeof(Notification), NULL);

		/* Finalize the stream transfer to send the last packet */
		Endpoint_ClearIN();

		/* Indicate a response is no longer ready */
		ResponseReady = false;
	}

	/* Don't process the data endpoints until the system is in the data initialized state, and the buffer is free */
	if ((CurrRNDISState == RNDIS_Data_Initialized) ) //&& !(MessageHeader->MessageLength))
	{

		/* Create a new packet header for reading/writing */
		RNDIS_Packet_Message_t RNDISPacketHeader;

		/* Select the data OUT endpoint */
		Endpoint_SelectEndpoint(CDC_RX_EPADDR);

		/* Check if the data OUT endpoint contains data, and that the IN buffer is empty */
		if (Endpoint_IsOUTReceived() ) // && !FrameInLength
		{
			/* Read in the packet message header */
			Endpoint_Read_Stream_LE(&RNDISPacketHeader, sizeof(RNDIS_Packet_Message_t), NULL);

			/* Stall the request if the data is too large */
/*			if (RNDISPacketHeader.DataLength > ETHERNET_FRAME_SIZE_MAX)
			{
				Endpoint_StallTransaction();
				return;
			}*/

			FrameInLength = RNDISPacketHeader.DataLength;

			Endpoint_WaitUntilReady();
			//Pick up here.

			FrameInRemain = FrameInLength;
//			printf( "GP: %d\n", FrameInLength );
			et_receivecallback( FrameInRemain );

			while( FrameInRemain )
				et_pop8();

			/* Finalize the stream transfer to send the last packet */
			Endpoint_ClearOUT();

			FrameInLength = 0;
		}

		/* Select the data IN endpoint */
		Endpoint_SelectEndpoint(CDC_TX_EPADDR);

		/* Check if the data IN endpoint is ready for more data, and that the IN buffer is full */
		if (Endpoint_IsINReady() && FrameOutLength)
		{
			/* Clear the packet header with all 0s so that the relevant fields can be filled */
			memset(&RNDISPacketHeader, 0, sizeof(RNDIS_Packet_Message_t));

			/* Construct the required packet header fields in the buffer */
			RNDISPacketHeader.MessageType   = REMOTE_NDIS_PACKET_MSG;
			RNDISPacketHeader.MessageLength = (sizeof(RNDIS_Packet_Message_t) + FrameOutLength );
			RNDISPacketHeader.DataOffset    = (sizeof(RNDIS_Packet_Message_t) - sizeof(RNDIS_Message_Header_t));
			RNDISPacketHeader.DataLength    = FrameOutLength;

			/* Send the packet header to the host */
			Endpoint_Write_Stream_LE(&RNDISPacketHeader, sizeof(RNDIS_Packet_Message_t), NULL);

			/* Send the Ethernet frame data to the host */
			Endpoint_Write_Stream_LE(&ETbuffer[sendbaseaddress], RNDISPacketHeader.DataLength, NULL);

			/* Finalize the stream transfer to send the last packet */
			Endpoint_ClearIN();

			/* Indicate Ethernet OUT buffer no longer full */
			FrameOutLength = 0;
		}
	}
}



//Internal functs

uint32_t crc32b(uint32_t crc, unsigned char *message, int len) {
   int i, j;
   uint32_t mask;
	uint8_t byte;

   i = 0;
//   crc = 0xFFFFFFFF;
	crc = ~crc;
   while (i < len) {
      byte = message[i];            // Get next byte.
      crc = crc ^ byte;
      for (j = 7; j >= 0; j--) {    // Do eight times.
         mask = -(crc & 1);
         crc = (crc >> 1) ^ (0xEDB88320 & mask);
      }
      i = i + 1;
   }
   return ~crc;
}

uint16_t internet_checksum( const unsigned char * start, uint16_t len )
{
	uint16_t i;
	const uint16_t * wptr = (uint16_t*) start;
	uint32_t csum = 0;
	for (i=1;i<len;i+=2)
	{
		csum = csum + (uint32_t)(*(wptr++));	
	}
	if( len & 1 )  //See if there's an odd number of bytes?
	{
		uint8_t * tt = (uint8_t*)wptr;
		csum += *tt;
	}
	while (csum>>16)
		csum = (csum & 0xFFFF)+(csum >> 16);
	csum = (csum>>8) | ((csum&0xff)<<8);
	return ~csum;
}



void et_dumpbytes( uint8_t len )
{
	while( len-- )
		et_pop8();
}

uint8_t et_pop8()
{
	if( !FrameInRemain ) return 0;

	while (!(Endpoint_IsReadWriteAllowed()))
	{
		Endpoint_ClearOUT();

		if ((Endpoint_WaitUntilReady()))
		{
			//XXX XXX SERIOUS PROBLEM IF WE HANDLE THIS WRONG.
			return 0;
		}
	}
	FrameInRemain--;
	return Endpoint_Read_8();
}


uint16_t et_pop16()
{
	uint16_t ret = et_pop8();
	return (ret<<8) | et_pop8();
}

void et_popblob( uint8_t * data, uint8_t len )
{
	while( len-- )
	{
		*(data++) = et_pop8();
	}
}

void et_pushpgmstr( const char * msg )
{
	uint8_t r;
	do
	{
		r = pgm_read_byte(msg++);
		if( !r ) break;
		et_push8( r );
	} while( 1 );
}

void et_pushpgmblob( const uint8_t * data, uint8_t len )
{
	while( len-- )
	{
		et_push8( pgm_read_byte(data++) );
	}
}


void et_pushstr( const char * msg )
{
	for( ; *msg; msg++ ) 
		et_push8( *msg );
}

void et_pushblob( const uint8_t * data, uint8_t len )
{
	while( len-- )
	{
		et_push8( *(data++) );
	}
}

void et_push16( uint16_t p )
{
	et_push8( p>>8 );
	et_push8( p&0xff );
}


//return 0 if OK, otherwise nonzero.
int8_t et_init( const unsigned char * macaddy )
{
	if( macaddy )
	{
		MyMAC[0] = macaddy[0];
		MyMAC[1] = macaddy[1];
		MyMAC[2] = macaddy[2];
		MyMAC[3] = macaddy[3];
		MyMAC[4] = macaddy[4];
		MyMAC[5] = macaddy[5];
	}

	return 0;
}

int8_t et_xmitpacket( uint16_t start, uint16_t len )
{
	//If we're here, ETbuffer[start] points to the first byte (dst MAC address)
	//Gotta calculate the checksum.
	if( FrameOutLength ) return 1;

	FrameOutLength = len;
	FrameOutStart = start;
	return 0;
}

unsigned short et_recvpack()
{
	//Stub function.
	return 0;
}

void et_start_checksum( uint16_t start, uint16_t len )
{
	uint16_t i;
	const uint16_t * wptr = (uint16_t*)&ETbuffer[start];
	uint32_t csum = 0;
	for (i=1;i<len;i+=2)
	{
		csum = csum + (uint32_t)(*(wptr++));	
	}
	if( len & 1 )  //See if there's an odd number of bytes?
	{
		uint8_t * tt = (uint8_t*)wptr;
		csum += *tt;
	}
	while (csum>>16)
		csum = (csum & 0xFFFF)+(csum >> 16);
	csum = (csum>>8) | ((csum&0xff)<<8);
	ETchecksum = ~csum;
}



#ifndef NOCOPY_MAC
void et_copy_memory( uint16_t to, uint16_t from, uint16_t length, uint16_t range_start, uint16_t range_end )
{
	uint16_t i;
	if( to == from )
	{
		return;
	}
	else if( to < from )
	{
		for( i = 0; i < length; i++ )
		{
			ETbuffer[to++] = ETbuffer[from++];
		}
	}
	else
	{
		to += length;
		from += length;
		for( i = 0; i < length; i++ )
		{
			ETbuffer[to--] = ETbuffer[from--];
		}
	}
}
#endif

void et_write_ctrl_reg16( uint8_t addy, uint16_t value )
{
	switch (addy )
	{
		case EERXRDPTL:
		case EEGPWRPTL:
			ETsendplace = value;
		default:
			break;
	}
}

uint16_t et_read_ctrl_reg16( uint8_t addy )
{
	switch( addy )
	{
		case EERXRDPTL:
		case EEGPWRPTL:
			return ETsendplace;
		default:
			return 0;
	}
}



/*
     Modification of "avrcraft" IP Stack.
    Copyright (C) 2014 <>< Charles Lohr
		CRC Code from: http://www.hackersdelight.org/hdcodetxt/crc.c.txt

    Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"),
	to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense,
	and/or sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
	in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
	CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
	TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/










