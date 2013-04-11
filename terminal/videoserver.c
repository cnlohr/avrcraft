#include "videoserver.h"
#include <stdio.h>

extern volatile char framebuffer[NTWIDTH*NTLINES];
unsigned char vsX;
unsigned char vsY;

static int VideoPutChar(char c, FILE *stream)
{
	
	return 0;
}

static FILE mystdout = FDEV_SETUP_STREAM( VideoPutChar, NULL, _FDEV_SETUP_WRITE );

void SetupVideoServer()
{
	stdout = &mystdout;
}
