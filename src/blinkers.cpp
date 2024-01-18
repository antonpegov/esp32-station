#include <Arduino.h>

#include "blinkers.h"
#include "config.h"

void blink(int pin, int highTime, int lowTime, int count, SemaphoreHandle_t mutex) {
    // Only print if we can take the mutex
    if (LOG_Blinks && xSemaphoreTake(mutex, (TickType_t)10) == pdTRUE) {
        Serial.print("LED on Pin " + String(pin) + " blinked " + String(count) +
                     " times ");
        Serial.println("using Core " + String(xPortGetCoreID()) + " with priority " +
                       String(uxTaskPriorityGet(NULL)));
        xSemaphoreGive(mutex);
    }

    digitalWrite(pin, HIGH);
    vTaskDelay(pdMS_TO_TICKS(highTime));
    digitalWrite(pin, LOW);
    vTaskDelay(pdMS_TO_TICKS(lowTime));
}