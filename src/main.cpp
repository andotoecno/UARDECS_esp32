#include <Arduino.h>
#include "esp32_uecs.h"
#define LED_pin 13

void setup() {
  delay(1000);
  pinMode(LED_pin,OUTPUT);
}

void loop() {
  digitalWrite(LED_pin,HIGH);
  delay(10000);
  digitalWrite(LED_pin,LOW);
  delay(1000);

}