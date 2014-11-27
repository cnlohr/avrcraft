#
#             LUFA Library
#     Copyright (C) Dean Camera, 2014.
#
#  dean [at] fourwalledcubicle [dot] com
#           www.lufa-lib.org
#
# --------------------------------------
#         LUFA Project Makefile.
# --------------------------------------

# Run "make help" for target help.

LUFA_SRC_USB = 
MCU          = atmega32u2
ARCH         = AVR8
BOARD        = USBKEY
F_CPU        = 16000000
F_USB        = $(F_CPU)
OPTIMIZATION = s
TARGET       = RNDISEthernet
LUFA_SRC_USB = \
	LUFA/Drivers/USB/Core/AVR8/Device_AVR8.c \
	LUFA/Drivers/USB/Core/AVR8/EndpointStream_AVR8.c \
	LUFA/Drivers/USB/Core/AVR8/Endpoint_AVR8.c \
	LUFA/Drivers/USB/Core/AVR8/Host_AVR8.c \
	LUFA/Drivers/USB/Core/AVR8/PipeStream_AVR8.c \
	LUFA/Drivers/USB/Core/AVR8/Pipe_AVR8.c \
	LUFA/Drivers/USB/Core/AVR8/USBController_AVR8.c \
	LUFA/Drivers/USB/Core/AVR8/USBInterrupt_AVR8.c \
	LUFA/Drivers/USB/Core/ConfigDescriptors.c \
	LUFA/Drivers/USB/Core/DeviceStandardReq.c \
	LUFA/Drivers/USB/Core/Events.c \
	LUFA/Drivers/USB/Core/HostStandardReq.c \
	LUFA/Drivers/USB/Core/USBTask.c
LUFA_SRC_SERIAL          := LUFA/Drivers/Peripheral/$(ARCH)/Serial_$(ARCH).c

MYNETLIB = net_compat.c iparpetc.c
SRC          = $(TARGET).c Descriptors.c $(LUFA_SRC_USB) $(LUFA_SRC_SERIAL)  myRNDIS.c tcp_awful.c $(MYNETLIB) dumbgame.c ../dumbcraft/dumbcraft.c ../dumbcraft/dumbutils.c ../libs/avr_print.c ../libs/util10.c ws2812.c

LUFA_PATH    = LUFA
CC_FLAGS     = -DUSE_LUFA_CONFIG_HEADER -IConfig/
LD_FLAGS     = -Wl,--relax -Wa,-a,-ad -Wl,--gc-sections
PROGRAMCODE  = m32u2
# Default target
all: $(TARGET).hex burn

CFLAGS  := $(CC_FLAGS) -flto -fwhole-program -pipe -gdwarf-2 -g2 -mmcu=$(MCU) -I. -I../LUFA/.. -DARCH=ARCH_AVR8 -DBOARD=BOARD_USBKEY -DF_USB=16000000UL -DF_CPU=16000000UL -mrelax -Os -std=gnu99 -I../dumbcraft -I../libs #-Wall

CFLAGS := $(CFLAGS) -DMUTE_PRINTF

$(TARGET).elf : $(SRC)
	avr-gcc $(CFLAGS) $(LD_FLAGS) $(SRC) -o $@  > $(TARGET).lst

$(TARGET).hex : $(TARGET).elf
	avr-objcopy -O ihex -R .eeprom -R .fuse -R .lock -R .signature $^ $@ 

symbol-sizes :
	avr-nm --size-sort --demangle --radix=d $(TARGET).elf


size :
	avr-size --mcu=$(MCU) --format=avr $(TARGET).elf

burn : size $(TARGET).hex
	avrdude -V -c usbtiny -p $(PROGRAMCODE) -U flash:w:$(TARGET).hex

readfuses :
	avrdude -c usbtiny -p $(PROGRAMCODE) -U hfuse:r:high.txt:b -U lfuse:r:low.txt:b

burnfuses :
	avrdude -c usbtiny -p $(PROGRAMCODE) -U lfuse:w:0xDE:m -U hfuse:w:0xD9:m -U efuse:w:0xCC:m

clean :
	rm -rf $(TARGET).elf $(TARGET).hex $(TARGET).lst *.o *.map



