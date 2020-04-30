#include "assert.h"
#include "keyboard.h"
#include "printf.h"
#include "uart.h"
#include "timer.h"
#include "game.h"


void main(void)
{
    uart_init();

    printf("Testing board module.\n");
    printf("All done!\n");
    uart_putchar(EOT);
}
