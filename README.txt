My 8-bit Microscope Slide Minecraft Server

All contents is Copyright 2012 <>< Charles Lohr
All code and documentation is licensed under the MIT-x11 or NewBSD license.
You choose.  See license info at the end of this readme.
You can use this minecraft server and associated TCP/IP stack commercially.

Check out the AVR version of this in this video:
http://www.youtube.com/watch?v=EZRLOanNQ_w

This has a few subprojects.
* dumbcraft => Minimalistic Core C Minecraft Server
+ demo_x86_dumbcraft => dumbcraft wrapper for Linux desktops
+ demo_mineandhttp => dumbcraft wrapper for atmega328
* http => http server and demo
* avr_hardware => board layout design for AVR server
* ipcore => core AVR IP stack for the enc424j600
* mfs => minimalistic read-only in-flash filesystem for AVRs
* microsd => minimalistic FAT32 read-only SD/SDHC filesystem for AVRs
* ntsc => Code to help produce text-mode NTSC.
+indicates available tests.

----Overview----

This is my 8-bit AVR Minecraft Server... and some extra goodies.  This package
includes:

  * 754lib - A toolset for IEEE754-1985 doubles and floats to fixed point
  * os_generic - My toolset threads (unused), mutices (unused), and timing
  * util10 - My integer->string toolset for this project.
  * dumbcraft (.c, .h)
  * an x86 demo for dumbcraft (compiles and runs on Linux)
  * an AVR-buildable copy (see below)
  * a tool for generating gzipped map chunks (requires Linux)

The AVR folder contains:

  * avr_print - SPI debugging toolset (for use with tinyisp and tinyispterm)
  * my bitbanged ethernet (MAC + ARP + ICMP + UDP + TCP) driver (see below)
  * a really dumb HTTP server
  * a driver for SD cards (and a FAT32 driver) NO SUPPORT FOR FAT16!

Oh yeah, with debugging on, this all fits in about 17kB.

Depending on your settings (dumbcraft circular buffer, etc.) you can use
from about 600 bytes of static RAM to about 1.5 comfortably.

Sadly this does not include the stack, so if you can push data to the
clients and forget about it, that's probably for the best.


----What the dumbcraft does (and doesn't do)----

Dumbcraft is split into a C file and a static header file.  This architecture
can (and likely will) change soon, but I wanted to get this tool out the door!

The .C file contains the core dumbcraft toolset.  Because it doesn't support
any mode of communications with the outside world, all communications must be
done through various functions.  These functions interface 

What works:
  * The stock Minecraft 1.4.2 Server on Windows or Linux (most of the time)
  * More than one player
  * Running around
  * Pulling Levers
  * Updating blocks
  * Loading main map chunk
  * Chat

What doesn't work:
  * Day/night
  * MOBs
  * Physics
  * Animations
  * Crafting
  * Falling Blocks
  * Mining
  * Placing Blocks
  * Collecting things
  * The Nether
  * The End
  * Redstone
  * Water
  * Lava
  * Well, just about everything.

Take a look through dumbcraft.c to get a better hint as to how this all goes together.

----Mapmake----

Map make creates a chunk to be sent by 0x33.  This code doesn't seem to work
very well... It seems to work well enough to make big plot of green ground.

Try dinking with this to figure out why some other chunk types don't work?

It's used to generate the 'mapdata' buffer used in dumbcraft.c


----My IP Stack----

I wrote my own TCP/IP stack for this project, I've been working with it for
quite some time and I needed a good test.  Minecraft has shown to be an
excellent test.  It uses an ENC424j600 from Microchip in SPI mode. It doesn't
use the SPI hardware on an AVR, instead it bit bangs all communications with
it.  This is to make it possible to put it on any geographically convenient
pins.  (I do this on one-layer)

The stack contains the main MAC code and a file for the low level SPI
functionality.  Since the SPI has to be really fast, I've opted to write it
in assembly.

If you want to use the TCP/IP stack, you can feel free to copy it all out,
including test.c and rip out my code.  You'll need the basic callbacks from
TCP or UDP, but other than that you're good to go.

Reconfigure eth_config.h to your situation and profit.  You can get a UDP
server in a little under 4kB, and TCP for an extra ~2.5k more.

I included an HTTP server with it, but it barely counts as an HTTP server
since it requires additional code to serve ... anything.  You can see that it
is difficult to get it to server webpages from the SD card :-/.


----My FAT32/SD SPI Stack----

This stack is still pretty buggy but lets you read files from a FAT32
partition on a SD card.  It can either bin the root or in the FIRST partition.

It can list files in a folder, seek to folders, open files and read them 
sequentially... that's... about... it. 

The SD driver supports SDHC and SD cards and supports reading/writing at the
block level.  Keep in mind the FAT32 driver does not support writing.

Consider the test.c in the avr folder an example of how to do this.

----Building----

You'll need at the minimum: avrdude, avr-gcc and basic build-utils. I
recommend you get my copy of tinyispterm since that'll help you debug.

It prints debugging data through the SPI port if -DMUTE_PRINTF isn't set.
Everything's geared around a TinyISP from LadyAda... I personally use a tiny
variant of it I designed.  You can get the single-layer designs and the tool
here:

https://github.com/cnlohr/tinyispterm

The hardware details for this project can be found in the avr_hardware folder.

I am not a trained electrical engineer.  You will have to know your stuff
before venturing on this project!  It is tricky, and I didn't really document
anything very well.

I designed the PCB in ExpressPCB and designed the schematic later.  They don't
match and I got frustrated, so trust the PCB over the SCH.

Yes, it's a single layer.  No, there are no jumpers.


----Resources and Thanks----

Special thanks to the folks at #mcdevs.  They made this awesome thing here:
http://www.wiki.vg/Protocol

I used a few guides when working on the SD card code.
http://www.esawdust.com/blog/serial/files/SPI_SD_MMC_ATMega128_AVR.html
http://elm-chan.org/docs/mmc/mmc_e.html 

For the FAT32 driver, I used this:
http://staff.washington.edu/dittrich/misc/fatgen103.pdf


----The MIT/x11 License----

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

----The NewBSD License----

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

    Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.


