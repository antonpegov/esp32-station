#include <Arduino.h>

#define led1HighTimeMS 1000
#define led1LowTimeMS 5000
#define led2HighTimeMS 1500
#define led2LowTimeMS 3000

int outPin1Count = 0;
int outPin2Count = 0;
int outPin1 = 1; // External LED1
int outPin2 = 2; // External LED2

void blink(int pin, u_int highTime, u_int lowTime, int count);
void blinkLed1Task(void *parameter);
void blinkLed2Task(void *parameter);
SemaphoreHandle_t mutex;

#pragma region Tasks
void blinkLed1Task(void *parameter)
{
  for (;;)
  {
    blink(outPin1, led1HighTimeMS, led1LowTimeMS, ++outPin1Count);
  }
}

void blinkLed2Task(void *parameter)
{
  for (;;)
  {
    blink(outPin2, led2HighTimeMS, led2LowTimeMS, ++outPin2Count);
  }
}
#pragma endregion

void setup()
{
  // Create the mutex semaphore
  mutex = xSemaphoreCreateMutex();
  Serial.begin(9600);
  pinMode(outPin1, OUTPUT);
  pinMode(outPin2, OUTPUT);
  Serial.println("Starting Blink Tasks...");
  xTaskCreate(blinkLed1Task, "Blink LED 1", 10000, NULL, 3, NULL);
  xTaskCreate(blinkLed2Task, "Blink LED 2", 10000, NULL, 3, NULL);
}

void loop() {}

void blink(int pin, u_int highTime, u_int lowTime, int count)
{
  // Only print if we can take the mutex
  if (xSemaphoreTake(mutex, (TickType_t)10) == pdTRUE)
  {
    Serial.println("LED" + String(pin) + " blinked " + String(count) + " times...");
    xSemaphoreGive(mutex);
  }

  digitalWrite(pin, HIGH);
  vTaskDelay(pdMS_TO_TICKS(highTime));
  digitalWrite(pin, LOW);
  vTaskDelay(pdMS_TO_TICKS(lowTime));
}