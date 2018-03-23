# HLAVNI MAKEFILE PRO SYSTEM FreeRTOS

# zdrojove soubory aplikace
SRCS = main.c stm32f4xx_it.c system_stm32f4xx.c main_tasks.c
SRCS += ../lib/src/stm32f4_discovery.c


# FreeRTOS - zdrojove soubory jadra OS (nektere nejsou vyuzity)
SRCS += ../FreeRTOS/Source/croutine.c
SRCS += ../FreeRTOS/Source/event_groups.c
SRCS += ../FreeRTOS/Source/list.c
SRCS += ../FreeRTOS/Source/queue.c
SRCS += ../FreeRTOS/Source/tasks.c
SRCS += ../FreeRTOS/Source/timers.c


# FreeRTOS - zdrojove soubory spravy pameti a pro konkretni mikrokontroler
SRCS += ../FreeRTOS/Source/portable/MemMang/heap_1.c
SRCS += ../FreeRTOS/Source/portable/GCC/ARM_CM4F/port.c


# nazev projektu
PROJ_NAME = psd_freertos

###################################################

CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy
SIZE=arm-none-eabi-size

CFLAGS  = -g -O2 -Wall -Tstm32_flash.ld 
CFLAGS += -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork
CFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16

###################################################

vpath %.c src
vpath %.a lib

ROOT=$(shell pwd)

CFLAGS += -Iinc -Ilib -Ilib/inc 
CFLAGS += -Ilib/inc/core -Ilib/inc/peripherals
CFLAGS += -IFreeRTOS/Source -IFreeRTOS/Source/include -IFreeRTOS/Source/portable/GCC/ARM_CM4F 

SRCS += lib/startup_stm32f4xx.s # add startup file to build

OBJS = $(SRCS:.c=.o)

###################################################

.PHONY: lib proj

all: lib proj

lib:
	$(MAKE) -C lib

proj: 	$(PROJ_NAME).elf

$(PROJ_NAME).elf: $(SRCS)
# pridana matematicka knihovna (-lm) - bez teto upravy hlasi linker chybu
	$(CC) $(CFLAGS) $^ -o $@ -Llib -lstm32f4 -lm
	$(OBJCOPY) -O ihex $(PROJ_NAME).elf $(PROJ_NAME).hex
	$(OBJCOPY) -O binary $(PROJ_NAME).elf $(PROJ_NAME).bin
	@echo -----------------------------------------------------
	@echo HEX soubor vytvoren
	@echo -----------------------------------------------------
	@echo Velikost programu:
	$(SIZE) $(PROJ_NAME).hex

clean:
	rm -f *.o
	rm -f $(PROJ_NAME).elf
	rm -f $(PROJ_NAME).hex
	rm -f $(PROJ_NAME).bin
