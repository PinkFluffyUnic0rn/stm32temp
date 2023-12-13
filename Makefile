CC=arm-none-eabi-gcc

SOURCES=$(wildcard ./*.c) $(wildcard ./Drivers/Src/*.c)
OBJECTS=$(SOURCES:.c=.o)
LDFLAGS=-mcpu=cortex-m4 -T./linker-script.ld --specs=nosys.specs \
	-static --specs=nano.specs -mfpu=fpv4-sp-d16 \
	-mfloat-abi=hard -mthumb -u _printf_float -lc -lm
CFLAGS=-mcpu=cortex-m4 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32F303xC -c \
       -I. -I./Drivers/Inc/Legacy -I./Drivers/Inc \
       -I./Drivers/CMSIS/Device/ST/STM32F3xx/Include \
       -I./Drivers/CMSIS/Include -Os -Wall --specs=nano.specs \
       -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -lm
SFLAGS=-mcpu=cortex-m4 -c -x assembler-with-cpp --specs=nano.specs \
       -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb

all: load

load: prog.elf
	openocd -f interface/stlink.cfg -f target/stm32f3x.cfg \
		-c "program prog.elf verify reset exit"

prog.elf: $(OBJECTS) ./startup.o
	$(CC) $(LDFLAGS) $(OBJECTS) ./startup.o -o $@

.s.o:
	$(CC) $(SFLAGS) $< -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm $(OBJECTS) ./startup.o
