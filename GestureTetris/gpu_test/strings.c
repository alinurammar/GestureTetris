#include "strings.h"

static unsigned int pow(unsigned int base, unsigned int power);
size_t min(size_t a, size_t b);

void *memset(void *s, int c, size_t n)
{
    char* position = (char*) s;
    while(n > 0) {
      *position = (char) c;
      position++;
      n--;
    }
    return s;
}

void *memcpy(void *dst, const void *src, size_t n)
{
    char* destination = (char*) dst;
    char* source = (char*) source;

    while(n > 0) {
      *destination = *source;
      destination++;
      source++;
      n--;
    }
    return dst;
}

size_t strlen(const char *s)
{
    /* Implementation a gift to you from lab3 */
    int n = 0;
    while (s[n] != '\0') {
        n++;
    }
    return n;
}

int strcmp(const char *s1, const char *s2)
{
    while (*s1 && *s2) {
      if(*s1 > *s2) {
        return 1;
      }
      if(*s1 < *s2) {
        return -1;
      }
      s1 += 1;
      s2 += 1;
    }
    //equal until second ends
    if(*s1 && !*s2) {
      return 1;
    }
    //equal until first ends
    if(*s2 && !*s1) {
      return -1;
    }
    //equal until both end
    return 0;
}

size_t strlcat(char *dst, const char *src, size_t maxsize)
{
    int dstLength = strlen(dst);
    int srcLength = strlen(src);
    //copy as much of src as possible
    memcpy(dst + dstLength, src, min(srcLength, maxsize - dstLength - 1));
    //add terminating character
    memset(dst + dstLength + min(srcLength, maxsize - dstLength - 1), 0, 1);
    //return total lenght if no constraint
    return srcLength + dstLength;
}

size_t min(size_t a, size_t b) {
  return (a > b) ? b : a;
}

unsigned int strtonum(const char *str, const char **endptr)
{
    //start with endptr inialized and pointing to the beginning of the string
    if(!endptr) {
      const char* stringPointer[1];
      endptr = stringPointer;
    }
    *endptr = str;
    //determine if string is hexedcimal
    int hex = 0;
    if((*endptr)[0] == '0' && (*endptr)[1] == 'x')
    {
      hex = 1;
      *endptr+=2;
    }
    const char* start = *endptr;
    //advance endptr as long as charcters ascii for 0-9 or if hex A-F as well
    while((**endptr >= 48 && **endptr <= 57) ||
          (hex && ((**endptr >= 65 && **endptr <= 70) || (**endptr >= 'a' && **endptr <= 'f'))))
    {
        (*endptr)++;
    }
    //sum total
    unsigned int total = 0;
    unsigned int digit = 0;
    for(const char* pos = *endptr - 1; start <= pos; pos--) {
      //store ascii value of digit
      int digitValue = *pos;
      //find actual value of digit
      if(digitValue < 60) {
        digitValue-=48;
      } else if(digitValue < 75) {
        digitValue-=55;
      } else {
        digitValue-=87;
      }
      //add to total
      total += (hex) ? digitValue * pow(16, digit) : digitValue * pow(10, digit);
      digit++;
    }
    return total;
}

static unsigned int pow(unsigned int base, unsigned int power) {
  unsigned int total = 1;
  while(power > 0) {
    total *= base;
    power--;
  }
  return total;
}
