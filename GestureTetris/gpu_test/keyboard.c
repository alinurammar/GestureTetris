#include "gpio.h"
#include "gpioextra.h"
#include "keyboard.h"
#include "ps2.h"
#include "printf.h"
#include "interrupts.h"
#include "uart.h"
#include "ringbuffer.h"

struct scanner_state {
    unsigned int scancode;
    int bits_received;
    int num_ones;
};

static unsigned int CLK, DATA;
static keyboard_modifiers_t modifiers = 0;
static struct scanner_state scanner;
rb_t* ringbuffer;

static void clear_scan_state(void) {
  scanner.scancode = 0;
  scanner.bits_received = 0;
  scanner.num_ones = 0;
}

static bool clock_edge(unsigned int pc) {
    if(gpio_check_and_clear_event(CLK)) {
      int cur_bit = gpio_read(DATA);
      if(scanner.bits_received == 0 && cur_bit == 0) {
          //start of sequence
          scanner.bits_received++;
      } else if(scanner.bits_received > 0 && scanner.bits_received < 9) {
          //main phase
          scanner.bits_received++;
          scanner.num_ones += cur_bit;
          scanner.scancode |= (cur_bit << (scanner.bits_received - 2));
      } else if(scanner.bits_received == 9) {
          //validate parity
          scanner.bits_received++;
          scanner.num_ones += cur_bit;
          if(scanner.num_ones % 2 == 0) {
            //bad parity, start over
            clear_scan_state();
          }
      } else if(scanner.bits_received == 10) {
        //validate stop bit
        if(cur_bit) {
          rb_enqueue(ringbuffer, scanner.scancode);
        }
        clear_scan_state();
      }
      return 1;
    }
    return 0;
}

void keyboard_init(unsigned int clock_gpio, unsigned int data_gpio)
{
    CLK = clock_gpio;
    gpio_set_input(CLK);
    gpio_set_pullup(CLK);

    DATA = data_gpio;
    gpio_set_input(DATA);
    gpio_set_pullup(DATA);

    //create ring buffer
    ringbuffer = rb_new();

    //setup scanner
    clear_scan_state();

    //set up interrupts
    gpio_enable_event_detection(CLK, GPIO_DETECT_FALLING_EDGE);
    interrupts_attach_handler(clock_edge, INTERRUPTS_GPIO3);
}

unsigned char keyboard_read_scancode(void)
{
  int scancode;
  while(!rb_dequeue(ringbuffer, &scancode));
  return scancode;
}

key_action_t keyboard_read_sequence(void)
{
    key_action_t action;
    unsigned char scancode = keyboard_read_scancode();
    if(scancode == PS2_CODE_EXTENDED) {
      action = keyboard_read_sequence();
    } else if(scancode == PS2_CODE_RELEASE) {
      action = keyboard_read_sequence();
      action.what = KEY_RELEASE;
    } else {
      action.what = KEY_PRESS;
      action.keycode = scancode;
    }
    return action;
}

void set_modifers(key_action_t action) {
  int code = 0;
  switch (ps2_keys[action.keycode].ch) {
    case PS2_KEY_SHIFT: code = KEYBOARD_MOD_SHIFT;
      break;
    case PS2_KEY_ALT: code = KEYBOARD_MOD_ALT;
      break;
    case PS2_KEY_CTRL: code = KEYBOARD_MOD_CTRL;
      break;
    case PS2_KEY_CAPS_LOCK: code = KEYBOARD_MOD_CAPS_LOCK;
      break;
    case PS2_KEY_SCROLL_LOCK: code = KEYBOARD_MOD_SCROLL_LOCK;
      break;
    case PS2_KEY_NUM_LOCK: code = KEYBOARD_MOD_SCROLL_LOCK;
      break;
  }
  if(code == KEYBOARD_MOD_CAPS_LOCK) {
    if(action.what == KEY_PRESS) {
      modifiers = modifiers ^ KEYBOARD_MOD_CAPS_LOCK;
    }
  } else {
      modifiers = (action.what == KEY_PRESS) ? modifiers | code : modifiers & ~code;
  }
}

int isMod(ps2_key_t key) {
  return key.ch >= 0x90 && key.ch <= 0x93;
}

int isAlpha(ps2_key_t key) {
  return key.ch >= 'a' && key.ch <= 'z';
}

key_event_t keyboard_read_event(void)
{
    key_event_t event;
    event.action = keyboard_read_sequence();
    event.key = ps2_keys[event.action.keycode];
    if(isMod(event.key)) {
      set_modifers(event.action);
    }
    event.modifiers = modifiers;
    return event;
}

unsigned char keyboard_read_next(void)
{
    key_event_t event = keyboard_read_event();
    //wait for non modifier key press
    while(isMod(event.key) || event.action.what != KEY_PRESS) {
      event = keyboard_read_event();
    }
    //return char for key
    ps2_key_t key = event.key;
    if(modifiers & KEYBOARD_MOD_SHIFT) {
      //shift pressed
      return (key.other_ch) ? key.other_ch : key.ch;
    }
    if ((modifiers & KEYBOARD_MOD_CAPS_LOCK) && isAlpha(key)) {
      //capslock active and relevant
      return key.other_ch;
    }
    return key.ch;
}
