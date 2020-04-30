#include "../assert.h"
#include "../gpio.h"
#include "../timer.h"

void test_gpio_set_get_function(void) {
    gpio_init();

    // Test get pin function (pin2 defaults to input)
    assert( gpio_get_function(GPIO_PIN2) == GPIO_FUNC_INPUT );

    // Test set pin to output
    gpio_set_output(GPIO_PIN2);

    // Test get pin function after setting
    assert( gpio_get_function(GPIO_PIN2) == GPIO_FUNC_OUTPUT );

    //test pins in other registers
    gpio_set_output(GPIO_PIN19);
    gpio_set_output(GPIO_PIN27);
    gpio_set_output(GPIO_PIN34);
    gpio_set_output(GPIO_PIN48);
    assert( gpio_get_function(GPIO_PIN19) == GPIO_FUNC_OUTPUT );
    assert( gpio_get_function(GPIO_PIN27) == GPIO_FUNC_OUTPUT );
    assert( gpio_get_function(GPIO_PIN34) == GPIO_FUNC_OUTPUT );
    assert( gpio_get_function(GPIO_PIN48) == GPIO_FUNC_OUTPUT );
    //look at other modes
    gpio_set_function(GPIO_PIN31, GPIO_FUNC_ALT5);
    gpio_set_function(GPIO_PIN24, GPIO_FUNC_ALT3);
    gpio_set_function(GPIO_PIN50, GPIO_FUNC_ALT2);
    assert( gpio_get_function(GPIO_PIN31) == GPIO_FUNC_ALT5 );
    assert( gpio_get_function(GPIO_PIN24) == GPIO_FUNC_ALT3 );
    assert( gpio_get_function(GPIO_PIN50) == GPIO_FUNC_ALT2 );
    //many pins in same register
    gpio_set_function(GPIO_PIN11, GPIO_FUNC_ALT5);
    gpio_set_function(GPIO_PIN12, GPIO_FUNC_ALT3);
    gpio_set_function(GPIO_PIN13, GPIO_FUNC_ALT2);
    gpio_set_function(GPIO_PIN16, GPIO_FUNC_ALT4);
    gpio_set_function(GPIO_PIN17, GPIO_FUNC_OUTPUT);
    assert( gpio_get_function(GPIO_PIN11) == GPIO_FUNC_ALT5 );
    assert( gpio_get_function(GPIO_PIN12) == GPIO_FUNC_ALT3 );
    assert( gpio_get_function(GPIO_PIN13) == GPIO_FUNC_ALT2 );
    assert( gpio_get_function(GPIO_PIN16) == GPIO_FUNC_ALT4 );
    assert( gpio_get_function(GPIO_PIN17) == GPIO_FUNC_OUTPUT );
    //improper requests
    assert( gpio_get_function(70) == -1 );
    assert( gpio_get_function(-10) == -1 );
}

void test_gpio_read_write(void) {
    gpio_init();
    gpio_set_function(GPIO_PIN20, GPIO_FUNC_OUTPUT);

    // Test gpio_write low, then gpio_read
    gpio_write(GPIO_PIN20, 0);
    assert( gpio_read(GPIO_PIN20) ==  0 );

   // Test gpio_write high, then gpio_read
    gpio_write(GPIO_PIN20, 1);
    assert( gpio_read(GPIO_PIN20) ==  1 );

    //near register boundries test
    gpio_set_function(GPIO_PIN31, GPIO_FUNC_OUTPUT);
    gpio_set_function(GPIO_PIN32, GPIO_FUNC_OUTPUT);
    gpio_set_function(GPIO_PIN33, GPIO_FUNC_OUTPUT);

    gpio_write(GPIO_PIN31, 1);
    assert( gpio_read(GPIO_PIN31) ==  1 );
    gpio_write(GPIO_PIN32, 0);
    assert( gpio_read(GPIO_PIN32) ==  0 );
    gpio_write(GPIO_PIN33, 1);
    assert( gpio_read(GPIO_PIN33) ==  1 );

    //upper register test and rewrite test
    gpio_set_function(GPIO_PIN41, GPIO_FUNC_OUTPUT);
    gpio_set_function(GPIO_PIN38, GPIO_FUNC_OUTPUT);

    gpio_write(GPIO_PIN41, 0);
    assert( gpio_read(GPIO_PIN41) ==  0 );
    gpio_write(GPIO_PIN38, 0);
    assert( gpio_read(GPIO_PIN38) ==  0 );
    gpio_write(GPIO_PIN41, 1);
    assert( gpio_read(GPIO_PIN41) ==  1 );
    gpio_write(GPIO_PIN38, 1);
    assert( gpio_read(GPIO_PIN38) ==  1 );

    //low register test
    gpio_set_function(GPIO_PIN3, GPIO_FUNC_OUTPUT);
    gpio_set_function(GPIO_PIN5, GPIO_FUNC_OUTPUT);
    gpio_set_function(GPIO_PIN7, GPIO_FUNC_OUTPUT);

    gpio_write(GPIO_PIN3, 1);
    assert( gpio_read(GPIO_PIN3) ==  1 );
    gpio_write(GPIO_PIN5, 1);
    assert( gpio_read(GPIO_PIN5) ==  1 );
    gpio_write(GPIO_PIN7, 1);
    assert( gpio_read(GPIO_PIN7) ==  1 );

    //bad requests
    assert( gpio_read(-10) == GPIO_INVALID_REQUEST);
    assert( gpio_read(70) == GPIO_INVALID_REQUEST);
}

void test_timer(void) {
    timer_init();

    // Test timer tick count incrementing
    unsigned int start = timer_get_ticks();
    for( int i=0; i<10; i++ ) { /* Spin */ }
    unsigned int finish = timer_get_ticks();
    assert( finish > start );

    // Test timer delay
    int usecs = 100;
    start = timer_get_ticks();
    timer_delay_us(usecs);
    finish = timer_get_ticks();
    assert( finish >= start + usecs );

    //Make sure reasonably accurate
    usecs = 200;
    start = timer_get_ticks();
    timer_delay_us(usecs);
    finish = timer_get_ticks();
    assert( (finish >= start + usecs)  && (finish <= start + usecs + 5));

}

// Uncomment each call below when you have implemented the functions
// and are ready to test them

void main(void) {
    test_gpio_set_get_function();
    test_gpio_read_write();
    test_timer();
}
