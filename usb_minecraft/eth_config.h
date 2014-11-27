#ifndef _ETH_CONFIG_H
#define _ETH_CONFIG_H

#include <avr/io.h>

//#define ETHERNET_FRAME_SIZE_MAX 584
#define ETHERNET_FRAME_SIZE_MAX 256
#define ATTR_PACKED __attribute__ ((packed))

#define ETBUFFERSIZE (ETHERNET_FRAME_SIZE_MAX)
#define MAX_FRAMELEN (ETHERNET_FRAME_SIZE_MAX)
#define TX_SCRATCHES 1
#define RX_BUFFER_START 0
#define RX_BUFFER_END   MAX_FRAMELEN

//#define INCLUDE_UDP
#define NO_HTTP
#define HTTP_CONNECTIONS 0

//If we don't have a RX buffer, we can't do copying tricks.
#define NOCOPY_MAC

#define INCLUDE_TCP
#define AWFUL_TCP

#define TCP_SOCKETS 2

#define TCP_TICKS_BEFORE_DEAD 10

#define CIRC_BUFFER_SIZE 54

extern unsigned char MyMAC[];

#endif

