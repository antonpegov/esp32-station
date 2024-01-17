#include "./blinkers.h"
#include <Arduino.h>

void blink(int pin, int highTime, int lowTime, int count, SemaphoreHandle_t mutex) {
    // Only print if we can take the mutex
    if (xSemaphoreTake(mutex, (TickType_t)10) == pdTRUE) {
        Serial.println("LED" + String(pin) + " blinked " + String(count) + " times...");
        xSemaphoreGive(mutex);
    }

    digitalWrite(pin, HIGH);
    vTaskDelay(pdMS_TO_TICKS(highTime));
    digitalWrite(pin, LOW);
    vTaskDelay(pdMS_TO_TICKS(lowTime));
}