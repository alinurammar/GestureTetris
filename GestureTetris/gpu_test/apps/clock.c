#include "../gpio.h"
#include "../timer.h"

void setupDisplay(void);
void displayDigit(int digit);

char bitPatterns[] = {0b01111110, 0b00110000, 0b01101101, 0b01111001,
                      0b00110011, 0b01011011, 0b01011111, 0b01110000,
                      0b01111111, 0b01110011, 0b01110111, 0b00011111,
                      0b01001110, 0b00111101, 0b10011111, 0b01000111};

void main(void)
{
    setupDisplay();
    gpio_set_function(GPIO_PIN2, GPIO_FUNC_INPUT);
    //draw horizontal bar before clock start
    gpio_write(GPIO_PIN26, 1);
    //wait till button press
    while(gpio_read(GPIO_PIN2) == 0) {}
    //run clock
    unsigned long totalTicks = 0;
    unsigned int lastTicks = timer_get_ticks();
    unsigned int elapsedSeconds;
    for(int digit = 0; /*forever*/ ; digit = (digit + 1) % 4) {
      gpio_write(10 + digit, 1);
      //increment time
      totalTicks += timer_get_ticks() - lastTicks;
      lastTicks = timer_get_ticks();
      elapsedSeconds = totalTicks / 1000000;
      //draw number depending on digit
      if(digit == 3) {
        displayDigit((elapsedSeconds % 60) % 10);
      }
      else if(digit == 2) {
        displayDigit((elapsedSeconds % 60) / 10);
      }
      else if(digit == 1) {
        displayDigit((elapsedSeconds / 60) % 10);
      } else {
        displayDigit((elapsedSeconds / 60) / 10);
      }

      timer_delay_us(2500);
      gpio_write(10 + digit, 0);
    }
}

void setupDisplay(void) {
  //intialize digit select pins
  for(int i = 10; i <= 13; i++) {
    gpio_set_output(i);
    gpio_write(i, 1);
  }
  //initialize segment led pins
  for(int i = 20; i <= 26; i++) {
    gpio_set_output(i);
  }
}

void displayDigit(int digit) {
  if(digit < 0 || digit > 15) {
    return;
  }
  char toDisplay = bitPatterns[digit];
  for(int i = 26; i >= 20; i--) {
    //if first bit is 1 turn on, else turn off
    if((toDisplay & 1) == 1) {
      gpio_write(i, 1);
    } else {
      gpio_write(i, 0);
    }
    toDisplay = toDisplay >> 1;
  }
}
