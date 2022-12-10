#include <Arduino.h>
#include <unity.h>

#define LED_pin 13

void test_led_builtin_pin_number(void)
{
    TEST_ASSERT_EQUAL(LED_pin, LED_BUILTIN);
}

void test_led_state_high(void)
{
    digitalWrite(LED_BUILTIN, HIGH);
    TEST_ASSERT_EQUAL(HIGH, LED_BUILTIN);
}

void test_led_state_low(void)
{
    digitalWrite(LED_BUILTIN, LOW);
    TEST_ASSERT_EQUAL(LOW, LED_BUILTIN);
}

void setup()
{
    delay(1000);

    UNITY_BEGIN();//start unit test
    RUN_TEST(test_led_builtin_pin_number);

    pinMode(LED_pin, OUTPUT);
}

uint8_t i = 0;
uint8_t blink_counts = 5;

void loop()
{
    if (i < blink_counts)
    {

        RUN_TEST(test_led_state_high);
        delay(10000);
        RUN_TEST(test_led_state_low);
        delay(1000);
        i++;
    }
    else if (i == blink_counts)
    {
        UNITY_END();// stop unit test
    }
}