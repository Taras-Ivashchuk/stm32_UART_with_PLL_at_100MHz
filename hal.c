#include "hal.h"

uint32_t SystemCoreClock = 16000000;
volatile uint32_t g_ticks = 0;

void gpio_set_mode(uint16_t pin, uint8_t mode) {
	uint32_t pinno = PINNO(pin);
	uint32_t pinbank = PINBANK(pin);
	GPIO_TypeDef *gpio = GPIO(pinbank);
	RCC->AHB1ENR |= BIT(PINBANK(pin));				 // Enable GPIO clock
	gpio->MODER &= ~(3U << (pinno * 2));			 // clear the moder bits
	gpio->MODER |= (uint32_t) (mode << (pinno * 2)); // assign the mode
}

void gpio_write(uint16_t pin, bool val) {
	uint32_t pinbank = PINBANK(pin);
	GPIO_TypeDef *gpio = GPIO(pinbank);
	uint32_t pinno = PINNO(pin);
	gpio->BSRR |= (1U << pinno) << (val ? 0 : 16);
}

// Function is used to init sysclock based on CPU ticks
// Executes NOP operation count times
// spin(1) takes 4us, which is 80 CPU ticks when CPU FREQ is 16MHz
void spin(volatile uint32_t count) {
	while (count--)
		(void) 0;
}

void SysTick_Handler(void) {
	g_ticks++;
}

void delay(uint32_t ms) {
	uint32_t alarm = g_ticks + ms;
	while (g_ticks < alarm)
		(void) 0;
}

bool timer_expired(uint32_t *timer, uint32_t period, uint32_t ticks) {
	if (ticks + period < *timer)
		*timer = 0; // timer overflow
	if (*timer == 0)
		*timer = ticks + period; // set the timer for the first time
	if (ticks < *timer)
		return false; // not expired? return
	*timer = (ticks - *timer) < period ? *timer + period
									   : ticks + period; // Expired, return true
	return true;
}

void gpio_set_af(uint16_t pin, uint8_t af) {
	uint32_t pinno = PINNO(pin);
	uint32_t pinbank = PINBANK(pin);
	GPIO_TypeDef *gpio = GPIO(pinbank);

	gpio->AFR[pin >> 3] &= (uint32_t) ~(
			0x0F << (4 * (pinno & 7))); // clear the current af value (4 bits)
	gpio->AFR[pin >> 3] |=
			(uint32_t) (af << (4 * (pinno & 7))); // set the af (4 bits)
}

// Returns the clock prescaler for APB1, APB2 buses
int get_APB_presc(uint32_t n) {
	uint32_t s; // status
	if (n == 1U) {
		s = RCC->CFGR & RCC_CFGR_PPRE1;
		s = s >> 10;
	} else if (n == 2U) {
		s = RCC->CFGR & RCC_CFGR_PPRE2;
		s = s >> 13;
	} else {
		return -1;
	}
	switch (s) {
		case 0:
			return 0;
		case 4:
			return 2;
		case 5:
			return 4;
		case 6:
			return 8;
		case 7:
			return 16;
		default:
			return -1;
	}
}

void uart_init(USART_TypeDef *uart, uint32_t baud) {
	uint8_t af = 0;
	uint16_t tx = 0;
	uint16_t rx = 0;
	int bus_clock_prescaler = 0;
	if (uart == UART1) {
		af = 7;
		tx = PIN('A', 9);
		rx = PIN('A', 10);
		RCC->APB2ENR |= BIT(4);
		bus_clock_prescaler = get_APB_presc(1);
	} else if (uart == UART2) {
		af = 7;
		tx = PIN('A', 2);
		rx = PIN('A', 3);
		RCC->APB1ENR |= BIT(17);
		bus_clock_prescaler = get_APB_presc(2);
	} else if (uart == UART6) {
		af = 8;
		tx = PIN('A', 11);
		rx = PIN('A', 12);
		RCC->APB2ENR |= BIT(5);
		bus_clock_prescaler = get_APB_presc(1);
	} else {
		return;
	}

	if (bus_clock_prescaler == -1)
		return;

	gpio_set_mode(tx, GPIO_MODE_AF);
	gpio_set_af(tx, af);
	gpio_set_mode(rx, GPIO_MODE_AF);
	gpio_set_af(rx, af);

	uart->CR1 = 0; // disable UART at first
	bus_clock_prescaler = (bus_clock_prescaler == 0) ? 1 : bus_clock_prescaler;
	uart->BRR = (SystemCoreClock / (unsigned long) bus_clock_prescaler) / baud;
	uart->CR1 |= BIT(13) | BIT(3) | BIT(2); // UE, TX, RE
}

void uart_write_byte(USART_TypeDef *uart, uint8_t data) {
	uart->DR = data;
	while ((uart->SR & BIT(7)) == 0)
		spin(1);
}

int uart_read_ready(struct uart *uart) {
	return uart->SR & BIT(5);
}

uint8_t uart_read_byte(struct uart *uart) {
	return (uint8_t) uart->DR & (0xFF);
}

void uart_write_buf(USART_TypeDef *uart, const char *buf, size_t buf_size) {
	while (buf_size--)
		uart_write_byte(uart, *(buf++));
}

int wait_for_reg_status(volatile uint32_t *reg, uint32_t mask, uint32_t status,
		uint32_t attempts) {
	while ((*reg & mask) != status) {
		if (!attempts) {
			return -1;
		}
		--attempts;
		spin(CPU_TICKS);
	}
	return 0;
}

// enable instruction prefetch, flash latency, and instructions/data cache
// flash latency matching 100 Mhz Clock is 4 cycles(3 waites)
// 3.4.2: ART,  3.8.1: Flash interface registers
sysclock_status_t init_flash(void) {
	// “header LATENCY mask is narrower than RM0383; use own 4-bit mask”.
	uint32_t const latency_mask = 0xFU << FLASH_ACR_LATENCY_Pos;
	FLASH->ACR &= ~(
			FLASH_ACR_DCEN | FLASH_ACR_ICEN | FLASH_ACR_PRFTEN | latency_mask);
	FLASH->ACR |= FLASH_ACR_DCEN | FLASH_ACR_ICEN | FLASH_ACR_PRFTEN |
				  FLASH_ACR_LATENCY_3WS;

	// wait to ensure settings applied
	if (wait_for_reg_status(
				&(FLASH->ACR), latency_mask, FLASH_ACR_LATENCY_3WS, 20))
		return SYSCLK_FLASH_TIMEOUT;

	return SYSCLK_OK;
}

/* Initialize HSE to the exetrnal crystal */
sysclock_status_t init_HSE(void) {
	// use the external crystal
	RCC->CR &= ~RCC_CR_HSEBYP;

	// turn on the HSE osclillator
	RCC->CR |= RCC_CR_HSEON;

	// wait till HSE is ready
	if (wait_for_reg_status(&(RCC->CR), RCC_CR_HSERDY,
				(1U << RCC_CR_HSERDY_Pos), HSE_TIMEOUT_ATTEMPTS))
		return SYSCLK_HSE_TIMEOUT;

	return SYSCLK_OK;
}

/* Set up PLL as a source clock */
sysclock_status_t init_PLL(void) {
	// disable PLL before configuring
	RCC->CR &= ~RCC_CR_PLLON;

	// wait until PLL is off
	if (wait_for_reg_status(&(RCC->CR), RCC_CR_PLLRDY, 0, 20))
		return SYSCLK_PLL_TIMEOUT;

	// VCO input frequency = PLL input clock frequency / PLLM with 2 ≤ PLLM ≤ 63
	// VCOi = 25MHz / 25   = 1MHz
	RCC->PLLCFGR &= ~RCC_PLLCFGR_PLLM;
	RCC->PLLCFGR |=
			(RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLM_3 | RCC_PLLCFGR_PLLM_0);

	// VCO output frequency = VCO input frequency × PLLN with 50 ≤ PLLN ≤ 432
	// VCOo  = 1MHz * 400 = 400 MHz
	RCC->PLLCFGR &= ~RCC_PLLCFGR_PLLN;
	RCC->PLLCFGR |=
			(RCC_PLLCFGR_PLLN_8 | RCC_PLLCFGR_PLLN_7 | RCC_PLLCFGR_PLLN_4);

	// PLL output clock frequency = VCO frequency / PLLP with PLLP = 2, 4, 6, or
	// 8 PLLo = 400 MHz / 4 = 100 MHz
	RCC->PLLCFGR &= ~RCC_PLLCFGR_PLLP;
	RCC->PLLCFGR |= RCC_PLLCFGR_PLLP_0;

	// source the PLL from the HSE oscillator
	RCC->PLLCFGR |= RCC_PLLCFGR_PLLSRC;
	// enable PLL
	RCC->CR |= RCC_CR_PLLON;

	// wait till PLL is unlocked
	if (wait_for_reg_status(&(RCC->CR), RCC_CR_PLLRDY,
				(1U << RCC_CR_PLLRDY_Pos), PLL_TIMEOUT_ATTEMPTS))
		return SYSCLK_PLL_TIMEOUT;

	return SYSCLK_OK;
}

/* Fit speed for APB1, APB2 and AHB buses to requiered limits */
void init_buses(void) {
	// per datasheet Table 14, not RM0383
	// APB1 must not exceed 84MHz
	// APB2 must not exceed 100 MHZ
	// For APB1-2 the PLLo / prescaler <= 84,
	// prescaler = 2, so 100 / 2 = 50,
	RCC->CFGR &= ~RCC_CFGR_PPRE1;
	RCC->CFGR &= ~RCC_CFGR_PPRE2;
	RCC->CFGR |= (RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PPRE2_DIV2);
}

/* Use PLL as a system clock */
sysclock_status_t link_PLL_to_sysclock(void) {
	// PLL used as a system clock
	RCC->CFGR &= ~RCC_CFGR_SW;
	RCC->CFGR |= RCC_CFGR_SW_PLL;

	// wait until the PLL is used as a system clock
	if (wait_for_reg_status(&(RCC->CFGR), RCC_CFGR_SWS, RCC_CFGR_SWS_PLL, 20))
		return SYSCLK_SYSCLOCK_TIMEOUT;

	return SYSCLK_OK;
}

/* Regulates voltage scaling output to achieve a trade-off between performance
   and power consumption
   Can be modified only when PLL is off
*/
sysclock_status_t init_VOS(void) {
	// Power the APB1 before PWR_CR so changes not be ignored
	RCC->APB1ENR |= RCC_APB1ENR_PWREN;

	// Scale 1 mode <= 100 MHz, VOS=0b11
	PWR->CR |= (PWR_CR_VOS_0 | PWR_CR_VOS_1);

	return SYSCLK_OK;
}

/* Initialize system clock to be used with PLL */
sysclock_status_t init_sysclock(void) {
	sysclock_status_t s;

	if ((s = init_HSE()) != SYSCLK_OK)
		return s;
	if ((s = init_VOS()) != SYSCLK_OK)
		return s;
	if ((s = init_PLL()) != SYSCLK_OK)
		return s;
	// init_buses cannot fail
	init_buses();
	if ((s = init_flash()) != SYSCLK_OK)
		return s;
	if ((s = link_PLL_to_sysclock()) != SYSCLK_OK)
		return s;

	return SYSCLK_OK;
}

const char *get_sysclock_status_string(sysclock_status_t s) {
	switch (s) {
		case SYSCLK_OK:
			return "SYSCLOCK OK";
		case SYSCLK_HSE_TIMEOUT:
			return "SYSCLOCK HSE TIMEOUT";
		case SYSCLK_PLL_TIMEOUT:
			return "SYSCLOCK PLL TIMEOUT";
		case SYSCLK_FLASH_TIMEOUT:
			return "SYSCLOCK FLASH TIMEOUT";
		case SYSCLK_SYSCLOCK_TIMEOUT:
			return "SYSCLOCK TIMEOUT";
		case SYSCLK_VOS_TIMEOUT:
			return "SYSCLOCK VOS TIMEOUT";
		default:
			return "UNKNOWN ERROR";
	}
}
