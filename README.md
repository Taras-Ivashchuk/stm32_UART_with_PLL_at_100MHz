# stm32_UART_with_PLL_at_100MHz  — STM32F411CEU6(Black Pill)

A simple program that allows user to send messages via UART on the Black Pill.

The program uses a PLL to clock the system at 100 MHz for accurate timing.
The program is built using bare-metal programming and CMSIS and is based on the nice practical guide
https://github.com/cpq/bare-metal-programming-guide.


## How it works 
- The software initializes a system clock at the start by using HSE(external crystal) and PLL to run at 100MHz.
- SysTick increments a millisecond counter. The main loop prints a message every second and blinks the LED.
- The UART is configured to run at 115200 baud rate.

## Structure
- The Makefile is used to compile and flash the project to the board. 
- The linker script is used to define memory sections and define sizes, start and end regions for memory sections.
- The system_calls.c is used to provide stubs for system calls.
- The hal.h and hal.c contain user API.
- The cmsis_core and cmsis_f4 directories contain CMSIS header files.
- The STM32F411.svd file contains data needed for debugging.
- The main.c contains the firmware code.

## Environment
The project is built on bare-metal using GNU Arm Toolchain, GNU Make and CMSIS.

## Hardware
- STM32F411CEU6(Black Pill)
- Logic analyzer(WeAct Studio)

## Prerequisites
- gcc-arm-none-eabi, make, stlink-tools, git
- For more details please refer to the https://github.com/cpq/bare-metal-programming-guide.

## Debugging
- The Visual Studio Code is used together with Cortex-Debug extension.
- In order to provide the debugger data about the board a file STM32F411.svd is included in the repo(ST, Apache-2.0).
   ```sh
   # your port may differ, e.g. /dev/ttyACM0
   picocom -b 115200 /dev/ttyUSB0
   ```

## Usage
Before compiling the Makefile downloads CMSIS headers and puts it in the working directory.

To compile the firmware use:
```sh
make build
```

To flash the firmware use:
```sh
make flash
```

To clean the project use(note that this deletes your cmsis directories):
```sh
make clean
```
