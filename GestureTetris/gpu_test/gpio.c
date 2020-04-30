#include "gpio.h"

static volatile unsigned int *FSEL0 =  (unsigned int *)0x20200000;
static volatile unsigned int *SET0  =  (unsigned int *)0x2020001c;
static volatile unsigned int *CLR0  =  (unsigned int *)0x20200028;
static volatile unsigned int *LEV0  =  (unsigned int *)0x20200034;

void gpio_init(void) {
}

void gpio_set_function(unsigned int pin, unsigned int function) {
  if(pin < GPIO_PIN_FIRST || pin > GPIO_PIN_LAST || function < 0 || function > 7) {
    return;
  }
  //create pointer to the GPIOFSEL register that controls this pin
  volatile unsigned int *registerToSet = FSEL0 + (pin / 10);
  //clear current function
  *registerToSet = *registerToSet & ~(7 << 3*(pin % 10));
  //set new function
  *registerToSet = *registerToSet | (function << 3*(pin % 10));
}

unsigned int gpio_get_function(unsigned int pin) {
  if(pin < GPIO_PIN_FIRST || pin > GPIO_PIN_LAST) {
    return GPIO_INVALID_REQUEST;
  }
  //create pointer to the GPIOFSEL register that controls this pin
  volatile unsigned int *funcRegister = FSEL0 + (pin / 10);
  //shift register so that relevant pin function is in the first three bits
  volatile unsigned int shiftedFunction = *funcRegister >> 3*(pin % 10);
  //return first three bits
  return shiftedFunction & 7;
}

void gpio_set_input(unsigned int pin) {
  gpio_set_function(pin, GPIO_FUNC_INPUT);
}

void gpio_set_output(unsigned int pin) {
  gpio_set_function(pin, GPIO_FUNC_OUTPUT);
}

void gpio_write(unsigned int pin, unsigned int value) {
  if(pin < GPIO_PIN_FIRST || pin > GPIO_PIN_LAST) {
    return;
  }
  //set pin to be on
  if(value == 1) {
    volatile unsigned int *setRegister = SET0 + (pin / 32);
    *setRegister = (1 << (pin % 32));
  }
  //set pin to be off
  else if (value == 0){
    volatile unsigned int *clearRegister = CLR0 + (pin / 32);
    *clearRegister = (1 << (pin % 32));
  }
}

unsigned int gpio_read(unsigned int pin) {
  if(pin < GPIO_PIN_FIRST || pin > GPIO_PIN_LAST) {
    return GPIO_INVALID_REQUEST;
  }
  //create a pointer referring to the level register for the pin
  volatile unsigned int *levelRegister = LEV0 + (pin / 32);
  //return the bit referring to the chosen pin
  return (*levelRegister >> (pin % 32)) & 1;
}
