#include "hal.h"

void SystemInit(void) {
  // enable FPU, ARM Cortex Guide 4.6.1
  SCB->CPACR |= (0x3UL << 20) | (0x3UL << 22);
  // enable SYSCFG
  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
}

extern uint32_t SystemCoreClock;

int main(void) {
  sysclock_status_t s = init_sysclock();

  if (s != SYSCLK_OK) {
    // todo some blinking with spin, not delay because the systick is not ready yet
  }
  uint32_t status = RCC->CFGR & RCC_CFGR_SWS;
  if ((status >> 2U) == 0x02) {
    SystemCoreClock = 100000000; // 100 MHz
  } else {
    SystemCoreClock = 16000000; // 16 MHz
  }

  SysTick_Config(SystemCoreClock / 1000); // Sys tick every 1ms

  uint16_t led = PIN('C', 13);

  // set the output
  gpio_set_mode(led, GPIO_MODE_OUTPUT);

  // set the timer
  bool state = true; // turn on LED at the start
  uint32_t timer = 0;
  uint32_t period = 1000; // 1 sec

  // set UART
  uart_init(UART1, 115200);
  char *txt = "I like bare-metal programming!!";

  printf("status %s \r\n", get_sysclock_status_string(s));

  for (;;) {
    if (timer_expired(&timer, period, g_ticks)) {
      gpio_write(led, state);
      state = !state;
      printf("%s, ticks: %lu\r\n", txt, (unsigned long)g_ticks);
      printf("status %s \r\n", get_sysclock_status_string(s));
    }
  }
}
