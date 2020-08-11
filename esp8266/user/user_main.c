//Copyright 2015, 2018 <>< Charles Lohr, Adam Feinstein see LICENSE file.

/*==============================================================================
 * Includes
 *============================================================================*/

#include "mem.h"
#include "c_types.h"
#include "user_interface.h"
#include "ets_sys.h"
#include "uart.h"
#include "osapi.h"
#include "espconn.h"
#include "esp82xxutil.h"
#include "commonservices.h"
#include "vars.h"
#include <mdns.h>
#include <string.h>
#include <dumbcraft.h>
#include "dumbconfig.h"

/*==============================================================================
 * Process Defines
 *============================================================================*/

#define procTaskPrio        0
#define procTaskQueueLen    1
os_event_t    procTaskQueue[procTaskQueueLen];

/*==============================================================================
 * Variables
 *============================================================================*/

static os_timer_t some_timer;
static struct espconn *pUdpServer;
usr_conf_t * UsrCfg = (usr_conf_t*)(SETTINGS.UserData);

/*==============================================================================
 * Functions
 *============================================================================*/

/**
 * This is a timer set up in user_main() which is called every 100ms, forever
 * @param arg unused
 */
static void ICACHE_FLASH_ATTR timer100ms(void *arg)
{
	CSTick( 1 ); // Send a one to uart
}

/**
 * This callback is registered with espconn_regist_recvcb and is called whenever
 * a UDP packet is received
 *
 * @param arg pointer corresponding structure espconn. This pointer may be
 *            different in different callbacks, please don’t use this pointer
 *            directly to distinguish one from another in multiple connections,
 *            use remote_ip and remote_port in espconn instead.
 * @param pusrdata received data entry parameters
 * @param len      received data length
 */
static void ICACHE_FLASH_ATTR udpserver_recv(void *arg, char *pusrdata, unsigned short len)
{
	struct espconn *pespconn = (struct espconn *)arg;

	uart0_sendStr("X");
}

/**
 * UART RX handler, called by the uart task. Currently does nothing
 *
 * @param c The char received on the UART
 */
void ICACHE_FLASH_ATTR charrx( uint8_t c )
{
	//Called from UART.
}

/**
 * This is called on boot for versions ESP8266_NONOS_SDK_v1.5.2 to
 * ESP8266_NONOS_SDK_v2.2.1. system_phy_set_rfoption() may be called here
 */
void user_rf_pre_init(void)
{
	; // nothing
}


void ICACHE_FLASH_ATTR user_pre_init(void)
{
	LoadDefaultPartitionMap(); //You must load the partition table so the NONOS SDK can find stuff.
}


static struct espconn *pTcpServer;

#define MAX_CONNS MAX_PLAYERS

struct user_client_conn
{
	struct espconn *pespconn;
	int cansend:1;
	int id;
} connections[MAX_CONNS];

char my_server_name[16];



#define OUTCIRCBUFFSIZE 1024
#define SERVER_TIMEOUT 20
#define TXRXBUFFER 2048


uint8_t sendbuffer[TXRXBUFFER];
uint16_t sendptr;
uint8_t sendplayer;
uint8_t outcirc[OUTCIRCBUFFSIZE];
uint16_t outcirchead = 0;
uint8_t is_in_outcirc;

void StartupBroadcast() { is_in_outcirc = 1; }
void DoneBroadcast() { is_in_outcirc = 0; }
void PushByteK( uint8_t byte );

void extSbyte( uint8_t b )
{
	if( is_in_outcirc )
	{
		// printf( "Broadcast: %02x (%c)\n", b, b );
		outcirc[outcirchead & (OUTCIRCBUFFSIZE-1)] = b;
		outcirchead++;
	} else {
		PushByteK( b );
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
		// printf( "." );
		PushByteK( outcirc[(w++)&(OUTCIRCBUFFSIZE-1)] );
		i16--;
	}
	*whence = w;
	if( !i16 )
		return 1;
	else
		return 0;
}


void ICACHE_FLASH_ATTR SendData( uint8_t playerno, unsigned char * data, uint16_t packetsize )
{
	struct espconn *pespconn = connections[playerno].pespconn;
	if( pespconn != 0 )
	{
		espconn_sent( pespconn, data, packetsize );
		connections[playerno].cansend = 0;
	}
}


void SendStart( uint8_t playerno )
{
	sendplayer = playerno;
	sendptr = 0;
}
void PushByteK( uint8_t byte )
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
	return connections[playerno].cansend && connections[playerno].pespconn;
}


void ForcePlayerClose( uint8_t playerno, uint8_t reason )
{
	RemovePlayer( playerno );
//	close( playersockets[playerno] );

	//??? ??? ??? XXX TODO: I have no idea how to force a connection closed.

}
uint8_t readbuffer[TXRXBUFFER];
uint16_t readbufferpos = 0;
uint16_t readbuffersize = 0;

uint8_t Rbyte()
{
	uint8_t rb = readbuffer[readbufferpos++];
	// printf( "[%02x] ", rb );
	return rb;
}

uint8_t CanRead()
{
	return readbufferpos < readbuffersize;
}

SetServerName( const char * c )
{
	os_sprintf( my_server_name, "ESP8266Dumb" );
}

void ICACHE_FLASH_ATTR
at_tcpclient_recv(void *arg, char *pdata, unsigned short len)
{
	int i;
	struct espconn *pespconn = (struct espconn *)arg;
	struct user_client_conn *conn = (struct user_client_conn*)pespconn->reverse;

	memcpy( readbuffer, pdata, len );

	readbufferpos = 0;
	readbuffersize = len;

	
	GotData( conn->id );
	
	return;
}

static void ICACHE_FLASH_ATTR
at_tcpserver_recon_cb(void *arg, sint8 errType)
{
	struct espconn *pespconn = (struct espconn *)arg;
	struct user_client_conn *conn = (struct user_client_conn*)pespconn->reverse;
	uart0_sendStr( "Repeat Callback\r\n" );
}

static void ICACHE_FLASH_ATTR
at_tcpserver_discon_cb(void *arg)
{
	struct espconn *pespconn = (struct espconn *) arg;
	struct user_client_conn *conn = (struct user_client_conn*)pespconn->reverse;

	conn->pespconn = 0;
	uart0_sendStr("Disconnect.\r\n" );
}

static void ICACHE_FLASH_ATTR
at_tcpclient_sent_cb(void *arg)
{
	struct espconn *pespconn = (struct espconn *) arg;
	struct user_client_conn *conn = (struct user_client_conn*)pespconn->reverse;

	conn->cansend = 1;

//	uart0_sendStr("\r\nSEND OK\r\n");
}



LOCAL void ICACHE_FLASH_ATTR
at_tcpserver_listen(void *arg)
{
	struct espconn *pespconn = (struct espconn *)arg;
	uint8_t i;
	uart0_sendStr("get tcpClient:\r\n");
	for( i = 0; i < MAX_CONNS; i++ )
	{
		if( connections[i].pespconn == 0 )
		{
			break;
		}
	}
	if( i == MAX_CONNS )
	{
		return;
	}

	connections[i].pespconn = pespconn;
	connections[i].cansend = 1;
	connections[i].id = i;

	pespconn->reverse = (void*)&connections[i];
	espconn_regist_recvcb(pespconn, at_tcpclient_recv);
	espconn_regist_reconcb(pespconn, at_tcpserver_recon_cb);
	espconn_regist_disconcb(pespconn, at_tcpserver_discon_cb);
	espconn_regist_sentcb(pespconn, at_tcpclient_sent_cb);
	uart0_sendStr("Link\r\n");
	AddPlayer( i );
}

static void ICACHE_FLASH_ATTR
at_procTask(os_event_t *events)
{
	CSTick( 0 );

	// Post the task in order to have it called again
	system_os_post(procTaskPrio, 0, 0 );

	static int last_value = 0;
	static int updates_without_tick = 0;

	if( events->sig == 0 && events->par == 0 )
	{
		//Idle Event.
		//This function is called ~10000x/sec.
		
		uart0_sendStr( "." );
		
		if( connections[0].pespconn && connections[0].cansend )
		{
			UpdateServer();
			if( updates_without_tick++ > 500 ) 
			{
				TickServer();
			}
		    //espconn_sent( connections[0].pespconn, "hello\r\n", 7 );
		}
	}
}

/**
 * The default method, equivalent to main() in other environments. Handles all
 * initialization
 */
void ICACHE_FLASH_ATTR user_init(void)
{
	// Initialize the UART
	uart_init(BIT_RATE_115200, BIT_RATE_115200);

	os_printf("\r\nesp82XX Web-GUI\r\n%s\b", VERSSTR);

	//Uncomment this to force a system restore.
	//	system_restore();

	// Load settings and pre-initialize common services
	CSSettingsLoad( 0 );
	CSPreInit();

	// Start a UDP Server
    pUdpServer = (struct espconn *)os_zalloc(sizeof(struct espconn));
	ets_memset( pUdpServer, 0, sizeof( struct espconn ) );
	espconn_create( pUdpServer );
	pUdpServer->type = ESPCONN_UDP;
	pUdpServer->proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
	pUdpServer->proto.udp->local_port = COM_PORT;
	espconn_regist_recvcb(pUdpServer, udpserver_recv);

	if( espconn_create( pUdpServer ) )
	{
		while(1) { uart0_sendStr( "\r\nFAULT\r\n" ); }
	}

	// Initialize common settings
	CSInit( 1 );

	// Start MDNS services
	SetServiceName( "espcom" );
	AddMDNSName(    "esp82xx" );
	AddMDNSName(    "espcom" );
	AddMDNSService( "_http._tcp",    "An ESP82XX Webserver", WEB_PORT );
	AddMDNSService( "_espcom._udp",  "ESP82XX Comunication", COM_PORT );
	AddMDNSService( "_esp82xx._udp", "ESP82XX Backend",      BACKEND_PORT );

	// Set timer100ms to be called every 100ms
	os_timer_disarm(&some_timer);
	os_timer_setfn(&some_timer, (os_timer_func_t *)timer100ms, NULL);
	os_timer_arm(&some_timer, 100, 1);

	os_printf( "Boot Ok.\n" );

	// Set the wifi sleep type
	wifi_set_sleep_type(LIGHT_SLEEP_T);
	wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);


	pTcpServer = (struct espconn *)os_zalloc(sizeof(struct espconn));
	pTcpServer->type = ESPCONN_TCP;
	pTcpServer->state = ESPCONN_NONE;
	pTcpServer->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
	pTcpServer->proto.tcp->local_port = MINECRAFT_PORT;
	espconn_regist_connectcb(pTcpServer, at_tcpserver_listen);
	espconn_accept(pTcpServer);
	espconn_regist_time(pTcpServer, SERVER_TIMEOUT, 0);

	uart0_sendStr("Hello, world.  Starting server.\n" );
	InitDumbcraft();
	
	os_sprintf( my_server_name, "ESP8266Dumb" );


	// Add a process and start it
	system_os_task(at_procTask, procTaskPrio, procTaskQueue, procTaskQueueLen);
	system_os_post(procTaskPrio, 0, 0 );
}

/**
 * This will be called to disable any interrupts should the firmware enter a
 * section with critical timing. There is no code in this project that will
 * cause reboots if interrupts are disabled.
 */
void ICACHE_FLASH_ATTR EnterCritical(void)
{
	;
}

/**
 * This will be called to enable any interrupts after the firmware exits a
 * section with critical timing.
 */
void ICACHE_FLASH_ATTR ExitCritical(void)
{
	;
}
