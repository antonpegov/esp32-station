#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include "./blinkers.h"
#include "./helpers.h"

#define led1HighTimeMS 1000
#define led1LowTimeMS 5000
#define led2HighTimeMS 1500
#define led2LowTimeMS 3000

#define SDA_Pin 40
#define SCL_Pin 41

int outPin1Count = 0;
int outPin2Count = 0;
int outPin1 = 1; // External LED1
int outPin2 = 2; // External LED2
SemaphoreHandle_t mutex;

void blinkLed1Task(void *parameter);
void blinkLed2Task(void *parameter);
void showLogo();
void showText();

#pragma region Tasks
void blinkLed1Task(void *parameter) {
    for (;;) blink(outPin1, led1HighTimeMS, led1LowTimeMS, ++outPin1Count, mutex);
}

void blinkLed2Task(void *parameter) {
    for (;;) blink(outPin2, led2HighTimeMS, led2LowTimeMS, ++outPin2Count, mutex);
}
#pragma endregion

void setup() {
    mutex = xSemaphoreCreateMutex();
    pinMode(outPin1, OUTPUT);
    pinMode(outPin2, OUTPUT);
    Wire.begin(SDA_Pin, SCL_Pin);
    Serial.begin(9600);
    delay(1000);
    // Create the mutex semaphore
    Serial.println(status("Starting Blink Tasks..."));
    xTaskCreate(blinkLed1Task, "Blink LED 1", 10000, NULL, 3, NULL);
    xTaskCreate(blinkLed2Task, "Blink LED 2", 10000, NULL, 3, NULL);

    Serial.println(status("SHT31 Sensor initialized..."));
}

void loop() {}