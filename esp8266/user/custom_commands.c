//Copyright 2015 <>< Charles Lohr, see LICENSE file.

#include <commonservices.h>
#include "esp82xxutil.h"

int ICACHE_FLASH_ATTR CustomCommand(char * buffer, int retsize, char *pusrdata, unsigned short len) {
	char * buffend = buffer;

	switch( pusrdata[1] ) {
		// Custom command test
		case 'C': case 'c':
			buffend += ets_sprintf( buffend, "CC" );
        	os_printf("CC");
			return buffend-buffer;
		break;

		// Echo to UART
		case 'E': case 'e':
			if( retsize <= len ) return -1;
			ets_memcpy( buffend, &(pusrdata[2]), len-2 );
			buffend[len-2] = '\0';
			os_printf( "%s\n", buffend );
			return len-2;
		break;
	}

	return -1;
}
