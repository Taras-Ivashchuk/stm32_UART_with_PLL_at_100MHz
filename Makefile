CFLAGS  ?=  -W -Wall -Wextra -Werror -Wundef -Wshadow -Wdouble-promotion \
            -Wformat-truncation -fno-common -Wconversion \
            -g3 -O0 -ffunction-sections -fdata-sections -I. \
			-Icmsis_core/CMSIS/Core/Include -Icmsis_f4/Include \
            -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 $(EXTRA_CFLAGS)
LDFLAGS ?= -Tlink.ld -nostartfiles -nostdlib --specs=nano.specs -lc -lgcc -Wl,--gc-sections -Wl,-Map=$@.map

SOURCES = $(wildcard *.c)
SOURCES += cmsis_f4/Source/Templates/gcc/startup_stm32f411xe.s

ifeq ($(OS),Windows_NT)
  RM = cmd /C del /Q /F
else
  RM = rm -rf
endif

build: firmware.bin

firmware.bin: firmware.elf 
	arm-none-eabi-objcopy -O binary $< $@

firmware.elf: cmsis_core cmsis_f4 hal.h link.ld Makefile $(SOURCES)
	arm-none-eabi-gcc $(SOURCES) $(CFLAGS) $(LDFLAGS) -o $@

flash: firmware.bin
	st-flash --reset write $< 0x8000000

clean:
	$(RM) firmware.* cmsis_* *.o

cmsis_core: 
	git clone --depth 1 -b 5.9.0 https://github.com/ARM-software/CMSIS_5 $@ 

cmsis_f4: 
	git clone --depth 1 -b v2.6.11 https://github.com/STMicroelectronics/cmsis_device_f4 $@ 
