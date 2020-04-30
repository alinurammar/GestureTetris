#include "interrupts.h"
#include "keyboard.h"
#include "printf.h"
#include "timer.h"
#include "uart.h"
#include "shell.h"
#include "strings.h"
#include "assert.h"


void test_clock_events(void)
{
    printf("Type on your PS/2 keyboard to generate clock events. You've got 10 seconds, go!\n");
    timer_delay(10);
    printf("Time's up!\n");
}


/*
 * This function tests the behavior of the assign5 keyboard
 * implementation versus the new-improved assign7 version. If using the
 * assign5 implementation, a key press that arrives while the main program
 * is waiting in delay is simply dropped. Once you upgrade your
 * keyboard implementation to be interrupt-driven, those keys should
 * be queued up and can be read after delay finishes.
 */
void test_read_delay(void)
{
    while (1) {
        printf("Test program waiting for you to press a key (q to quit): ");
        uart_flush();
        char ch = keyboard_read_next();
        printf("\nRead: %c\n", ch);
        if (ch == 'q') break;
        printf("Test program will now pause for 1 second... ");
        uart_flush();
        timer_delay(1);
        printf("done.\n");
    }
    printf("\nGoodbye!\n");
}

static void test_keyboard_assert(void)
{
    char ch;
    printf("\nHold down Shift and type 'g'\n");
    ch = keyboard_read_next();
    assert(ch == 'G');  // confirm user can follow directions and correct key char generated

    //my tests
    printf("\nHold down Shift and type '4'\n");
    ch = keyboard_read_next();
    assert(ch == '$');

    printf("\nHold down Shift and type ','\n");
    ch = keyboard_read_next();
    assert(ch == '<');

    printf("\nActivate capslock and type 'b'\n");
    ch = keyboard_read_next();
    assert(ch == 'B');

    printf("\nActivate capslock and type '9'\n");
    ch = keyboard_read_next();
    assert(ch == '9');

    printf("\ntype '-'\n");
    ch = keyboard_read_next();
    assert(ch == '-');

    printf("\ntype '='\n");
    ch = keyboard_read_next();
    assert(ch == '=');

    printf("\ntype shift and '='\n");
    ch = keyboard_read_next();
    assert(ch == '+');

    printf("\ntype shift and ' '\n");
    ch = keyboard_read_next();
    assert(ch == ' ');

    printf("\ntype shift and '`'\n");
    ch = keyboard_read_next();
    assert(ch == '~');

    printf("\ntype shift and '[]'\n");
    ch = keyboard_read_next();
    assert(ch == '{');
}

static void user_test_readline(void) {
    printf("Hello, you will be responsible for testing shell_readline\n");
    printf("Enter any input, and it should be echoed exactly, or else there is a bug\n");
    printf("Play around with backspace, as that could be a potential source for errors\n");
    printf("Make sure to hit the left and right bounds\n");
    shell_init(printf);
    for(int i = 0; i < 10; i++) {
      char line[51];
      memset(line, 0, 51);
      shell_readline(line, 50);
      printf("%s\n", line);
      printf("____________________________\n");
    }
    printf("Done\n");
}

void main(void)
{
    interrupts_init();
    gpio_init();
    timer_init();
    uart_init();
    keyboard_init(KEYBOARD_CLOCK, KEYBOARD_DATA);
    interrupts_global_enable();

    test_keyboard_assert();
    user_test_readline();
    // test_clock_events();  // wait 10 seconds for clock_edge handler to report clock edges
    test_read_delay();  // what happens to keys typed while main program blocked in delay?
    uart_putchar(EOT);
}
