#include "assert.h"
#include "keyboard.h"
#include "printf.h"
#include "uart.h"
#include "timer.h"
#include "malloc.h"
#include "shell.h"
#include "strings.h"
#include "backtrace.h"

#define ESC_SCANCODE 0x76

char* output;

static void test_keyboard_scancodes(void)
{
    printf("\nNow reading single scancodes. Type ESC to finish this test.\n");
    while (1) {
        unsigned char scancode = keyboard_read_scancode();
        printf("[%02x]", scancode);
        if (scancode == ESC_SCANCODE) break;
    }
    printf("\nDone with scancode test.\n");
}

static void test_keyboard_sequences(void)
{
    printf("\nNow reading scancode sequences (key actions). Type ESC to finish this test.\n");
    while (1) {
        key_action_t action = keyboard_read_sequence();
        printf("%s [%02x]\n", action.what == KEY_PRESS ? "  Press" :"Release", action.keycode);
        if (action.keycode == ESC_SCANCODE) break;
    }
    printf("Done with scancode sequences test.\n");
}

static void test_keyboard_events(void)
{
    printf("\nNow reading key events. Type ESC to finish this test.\n");
    while (1) {
        key_event_t evt = keyboard_read_event();
        printf("%s PS2_key: {%c,%c} Modifiers: 0x%x\n", evt.action.what == KEY_PRESS? "  Press" : "Release", evt.key.ch, evt.key.other_ch, evt.modifiers);
        if (evt.action.keycode == ESC_SCANCODE) break;
    }
    printf("Done with key events test.\n");
}

static void test_keyboard_chars(void)
{
    printf("\nNow reading chars. Type ESC to finish this test.\n");
    while (1) {
        char c = keyboard_read_next();
        if (c >= '\t' && c <= 0x80)
            printf("%c", c);
        else
            printf("[%02x]", c);
        if (c == ps2_keys[ESC_SCANCODE].ch) break;
    }
    printf("\nDone with key chars test.\n");
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

int returnString(const char *format, ...) {
  va_list args;
  va_start(args, format);
  char buf[200];
  int ret = vsnprintf(buf, 200, format, args);
  strlcat(output, buf, 1024);
  return ret;
}

static void test_shell_eval(void) {
  output = malloc(1024);
  memset(output, 0, 1024);
  shell_init(returnString);
  //test echo and tokenizer
  shell_evaluate("echo Hello     World");
  assert(strcmp(output, "Hello World \n") == 0);
  memset(output, 0, 1024);
  shell_evaluate("echo Hello    World One      Two Three");
  assert(strcmp(output, "Hello World One Two Three \n") == 0);
  memset(output, 0, 1024);
  shell_evaluate("echo Simple Special~~!@##$|");
  assert(strcmp(output, "Simple Special~~!@##$| \n") == 0);
  memset(output, 0, 1024);
  //help
  shell_evaluate("help help");
  assert(strcmp(output, "help: <cmd> prints a list of commands or description of cmd\n") == 0);
  memset(output, 0, 1024);
  shell_evaluate("help please");
  assert(strcmp(output, "error: no such command `please`.\n") == 0);
  memset(output, 0, 1024);
  //peek input errors
  shell_evaluate("peek ");
  assert(strcmp(output, "error: peek expects 1 argument [address]\n") == 0);
  memset(output, 0, 1024);
  shell_evaluate("peek bob");
  assert(strcmp(output, "error: peek cannot convert 'bob'\n") == 0);
  memset(output, 0, 1024);
  shell_evaluate("peek aaa");
  assert(strcmp(output, "error: peek cannot convert 'aaa'\n") == 0);
  memset(output, 0, 1024);
  shell_evaluate("peek 0x8503");
  assert(strcmp(output, "error: peek address must be 4-byte aligned\n") == 0);
  memset(output, 0, 1024);
  shell_evaluate("peek 0x8003");
  assert(strcmp(output, "error: peek address must be 4-byte aligned\n") == 0);
  memset(output, 0, 1024);
  //peek consistency
  char temp[100];
  memset(temp, 0, 100);

  shell_evaluate("peek 0x8008");
  strlcat(temp, output, 100);
  memset(output, 0, 1024);
  shell_evaluate("peek 0x8008");
  assert(strcmp(output, temp) == 0);
  memset(temp, 0, 100);
  memset(output, 0, 1024);

  shell_evaluate("peek 0x8018");
  strlcat(temp, output, 100);
  memset(output, 0, 1024);
  shell_evaluate("peek 0x8018");
  assert(strcmp(output, temp) == 0);
  memset(temp, 0, 100);
  memset(output, 0, 1024);

  shell_evaluate("peek 0x80C8");
  strlcat(temp, output, 100);
  memset(output, 0, 1024);
  shell_evaluate("peek 0x80C8");
  assert(strcmp(output, temp) == 0);
  memset(output, 0, 1024);
  //poke input errors
  shell_evaluate("poke ");
  assert(strcmp(output, "error: poke expects 2 arguments [address] [value]\n") == 0);
  memset(output, 0, 1024);
  shell_evaluate("poke 0x8000");
  assert(strcmp(output, "error: poke expects 2 arguments [address] [value]\n") == 0);
  memset(output, 0, 1024);
  shell_evaluate("poke bob 1");
  assert(strcmp(output, "error: poke cannot convert 'bob'\n") == 0);
  memset(output, 0, 1024);
  shell_evaluate("poke aaa 1");
  assert(strcmp(output, "error: poke cannot convert 'aaa'\n") == 0);
  memset(output, 0, 1024);
  shell_evaluate("poke 0x8503 1");
  assert(strcmp(output, "error: poke address must be 4-byte aligned\n") == 0);
  memset(output, 0, 1024);
  shell_evaluate("poke 0x8003 4");
  assert(strcmp(output, "error: poke address must be 4-byte aligned\n") == 0);
  memset(output, 0, 1024);

  //combination
  shell_evaluate("poke 0x7000 45");
  memset(output, 0, 1024);
  shell_evaluate("peek 0x7000");
  assert(strcmp(output, "0x7000:  2d\n") == 0);
  memset(output, 0, 1024);

  shell_evaluate("poke 0x7A00 0x36F");
  memset(output, 0, 1024);
  shell_evaluate("peek 0x7A00");
  assert(strcmp(output, "0x7a00:  36f\n") == 0);
  memset(output, 0, 1024);

  shell_evaluate("poke 0x7A30 0x36FA");
  memset(output, 0, 1024);
  shell_evaluate("peek 0x7A30");
  assert(strcmp(output, "0x7a30:  36fa\n") == 0);
  memset(output, 0, 1024);

  shell_evaluate("poke 0x6A30 0xA36FA");
  memset(output, 0, 1024);
  shell_evaluate("peek 0x6A30");
  assert(strcmp(output, "0x6a30:  a36fa\n") == 0);
  memset(output, 0, 1024);

  //returns

  assert(shell_evaluate("help sadasdf"));
  assert(!shell_evaluate("help echo"));

  free(output);
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
    uart_init();
    keyboard_init(KEYBOARD_CLOCK, KEYBOARD_DATA);

    printf("Testing keyboard module.\n");

    test_shell_eval();

    user_test_readline();

    test_keyboard_scancodes();
    timer_delay_ms(500);

    test_keyboard_sequences();
    timer_delay_ms(500);

    test_keyboard_events();
    timer_delay_ms(500);

    test_keyboard_chars();

    test_keyboard_assert();

    printf("All done!\n");
    uart_putchar(EOT);
}
