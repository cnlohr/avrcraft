#ifndef _VIDEO_H
#define _VIDEO_H

#define NTWIDTH 48
#define NTLINES 25

extern volatile char framebuffer[NTWIDTH*NTLINES];
extern unsigned char videobeep;

#endif

