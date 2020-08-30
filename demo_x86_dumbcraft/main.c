#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <poll.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "dumbcraft.h"
#include "os_generic.h"


//MUST be a power of two.
#define OUTCIRCBUFFSIZE 512

int playersockets[MAX_PLAYERS];

uint8_t sendbuffer[16384];
uint16_t sendptr;
uint8_t sendplayer;

uint8_t  outcirc[OUTCIRCBUFFSIZE];
uint16_t outcirchead = 0;
uint8_t  is_in_outcirc;

void StartupBroadcast() { is_in_outcirc = 1; }
void DoneBroadcast() { is_in_outcirc = 0; }
void PushByte( uint8_t byte );
void extSbyte( uint8_t b )
{
	if( is_in_outcirc )
	{
//		printf( "Broadcast: %02x (%c)\n", b, b );
		outcirc[outcirchead & (OUTCIRCBUFFSIZE-1)] = b;
		outcirchead++;
	} else {
		PushByte( b );
	}
}

uint16_t GetRoomLeft()
{
	return 512-sendptr;
}


uint16_t GetCurrentCircHead()
{
	return outcirchead;
}

uint8_t UnloadCircularBufferOnThisClient( uint16_t * whence )
{
	uint16_t i16;
	uint16_t w = *whence;

	i16 = GetRoomLeft();
	while( w != outcirchead && i16 )
	{
//		printf( "." );
		PushByte( outcirc[(w++)&(OUTCIRCBUFFSIZE-1)] );
		i16--;
	}

	*whence = w;

	if( !i16 )
		return 1;
	else
		return 0;
}

void SendData( uint8_t playerno, unsigned char * data, uint16_t packetsize )
{
	int sock = playersockets[playerno];
	int r;
	if( sock >= 0 )
	{
		r = send( sock, data, packetsize, MSG_NOSIGNAL );
		if( r != packetsize )
		{
			fprintf( stderr, "Error: could not send (%d) code %d (%p %d)\n", sock, r, data, packetsize);
			ForcePlayerClose( playerno, 's' );
		}
	}
}

void SendStart( uint8_t playerno )
{
	sendplayer = playerno;
	sendptr = 0;
}

void PushByte( uint8_t byte )
{
	sendbuffer[sendptr++] = byte;
}

void EndSend( )
{
	if( sendptr != 0 )
		SendData( sendplayer, sendbuffer, sendptr );
	//printf( "\n" );
}

uint8_t CanSend( uint8_t playerno )
{
	return 1;
}


void ForcePlayerClose( uint8_t playerno, uint8_t reason )
{
	RemovePlayer( playerno );
	close( playersockets[playerno] );
	playersockets[playerno] = -1;
}

uint8_t readbuffer[65536];
uint16_t readbufferpos = 0;
uint16_t readbuffersize = 0;

uint8_t Rbyte()
{
	uint8_t rb = readbuffer[readbufferpos++];
//	printf( "[%02x] ", rb );
	return rb;
}

uint8_t CanRead()
{
	return readbufferpos < readbuffersize;
}

int main()
{
	int i;
	struct linger lin;
	int serversocket;
	struct sockaddr_in servaddr;
	struct pollfd fds[1];
	int lastticktime = 0;

	lin.l_onoff=1;
	lin.l_linger=0;

	for( i = 0; i < MAX_PLAYERS; i++ )
		playersockets[i] = -1;

	if( ( serversocket = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
	{
		fprintf( stderr, "Error: Could not create socket.\n" );
		return -1;
	}

	memset( &servaddr, 0, sizeof( servaddr ) );
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl( INADDR_ANY );
	servaddr.sin_port = htons( 25565 );

	if( bind( serversocket, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 )
	{
		fprintf( stderr, "Error: could not bind to socket.\n" );
		return -1;
	}

	if( listen( serversocket, 5 ) < 0 )
	{
		fprintf( stderr, "Error: could not listen to socket.\n" );
		return -1;
	}

	setsockopt( serversocket, SOL_SOCKET, SO_LINGER,(void*)(&lin), sizeof(lin) );  

	InitDumbcraft();

	while(1)
	{
		int rc;
//		double thistime = OGGetAbsoluteTime();
		struct timeval tv;
		gettimeofday( &tv, 0 );

		memset( fds, 0, sizeof( fds ) );
		fds[0].fd = serversocket;
		fds[0].events = POLLIN;

		rc = poll( fds, 1, 0 );

		if( rc < 0 )
		{
			fprintf( stderr, "Error: poll failed on server socket.\n" );
			return -1;
		}
		if( rc != 0 )
		{
			int foundsocketspot;
			int clientsocket = accept( serversocket, 0, 0 );
			setsockopt( clientsocket, SOL_SOCKET, SO_LINGER,(void*)(&lin), sizeof(lin) );  

			if( clientsocket > 0 )
			{
				for( i = 0; i < MAX_PLAYERS; i++ )
				{
					if( playersockets[i] < 0 )
					{
						foundsocketspot = i;
						break;
					}
				}
				if( foundsocketspot != MAX_PLAYERS )
				{
					playersockets[foundsocketspot] = clientsocket;
					AddPlayer( foundsocketspot );
				}
				else
				{
					close( clientsocket );
				}
			}
		}

		for( i = 0; i < MAX_PLAYERS; i++ )
		{
			int sock = playersockets[i];

			if( sock < 0 ) continue;

			fds[0].fd = sock;
			fds[0].events = POLLIN;

			rc = poll( fds, 1, 0 );

			if( rc < 0 )
			{
				ForcePlayerClose( i, 'f' );
				continue;
			}
			else if( rc == 0 )
				continue;

			rc = recv( sock, readbuffer, 65535, 0 );
			if( rc <= 0 )
			{
				ForcePlayerClose( i, 'r' );
				continue;
			}
			readbufferpos = 0;
			readbuffersize = rc;

			if( rc > 0 )
			{
//				printf( "Gotdata %d\n", i );
				GotData( i );
			}
		}

		UpdateServer();

		if( ( ( tv.tv_usec - lastticktime + 1000000 ) % 1000000 ) > 100000 )
		{
			TickServer();
			lastticktime = tv.tv_usec;
		}

		usleep( 100 );
	}
	return 0;
}
