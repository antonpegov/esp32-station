#include <Arduino.h>

void blink(int pin, int highTime, int lowTime, int count, SemaphoreHandle_t mutex);