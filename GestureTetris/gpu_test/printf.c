#include "printf.h"
#include "printf_internal.h"
#include <stdarg.h>
#include "strings.h"
#include "uart.h"

#define MAX_OUTPUT_LEN 1024

size_t min(size_t a, size_t b);
static unsigned int lengthToCode(const char *buf, size_t bufsize, int readLoc);

int unsigned_to_base(char *buf, size_t bufsize, unsigned int val, int base, int min_width)
{
    //find length of string conversion
    int length = 0;
    unsigned int valCopy = val;
    while(valCopy > 0) {
      valCopy = valCopy / base;
      length++;
    }
    int offset = 0;
    if(min_width > length) {
      offset = min_width - length;
    }
    //fill offset with 0s
    memset(buf, 48, offset);
    //add in number
    for(int i = offset + length - 1; i >= offset; i--) {
      unsigned int digit = val % base;
      char asciiDigit = (digit >= 10) ? digit + 87: digit + 48;
      val = val / base;
      if(i < bufsize - 1) {
        buf[i] = asciiDigit;
      }
    }
    //add terminating character
    buf[min(offset + length, bufsize - 1)] = '\0';
    return offset + length;
}

int signed_to_base(char *buf, size_t bufsize, int val, int base, int min_width)
{
    if(val >= 0) {
      return unsigned_to_base(buf, bufsize, val, base, min_width);
    } else {
      buf[0] = '-';
      return 1 + unsigned_to_base(buf + 1, bufsize - 1, val * -1, base, min_width - 1);
    }
}

int vsnprintf(char *buf, size_t bufsize, const char *format, va_list args)
{
  //inialize variables
  int writeLoc = 0;
  int readLoc = 0;
  int min_width = 0;
  //create working buffer
  char workBuf[MAX_OUTPUT_LEN];
  memset(workBuf, 0, MAX_OUTPUT_LEN);
  //flaw, may not calculate required lengths if length is greater than buffer size
  while(readLoc < strlen(format)) {
    //write simple text till code
    int textLength = lengthToCode(format, strlen(format), readLoc);
    memcpy(workBuf + writeLoc, format + readLoc, textLength);
    writeLoc += textLength;
    readLoc += textLength + 1;
    //break finished reading formating string or % with no code
    if(readLoc > strlen(format) - 1) {
        break;
    }
    //handle codes
    if(format[readLoc] == '%') {
        strlcat(workBuf, "%", MAX_OUTPUT_LEN);
        writeLoc++;
    }
    if(format[readLoc] == 'c') {
        memset(workBuf + writeLoc, va_arg(args, int), 1);
        writeLoc++;
    }
    if(format[readLoc] == 's') {
      char* toAppend = va_arg(args, char*);
      strlcat(workBuf, toAppend, MAX_OUTPUT_LEN);
      writeLoc += strlen(toAppend);
    }
    if(format[readLoc] == 'p') {
      strlcat(workBuf, "0x", MAX_OUTPUT_LEN);
      writeLoc += 2;
      int length = signed_to_base(workBuf + writeLoc, MAX_OUTPUT_LEN - writeLoc,
        va_arg(args, int), 16, 8);
      writeLoc += length;
    }
    //setup min_width parameter for d and x
    min_width = 1;
    if(format[readLoc] == '0') {
      const char* endLoc[1];
      min_width = strtonum(format + readLoc, endLoc);
      readLoc = *endLoc - format;
    }
    if(format[readLoc] == 'd') {
      int length = signed_to_base(workBuf + writeLoc, MAX_OUTPUT_LEN - writeLoc,
        va_arg(args, int), 10, min_width);
      writeLoc += length;
    }
    if(format[readLoc] == 'x') {
      int length = unsigned_to_base(workBuf + writeLoc, MAX_OUTPUT_LEN - writeLoc,
        va_arg(args, int), 16, min_width);
      writeLoc += length;
    }
    readLoc += 1;
  }
  buf[0] = '\0';
  return strlcat(buf, workBuf, bufsize);
}

int snprintf(char *buf, size_t bufsize, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    return vsnprintf(buf, bufsize, format, args);
}

int printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    char output[MAX_OUTPUT_LEN];
    int size = vsnprintf(output, MAX_OUTPUT_LEN, format, args);
    uart_putstring(output);
    return size;
}

//returns the number of characters starting from readloc before a % symbol is reached
static unsigned int lengthToCode(const char *buf, size_t bufsize, int readLoc) {
    int i = readLoc;
    while(i < bufsize) {
        if(buf[i] == '%') {
            return i - readLoc;
        }
        i++;
    }
    return i - readLoc;
}
