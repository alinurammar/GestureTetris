#include "shell.h"
#include "shell_commands.h"
#include "uart.h"
#include "keyboard.h"
#include "malloc.h"
#include "malloc_internal.h"
#include "strings.h"
#include "printf.h"
#include "shell_commands.h"
#include "pi.h"
#include "gprof.h"

#define LINE_LEN 80
#define NUM_CMDS 6
#define TOKEN_NUM 10

static formatted_fn_t shell_printf;

static const command_t commands[] = {
    {"help",    "<cmd> prints a list of commands or description of cmd", cmd_help},
    {"echo",    "<...> echos the user input to the screen", cmd_echo},
    {"reboot",  "reboot the Raspberry Pi back to the bootloader using `pi_reboot`", cmd_reboot},
    {"peek",    "Prints the contents (4 bytes) of memory at address", cmd_peek},
    {"poke",    "Stores `value` into the memory at `address`", cmd_poke},
    {"profile",    "usage \"profile [on | off | status | results]\", interfaces with gprof", cmd_profile}
};

command_t findCommand(const char* cmdName) {
  for(int i = 0; i < NUM_CMDS; i++) {
    if(strcmp(commands[i].name, cmdName) == 0) {
      return commands[i];
    }
  }
  shell_printf("error: no such command `%s`.\n", cmdName);
  command_t notFound;
  notFound.name = "notfound";
  return notFound;
}

int cmd_echo(int argc, const char *argv[])
{
    for (int i = 1; i < argc; ++i)
        shell_printf("%s ", argv[i]);
    shell_printf("\n");
    return 0;
}

int cmd_reboot(int argc, const char* argv[]) {
  pi_reboot();
  return 0;
}

int failed_convert(const char* arg, const char* rest) {
  int cmp_offset = 0;
  if(arg[0] == '0' && arg[1] == 'x') {
    cmp_offset = 2;
  }
  return strcmp(rest, arg + cmp_offset) == 0;
}

int cmd_peek(int argc, const char* argv[]) {
  if(argc < 2) {
    shell_printf("error: peek expects 1 argument [address]\n");
    return 1;
  }
  const char* rest = NULL;
  //get address
  unsigned int* address = (unsigned int*) strtonum(argv[1], &rest);
  if(failed_convert(argv[1], rest)) {
    shell_printf("error: peek cannot convert '%s'\n", argv[1]);
    return 1;
  }
  //verify alignment
  if((unsigned int) address % 4 != 0) {
    shell_printf("error: peek address must be 4-byte aligned\n");
    return 1;
  }
  shell_printf("%p:  %08x\n", address, *address);
  return 0;
}

int cmd_poke(int argc, const char* argv[]) {
  if(argc < 3) {
    shell_printf("error: poke expects 2 arguments [address] [value]\n");
    return 1;
  }
  const char* rest = NULL;
  //get address
  unsigned int* address = (unsigned int*) strtonum(argv[1], &rest);
  if(failed_convert(argv[1], rest)) {
    shell_printf("error: poke cannot convert '%s'\n", argv[1]);
    return 1;
  }
  //get value
  rest = NULL;
  unsigned int value = strtonum(argv[2], &rest);
  if(failed_convert(argv[2], rest)) {
    shell_printf("error: poke cannot convert '%s'\n", argv[2]);
    return 1;
  }
  //verify alignment
  if((unsigned int) address % 4 != 0) {
    shell_printf("error: poke address must be 4-byte aligned\n");
    return 1;
  }
  //write
  *address = value;
  return 0;
}

int cmd_help(int argc, const char *argv[])
{
    if(argc == 1) {
      //print all commands
      for(int i = 0; i < NUM_CMDS; i++) {
        shell_printf("%s: %s\n", commands[i].name, commands[i].description);
      }
    } else {
      command_t command = findCommand(argv[1]);
      if(strcmp(command.name, "notfound")) {
        shell_printf("%s: %s\n", command.name, command.description);
        return 0;
      }
    }
    return 1;
}

int cmd_profile(int argc, const char *argv[]) {

  if(strcmp(argv[1], "on") == 0) {
    gprof_on();
  } else if(strcmp(argv[1], "off") == 0) {
    gprof_off();
  } else if(strcmp(argv[1], "status") == 0) {
    if(gprof_is_active()){
      shell_printf("Status: on\n");
    } else {
      shell_printf("Status: off\n");
    }
  } else if(strcmp(argv[1], "results") == 0) {
    gprof_dump();
  } else {
    shell_printf("%s not in [on | off | status | results]\n", argv[1]);
  }
  return 0;
}

void shell_init(formatted_fn_t print_fn)
{
    gprof_init();
    shell_printf = print_fn;
}

void shell_bell(void)
{
    uart_putchar('\a');
}

void shell_readline(char buf[], size_t bufsize)
{
    for(int i = 0; i < bufsize; i++) {
      char in = keyboard_read_next();
      buf[i] = in;
      //handle enter
      if(in == '\n') {
        shell_printf("\n");
        buf[i] = '\0';
        return;
      }
      //handle other input
      if(in == '\b') {
        //backspace
        i--;
        if(i < 0) {
          shell_bell();
        } else {
          i--;
          shell_printf("\b \b");
        }
      } else if(i == bufsize - 1) {
        //out of bounds, as last char must '\n'
        shell_bell();
        i--;
      } else {
        shell_printf("%c", in);
      }
    }
}

int isWhiteSpace(const char c) {
  return c == ' ' || c == '\n' || c == '\t';
}

struct tokenArr {
  int length;
  const char** tokens;
};

void freeTokenArr(struct tokenArr arr) {
  for(int i = 0; i < arr.length; i++) {
    free((void*) arr.tokens[i]);
  }
  free((void*) arr.tokens);
}

void ensureToken(const char** tokens) {
    char* token = malloc(1);
    memset(token, '\0', 1);
    tokens[0] = token;
}

struct tokenArr tokenize(const char *line) {
  struct tokenArr arr;
  int numTokens = 0;
  int length = strlen(line);
  int size = TOKEN_NUM;
  const char** tokens = malloc(size * sizeof(char*));
  for(int i = 0; i < length; i++) {
    if(!isWhiteSpace(line[i])) {
      //find distance to end of token
      int dist = 1;
      while(i + dist < length && !isWhiteSpace(line[i + dist])) {
        dist++;
      }
      //create token string
      char* token = malloc(dist + 1);
      memcpy(token, line + i, dist);
      memset(token + dist, '\0', 1);
      //increase size if necessary
      numTokens++;
      if(numTokens > size) {
        size = size + TOKEN_NUM;
        tokens = realloc(tokens, size * sizeof(char*));
      }
      //store token
      tokens[numTokens - 1] = token;
      //move past token
      i += dist - 1;
    }
  }
  //add an empty token if none were added
  if(!numTokens) {
    ensureToken(tokens);
  }
  arr.length = numTokens;
  arr.tokens = tokens;
  return arr;
}

int shell_evaluate(const char *line)
{
    struct tokenArr tokenArr = tokenize(line);
    command_t command = findCommand(tokenArr.tokens[0]);
    int result = -1;
    if(strcmp(command.name, "notfound") != 0) {
      result = (command.fn)(tokenArr.length, tokenArr.tokens);
    }
    freeTokenArr(tokenArr);
    return result;
}

void shell_run(void)
{
    shell_printf("Welcome to the CS107E shell. Remember to type on your PS/2 keyboard!\n");
    while (1)
    {
        char line[LINE_LEN];

        shell_printf("Pi> ");
        shell_readline(line, sizeof(line));
        shell_evaluate(line);
    }
}
