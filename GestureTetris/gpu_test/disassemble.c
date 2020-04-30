#include "printf.h"
#include <stdint.h>
#include "uart.h"
#include "strings.h"


static const char *cond[16] = {"eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
                               "hi", "ls", "ge", "lt", "gt", "le", "", ""};
static const char *opcodes[16] = {"and", "eor", "sub", "rsb", "add", "adc", "sbc", "rsc",
                                  "tst", "teq", "cmp", "cmn", "orr", "mov", "bic", "mvn"};

static const char *shifts[4] = {"LSL", "LSR", "ASR", "ROR"};



/*
 * This bitfield is declared using exact same layout as bits are organized in
 * the encoded instruction. Accessing struct.field will extract just the bits
 * apportioned to that field. If you look at the assembly the compiler generates
 * to access a bitfield, you will see it simply masks/shifts for you. Neat!
 */

struct insn  {
    uint32_t reg_op2:4;
    uint32_t one:1;
    uint32_t shift_op: 2;
    uint32_t shift: 5;
    uint32_t reg_dst:4;
    uint32_t reg_op1:4;
    uint32_t s:1;
    uint32_t opcode:4;
    uint32_t imm:1;
    uint32_t kind:2;
    uint32_t cond:4;
};

//mirrors arm assemblies rotate right
unsigned int rotateRight(unsigned int rotation, unsigned int toRotate) {
  int rot = rotation << 1;
  return (toRotate << (32 - rot)) | (toRotate >> rot);
}
//decodes the shifter operand for shifter registers and appends to a string
static void decodeRegShifter(char* buf, int bufsize, struct insn in) {
  char shiftBuf[50];
  int shiftBufSize = 50;
  shiftBuf[0] = '\0';
  //there are two cases, shifting reg_op2 by an immediate and by another register
  if(in.one) {
    //register shift
    snprintf(shiftBuf, shiftBufSize, "r%01d, %s r%01d", in.reg_op2,
             shifts[in.shift_op], in.shift >> 1);
  } else {
    //immediate shift
    if(in.shift != 0) {
      snprintf(shiftBuf, shiftBufSize, "r%01d, %s #%d", in.reg_op2,
               shifts[in.shift_op], in.shift);
    } else {
      snprintf(shiftBuf, shiftBufSize, "r%01d", in.reg_op2);
    }
  }
  strlcat(buf, shiftBuf, bufsize);
}

static void decode(unsigned int *addr)
{
    struct insn in = *(struct insn *)addr;
    //decode
    char buf[50];
    int bufsize = 50;

    if(*addr == 0xe12fff1e) {
      //bx lr handled as special case as it is picked up by tst otherwise
      snprintf(buf, bufsize, "bx lr");
    } else if((in.kind == 0) && (in.imm == 0)) {
        //handle data processing instruction with no immediates
        char* setFlag = (in.s) ? "s" : "";
        if((strcmp(opcodes[in.opcode], "mov") == 0) || (strcmp(opcodes[in.opcode], "mvn") == 0)) {
            //handle mov instructions seperately
            snprintf(buf, bufsize, "%s%s%s r%01d, ", opcodes[in.opcode], setFlag,
            cond[in.cond], in.reg_dst);
            decodeRegShifter(buf, bufsize, in);
        } else if((strcmp(opcodes[in.opcode], "cmp") == 0) ||
                  (strcmp(opcodes[in.opcode], "cmn") == 0) ||
                  (strcmp(opcodes[in.opcode], "tst") == 0) ||
                  (strcmp(opcodes[in.opcode], "teq") == 0)) {
            //decode comparisons
            snprintf(buf, bufsize, "%s%s r%01d, ", opcodes[in.opcode],
            cond[in.cond], in.reg_op1);
            decodeRegShifter(buf, bufsize, in);
        } else {
            snprintf(buf, bufsize, "%s%s%s r%01d, r%01d, ", opcodes[in.opcode], setFlag,
            cond[in.cond], in.reg_dst, in.reg_op1);
            decodeRegShifter(buf, bufsize, in);
        }
    } else if((in.kind == 0) && (in.imm == 1)) {
        //handle data proccesing with immediates
        char* setFlag = (in.s) ? "s" : "";
        //handle mov instructions seperately
        if((strcmp(opcodes[in.opcode], "mov") == 0) || (strcmp(opcodes[in.opcode], "mvn") == 0)) {
            snprintf(buf, bufsize, "%s%s%s r%01d, #%01d", opcodes[in.opcode], setFlag,
            cond[in.cond], in.reg_dst, rotateRight(in.shift >> 1, *addr & 255));

        } else if((strcmp(opcodes[in.opcode], "cmp") == 0) ||
                  (strcmp(opcodes[in.opcode], "cmn") == 0) ||
                  (strcmp(opcodes[in.opcode], "tst") == 0) ||
                  (strcmp(opcodes[in.opcode], "teq") == 0)) {
            //decode comparisons
            snprintf(buf, bufsize, "%s%s r%01d, #%01d", opcodes[in.opcode],
            cond[in.cond], in.reg_op1, rotateRight(in.shift >> 1, *addr & 255));
        } else {
            snprintf(buf, bufsize, "%s%s%s r%01d, r%01d, #%01d", opcodes[in.opcode],
            setFlag, cond[in.cond], in.reg_dst, in.reg_op1, rotateRight(in.shift >> 1, *addr & 255));
        }
    } else if((in.kind == 2) && (in.imm == 1)) {
        //branches
        //find offset
        int offset = ((*addr << 8) >> 8);
        if(((offset >> 23) & 1) == 0) {
          offset = offset << 2;
          offset+=8;
        } else {
          int signExt = 511;
          signExt = signExt << 24;
          offset = offset & signExt;
          offset = offset << 2;
        }
        char* setL = (in.opcode >> 3) ? "l" : "";
        snprintf(buf, bufsize, "b%s%s %x", setL, cond[in.cond], (unsigned int) addr + offset);
    } else if((in.kind == 1) && (in.imm == 0)) {
        //load and store with immediate offsets
        char* bFlag = ((*addr >> 22) & 1) ? "b" : "";
        char* uFlag = ((*addr >> 23) & 1) ? "" : "-";
        char* sFlag = (in.s) ? "ldr" : "str";
        snprintf(buf, bufsize, "%s%s%s r%01d, [r%01d, #%s%01d] ", sFlag,
        cond[in.cond], bFlag, in.reg_dst, in.reg_op1, uFlag, ((*addr << 20) >> 20));

    } else if((in.kind == 1) && (in.imm == 1)) {
        //load and store with register offsets
        char* bFlag = ((*addr >> 22) & 1) ? "b" : "";
        char* uFlag = ((*addr >> 23) & 1) ? "" : "-";
        char* sFlag = (in.s) ? "ldr" : "str";

        snprintf(buf, bufsize, "%s%s%s r%01d, [r%01d, %s",
        sFlag, cond[in.cond], bFlag, in.reg_dst, in.reg_op1, uFlag);

        decodeRegShifter(buf, bufsize, in);
        strlcat(buf, "]", bufsize);
    }  else {
        snprintf(buf, bufsize, "%x", *addr);
    }
    //print
    printf("%p: %08x    %s\n", addr, *addr, buf);
}

void main(void)
{
    uart_init();
    unsigned int* start = (unsigned int*) 0x8000;
    int n = 0;
    while(n <= 100) {
      decode(start + n);
      n++;
    }
    uart_putchar(EOT);
}
