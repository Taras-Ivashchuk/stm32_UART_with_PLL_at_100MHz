#ifndef HAL_H
#define HAL_H

#include "stm32f411xe.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>

extern volatile uint32_t g_ticks;
extern uint32_t SystemCoreClock;

// enum values as per datasheet for MODER register
enum mode { GPIO_MODE_INPUT, GPIO_MODE_OUTPUT, GPIO_MODE_AF, GPIO_MODE_ANALOG };

// reset and clock control register
struct rcc {
  volatile uint32_t CR, PLLCFGR, CFGR, CIR, AHB1RSTR, AHB2RSTR, RESERVED0[2],
      APB1RSTR, APB2RSTR, RESERVED1[2], AHB1ENR, AHB2ENR, RESERVED2[2], APB1ENR,
      APB2ENR, RESERVED3[2], AHB1LPENR, AHB2LPENR, RESERVED4[2], APB1LPENR,
      APB2LPENR, RESERVED5[2], BDCR, CSR, RESERVED6[2], SSCGR, PLLI2SCFGR;
};

// SYSTICK
struct systick {
  volatile uint32_t CTRL, LOAD, VAL, CALIB;
};

struct uart {
  volatile uint32_t SR, DR, BRR, CR1, CR2, CR3, GTPR;
};

typedef enum {
  SYSCLK_OK,
  SYSCLK_HSE_TIMEOUT,
  SYSCLK_PLL_TIMEOUT,
  SYSCLK_FLASH_TIMEOUT,
  SYSCLK_SYSCLOCK_TIMEOUT,
  SYSCLK_VOS_TIMEOUT
} sysclock_status_t;

// GPIOA is at address 0x40020000, GPIOB is at 0x40020400, and so on
#define GPIO(bank) ((GPIO_TypeDef *)(0x40020000UL + 0x400UL * (bank)))

// We use 2-byte uint16_t value, where upper byte indicates GPIO bank, and lower
// byte indicates pin number:
// uint16_t pin2 = PIN('G', 11);   // G11  - GPIOG pin 11
#define PIN(bank, pinnum) (((bank) - 'A') << 8 | (pinnum))
#define PINBANK(pin) ((pin) >> 8U)
#define PINNO(pin) ((pin) & 255U)
#define BIT(bit) (1UL << (bit))
#define UART1 USART1
#define UART2 USART2
#define UART6 USART6
#define CPU_TICKS 50 // CPU ticks

// Example:
// uint16_t pin = PIN('A', 3);            // Pin A3
// gpio_set_mode(pin, GPIO_MODE_OUTPUT);  // Set to output
void gpio_set_mode(uint16_t pin, uint8_t mode);
void gpio_write(uint16_t pin, bool val);
void delay(uint32_t ms);
bool timer_expired(uint32_t *timer, uint32_t period, uint32_t ticks);
void uart_init(USART_TypeDef *uart, uint32_t baud);
void uart_write_buf(USART_TypeDef *uart, const char *buf, size_t buf_size);
sysclock_status_t init_sysclock(void);
const char *get_sysclock_status_string(sysclock_status_t s);
// Datasheet 6.3.10 Table 41. Main PLL characteristics at 16 MHz HSI
// 300us MAX PLL lock time / 25us per attempts = 12 
// 8 * 25us would give us the 200 us extra margin
// so 20 attempts give us 300us + 200 us
#define PLL_TIMEOUT_ATTEMPTS  20 

// Datasheet 6.3.8 Table 37. HSE 4-26 MHz oscillator characteristics
// Typical startup time is 2ms
// 2ms / 25us = 80, and 3x extra margin would give us 240 
#define HSE_TIMEOUT_ATTEMPTS 240

#endif
