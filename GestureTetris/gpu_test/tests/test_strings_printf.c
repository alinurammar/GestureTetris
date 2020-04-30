#include "assert.h"
#include "printf.h"
#include "../printf_internal.h"
#include <stddef.h>
#include "strings.h"
#include "uart.h"

static void test_memset(void)
{
    int numA = 0xefefefef;
    int numB = 2;

    memset(&numB, 0xef, sizeof(int));
    assert(numA == numB);

    //my tests
    int numC = 50;
    int numD = 10;
    memset(&numC, numD, 0);
    assert(numC = 50);
    memset(&numC, numD, 1);
    assert(numC == 10);
    memset(&numC, 0, 4);
    assert(numC == 0);
}

static void test_memcpy(void)
{
    int numA = 0x12345678;
    int numB = 2;

    memcpy(&numB, &numA, sizeof(int));
    assert(numA == numB);

    //mytests
    int numC = 130;
    int numD = 35;
    memcpy(&numC, &numD, 0);
    assert(numC == 130);

    int numE = 255;
    int numF = 0;
    memcpy(&numE, &numF, 1);
    assert(numE == 0);

    char test[3];
    memset(test, 0, 3);
    memcpy(test, "abcde", 2);
    assert(test[0] == 'a');
    assert(test[1] == 'b');
    assert(test[2] == '\0');

    char string[10];
    memset(string, 0, 10);
    memcpy(string, "abcdefghij", 9);
    assert(string[0] == 'a');
    assert(string[8] == 'i');
    assert(string[9] == '\0');
}

static void test_strlen(void)
{
    assert(strlen("green") == 5);
    assert(strlen("") == 0);
    assert(strlen("test") == 4);
}

static void test_strcmp(void)
{
    assert(strcmp("apple", "apple") == 0);
    assert(strcmp("apple", "applesauce") < 0);
    assert(strcmp("pears", "apples") > 0);
    //my tests
    assert(strcmp("ab", "ba") < 0);
    assert(strcmp("", "") == 0);
    assert(strcmp("cat", "dog") < 0);
    assert(strcmp("catsss", "dog") < 0);
    assert(strcmp("dogs", "dog") > 0);
    assert(strcmp("tree", "trees") < 0);
    assert(strcmp("zebra", "catnip") > 0);
    assert(strcmp("A", "a") < 0);

}

static void test_strlcat(void)
{
    char buf[20];
    memset(buf, 0x77, sizeof(buf)); // init contents with known value

    buf[0] = '\0'; // start with empty string
    assert(strlen(buf) == 0);
    strlcat(buf, "CS", sizeof(buf));
    assert(strlen(buf) == 2);
    assert(strcmp(buf, "CS") == 0);
    strlcat(buf, "107e", sizeof(buf));
    assert(strlen(buf) == 6);
    assert(strcmp(buf, "CS107e") == 0);

    //my tests
    char buf2[10];
    memset(buf2, 0, 9);
    strlcat(buf2, "abcdefghij", 10);
    assert(buf2[0] == 'a');
    assert(buf2[8] == 'i');
    assert(buf2[9] == '\0');
    assert(strlen(buf2) == 9);

    char buf3[15];
    memset(buf3, 0, 1);
    strlcat(buf3, "hello ", 10);
    assert(strlen(buf3) == 6);
    strlcat(buf3, "world! .......", 15);
    assert(strlen(buf3) == 14);
    assert(buf3[0] == 'h');
    assert(buf3[6] == 'w');
    assert(buf3[11] == '!');
    assert(buf3[12] == ' ');
    assert(buf3[13] == '.');
    assert(buf3[14] == '\0');
    assert(strcmp(buf3, "hello world! .") == 0);
}


static void test_strtonum(void)
 {
    printf("%s\n", "hello world");

    int val = strtonum("013", NULL);
    assert(val == 13);

    const char *input = "107rocks", *rest = NULL;
    val = strtonum(input, &rest);
    assert(val == 107);
    assert(rest == &input[3]);

    //hex
    int val2 = strtonum("0x2A4", NULL);
    assert(val2 == 676);

    int val3 = strtonum("0xF", NULL);
    assert(val3 == 15);


    int val4 = strtonum("0x100qqqqqqq", NULL);
    assert(val4 == 256);

    unsigned int val5 = strtonum("0xdeadbeef", NULL);
    // printf("tets: %d\n", val5);
    assert(val5 == 3735928559);

}

static void test_to_base(void)
{
    char buf[5];
    size_t bufsize = sizeof(buf);

    memset(buf, 0x77, bufsize); // init contents with known value

    int n = signed_to_base(buf, bufsize, -9999, 10, 6);
    assert(strcmp(buf, "-099") == 0);
    assert(n == 6);

    //my tests
    char buf2[10];
    size_t bufsize2 = sizeof(buf2);
    signed_to_base(buf2, bufsize2, 100, 10, 6);
    assert(strcmp(buf2, "000100") == 0);

    char buf3[6];
    int u = signed_to_base(buf3, 5, 500, 16, 3);
    assert(strcmp(buf3, "1f4") == 0);
    assert(u == 3);

    char buf4[7];
    u = signed_to_base(buf4, 7, 6500, 16, 7);
    assert(strcmp(buf4, "000196") == 0);
    assert(u == 7);

    //broken test_strcmp
    char buff5[100];
    int test = signed_to_base(buff5, 100, 0x0, 10, 0);
    assert(strcmp(buff5, "0"));
    assert(test == 0);
    char buff6[100];
    test = unsigned_to_base(buff6, 100, 0x0, 10, 0);
    assert(strcmp(buff6, "0"));
    assert(test == 0);
}

static void test_snprintf(void)
{
    char buf[100];
    size_t bufsize = sizeof(buf);

    memset(buf, 0x77, sizeof(buf)); // init contents with known value

    // Start off simple...
    snprintf(buf, bufsize, "Hello, world!");
    assert(strcmp(buf, "Hello, world!") == 0);

    //my tests
    snprintf(buf, bufsize, "Hello, %% world!");
    assert(strcmp(buf, "Hello, % world!") == 0);

    snprintf(buf, bufsize, "Hello, %c world!", 'a');
    assert(strcmp(buf, "Hello, a world!") == 0);

    snprintf(buf, bufsize, "test %s test", "test");
    assert(strcmp(buf, "test test test") == 0);

    snprintf(buf, bufsize, "%x", 0x46);
    assert(strcmp(buf, "46") == 0);

    snprintf(buf, bufsize, "nums %d nums %d", -45, 30);
    assert(strcmp(buf, "nums -45 nums 30") == 0);

    snprintf(buf, bufsize, "nums %x nums %x", 0xff, 0xa2);
    assert(strcmp(buf, "nums ff nums a2") == 0);

    // tests to add, more pointers, more return value, more weirdness
    snprintf(buf, bufsize, "%03x%%%03d", 0xff, 10);
    assert(strcmp(buf, "0ff%010") == 0);

    snprintf(buf, bufsize, "Pointer: %p", (void *) 0x20200007);
    assert(strcmp(buf, "Pointer: 0x20200007") == 0);

    char buf2[10];
    int n = snprintf(buf2, 10, "nums %x nums %x", 0xff, 0xa2);
    assert(strcmp(buf2, "nums ff n") == 0);
    assert(n == 15);

    // Decimal
    snprintf(buf, bufsize, "%d", 45);
    assert(strcmp(buf, "45") == 0);

    // Hexadecimal
    snprintf(buf, bufsize, "%04x", 0xef);
    assert(strcmp(buf, "00ef") == 0);

    // Pointer
    snprintf(buf, bufsize, "%p", (void *) 0x20200004);
    assert(strcmp(buf, "0x20200004") == 0);

    char buf3[30];
    snprintf(buf3, 30, "0x%x", (unsigned int) buf);
    //initially failed
    snprintf(buf, bufsize, "%p", buf);
    printf("buf pointer:%p\n", buf);
    assert(strcmp(buf, buf3) == 0);

    // Character
    snprintf(buf, bufsize, "%c", 'A');
    assert(strcmp(buf, "A") == 0);

    //String
    snprintf(buf, bufsize, "%s", "binky");
    assert(strcmp(buf, "binky") == 0);

    // Format string with intermixed codes
    snprintf(buf, bufsize, "CS%d%c!", 107, 'e');
    assert(strcmp(buf, "CS107e!") == 0);

    // Test return value
    assert(snprintf(buf, bufsize, "Hello") == 5);
    assert(snprintf(buf, 2, "Hello") == 5);

}


void main(void)
{

    // uart_init();
    // uart_putstring("Start execute main() in tests/test_strings_printf.c\n");
    test_memset();
    test_memcpy();
    test_strlen();
    test_strcmp();
    test_strlcat();
    test_strtonum();
    test_to_base();
    test_snprintf();

    // uart_putstring("Successfully finished executing main() in tests/test_strings_printf.c\n");
    // uart_putchar(EOT);
}
