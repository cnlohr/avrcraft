avrcraft
========

Minecraft server optimized for 8-bit devices.

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

## LICENSE
All code in this repository that was authored by me is freely licensable under the MIT/x11 or NewBSD licenses.
