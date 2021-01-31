#  Project Name
PROJECT=main

# libs dir
LIBDIR=../../..

# STM32 stdperiph lib defines
CDEFS=-DHSE_VALUE=25000000 -DSTM32F10X_CL -DUSE_STDPERIPH_DRIVER -DARP_PRIVATE_ONLY

#  List of the objects files to be compiled/assembled
OBJECTS=main.o

CMSIS_SOURCES=\
$(LIBDIR)/cmsis/startup_stm32f10x_cl.o \
$(LIBDIR)/cmsis/system_stm32f10x.o

STM_SOURCES=\
$(LIBDIR)/stm32f10x/src/stm32f10x_gpio.o \
$(LIBDIR)/stm32f10x/src/stm32f10x_rcc.o \
$(LIBDIR)/stm32f10x/src/stm32f10x_exti.o \
$(LIBDIR)/stm32f10x/src/misc.o \
$(LIBDIR)/stm32f10x/src/stm32f10x_usart.o \
$(LIBDIR)/stm32f10x/src/stm32f10x_iwdg.o

MAT_SOURCES=\
$(LIBDIR)/mat/circbuf8.o \
$(LIBDIR)/mat/itoa.o \
$(LIBDIR)/mat/serialq.o \
$(LIBDIR)/mat/fifo.o \
$(LIBDIR)/mat/misc.o \
$(LIBDIR)/mat/eth.o \
$(LIBDIR)/mat/net/arp.o \
$(LIBDIR)/mat/net/ipv4.o \
$(LIBDIR)/mat/net/icmp.o \
$(LIBDIR)/mat/net/udp.o \
$(LIBDIR)/mat/net/tcp.o

OBJECTS+=$(CMSIS_SOURCES)
OBJECTS+=$(STM_SOURCES)
OBJECTS+=$(MAT_SOURCES)

LSCRIPT=stm32f107vc.ld

OPTIMIZATION = s
DEBUG = dwarf-2
#LISTING = -Wa,-adhlns=$(<:%.c=%.lst)

#  Compiler Options
GCFLAGS = -g$(DEBUG)
GCFLAGS += $(CDEFS)
GCFLAGS += -O$(OPTIMIZATION)
GCFLAGS += -Wall -std=gnu99 -fno-common -mcpu=cortex-m3 -mthumb -ffunction-sections
GCFLAGS += -I$(LIBDIR)/stm32f10x/inc -I$(LIBDIR)/cmsis -I$(LIBDIR)
#GCFLAGS += -Wcast-align -Wcast-qual -Wimplicit -Wpointer-arith -Wswitch
#GCFLAGS += -Wredundant-decls -Wreturn-type -Wshadow -Wunused
LDFLAGS = -mcpu=cortex-m3 -mthumb -O$(OPTIMIZATION) -Wl,-Map=$(PROJECT).map -T$(LSCRIPT) -Wl,--gc-sections
ASFLAGS = $(LISTING) -mcpu=cortex-m3

#  Compiler/Assembler/Linker Paths
GCC = arm-none-eabi-gcc
AS = arm-none-eabi-as
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy
ifeq ($(OS), Windows_NT)
REMOVE = rm.py -f
else
REMOVE = rm -f
endif
SIZE = arm-none-eabi-size

#########################################################################

all: $(PROJECT).hex $(PROJECT).bin stats

$(PROJECT).bin: $(PROJECT).elf
#	$(OBJCOPY) -O binary -j .text -j .data $(PROJECT).elf $(PROJECT).bin
	$(OBJCOPY) -R .stack -O binary $(PROJECT).elf $(PROJECT).bin

$(PROJECT).hex: $(PROJECT).elf
	$(OBJCOPY) -R .stack -O ihex $(PROJECT).elf $(PROJECT).hex

$(PROJECT).elf: $(OBJECTS)
	$(GCC) $(LDFLAGS) $(OBJECTS) -o $(PROJECT).elf

stats: $(PROJECT).elf
	$(SIZE) $(PROJECT).elf

clean:
	$(REMOVE) $(OBJECTS)
	$(REMOVE) $(PROJECT).hex
	$(REMOVE) $(PROJECT).elf
	$(REMOVE) $(PROJECT).map
	$(REMOVE) $(PROJECT).bin

program:
	st-flash write main.bin 0x08000000

#########################################################################
#  Default rules to compile .c and .cpp file to .o
#  and assemble .s files to .o

.c.o :
	$(GCC) $(GCFLAGS) -c $< -o $@

.cpp.o :
	$(GCC) $(GCFLAGS) -c $< -o $@

.s.o :
	$(AS) $(ASFLAGS) -o $@ $<
#	$(AS) $(ASFLAGS) -o $(PROJECT)_crt.o $< > $(PROJECT)_crt.lst

#########################################################################
-include $(shell mkdir .dep) $(wildcard .dep/*)
