#include <Adafruit_SHT31.h>
#include <Arduino.h>
#include <SPI.h>
#include <TM1637TinyDisplay6.h>
#include <Wire.h>

#include "blinkers.h"
#include "config.h"
#include "display.h"
#include "helpers.h"

enum Display { Temp, Humi };
struct SensorData {
    float temperature;
    float humidity;
};

// TODO: Remove global variables
int led1Count = 0;
int led2Count = 0;

SemaphoreHandle_t mutex;
Adafruit_SHT31 sht = Adafruit_SHT31();
TM1637TinyDisplay6 display(TM1637_CLK_Pin, TM1637_DIO_Pin);

// Queue handles
static const uint8_t queue_lenght = 10;
static QueueHandle_t buttonsQueue = NULL;
static QueueHandle_t sensorQueue = NULL;

// Task handlers
static TaskHandle_t blinkLed1TaskHandler = NULL;
static TaskHandle_t blinkLed2TaskHandler = NULL;

#pragma region Tasks
void blinkLed1Task(void *parameter) {
    for (;;) blink(LED1_Pin, LED2_High, LED2_Low, ++led1Count, mutex);
}

void blinkLed2Task(void *parameter) {
    for (;;) blink(LED2_Pin, LED1_High, LED2_Low, ++led2Count, mutex);
}

void activitiIndicationLedTask(void *parameter) {
    int button = 0;

    for (;;) {
        if (xQueueReceive(buttonsQueue, &button, portMAX_DELAY) == pdTRUE) {
            if (button == 1) {
                digitalWrite(LED3_Pin, HIGH);
            } else
                digitalWrite(LED3_Pin, LOW);
        }
    }
}

void buttonsTask(void *parameter) {
    int button = 0;

    for (;;) {
        button = digitalRead(BUTTON1_Pin) == HIGH ? 1 : 0;

        // Send the button value to the queue
        if (xQueueSend(buttonsQueue, &button, portMAX_DELAY) != pdTRUE) {
            if (xSemaphoreTake(mutex, (TickType_t)10) == pdTRUE) {
                Serial.println(error("Queue full"));
                xSemaphoreGive(mutex);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(BUTTONS_Refresh));
    }
}

void sensorTask(void *parameter) {
    SensorData data;

    for (;;) {
        data.temperature = sht.readTemperature();
        data.humidity = sht.readHumidity();

        // Println the data to the serial monitor
        if (xSemaphoreTake(mutex, (TickType_t)10) == pdTRUE) {
            Serial.print("Temperature: " + String(data.temperature) + "°C\t\t");
            Serial.println("Humidity: " + String(data.humidity) + "%");
            xSemaphoreGive(mutex);
        }

        if (isnan(data.humidity)) {
            if (xSemaphoreTake(mutex, (TickType_t)10) == pdTRUE) {
                Serial.println(error("Failed to read humidity"));
                xSemaphoreGive(mutex);
            }
        } else if (isnan(data.temperature)) {
            if (xSemaphoreTake(mutex, (TickType_t)10) == pdTRUE) {
                Serial.println(error("Failed to read temperature"));
                xSemaphoreGive(mutex);
            }
        } else if (xQueueOverwrite(sensorQueue, &data) != pdTRUE) {
            if (xSemaphoreTake(mutex, (TickType_t)10) == pdTRUE) {
                Serial.println(error("Sensor data not sent to queue"));
                xSemaphoreGive(mutex);
            }
        }
        xQueueOverwrite(sensorQueue, &data);

        vTaskDelay(pdMS_TO_TICKS(SHT31_Refresh));
    }
}

void displayTask(void *parameter) {
    Display mode = Display::Temp;
    uint8_t segments[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    SensorData data;
    char array[10];
    int button = 0;

    for (;;) {
        if (xQueueReceive(buttonsQueue, &button, portMAX_DELAY) == pdTRUE) {
            if (button == 1) {
                mode = mode == Display::Temp ? Display::Humi : Display::Temp;
            }
        }

        if (xQueueReceive(sensorQueue, &data, portMAX_DELAY) == pdTRUE) {
            if (mode == Display::Temp) {
                sprintf(array, "%f", data.temperature);
                Serial.printf("Array: %s\n", array);
                segments[0] = display.encodeDigit(array[0] - '0');
                segments[1] = display.encodeDigit(array[1] - '0') | 0x80;
                segments[2] = display.encodeDigit(array[3] - '0');
                segments[4] = display.encodeASCII('°');
                segments[5] = display.encodeASCII('C');
            } else {
                sprintf(array, "%f", data.humidity);
                Serial.printf("Array: %s\n", array);
                segments[4] = display.encodeASCII('%');
            }

            segments[0] = display.encodeDigit(array[0] - '0');
            segments[1] = display.encodeDigit(array[1] - '0') | 0x80;
            segments[2] = display.encodeDigit(array[3] - '0');
            display.setSegments(segments);
        }
    }

    vTaskDelay(pdMS_TO_TICKS(DISPLAY_Refresh));
}

#pragma endregion

void setup() {
    mutex = xSemaphoreCreateMutex();

    pinMode(LED1_Pin, OUTPUT);
    pinMode(LED2_Pin, OUTPUT);
    pinMode(LED3_Pin, OUTPUT);
    pinMode(BUTTON1_Pin, INPUT_PULLDOWN);
    // pinMode(BUTTON2_Pin, INPUT_PULLUP);
    // pinMode(BUTTON3_Pin, INPUT_PULLUP);
    Wire.begin(SDA_Pin, SCL_Pin);
    display.begin();
    display.setBrightness(DISPLAY_Brightness);
    Serial.begin(SERIAL_Baud);
    delay(1000);

    if (!sht.begin(SHT31_Address)) { // Set to 0x45 for alternate i2c addr
        Serial.println("Couldn't find SHT31");
        while (1) delay(1);
    }

    // Create the queues
    buttonsQueue = xQueueCreate(queue_lenght, sizeof(int));
    sensorQueue = xQueueCreate(1, sizeof(SensorData));

    if (sensorQueue == NULL) {
        if (xSemaphoreTake(mutex, (TickType_t)10) == pdTRUE) {
            Serial.println(error("Sensor queue creation failed"));
            xSemaphoreGive(mutex);
        }
    }

    // Create the tasks
    xTaskCreate(blinkLed1Task, "Blink LED 1", 10000, NULL, 0, &blinkLed1TaskHandler);
    xTaskCreate(blinkLed2Task, "Blink LED 2", 10000, NULL, 0, &blinkLed2TaskHandler);
    xTaskCreate(activitiIndicationLedTask, "Activity LED", 10000, NULL, 1, NULL);
    xTaskCreate(buttonsTask, "Buttons", 10000, NULL, 1, NULL);
    xTaskCreate(sensorTask, "Sensor", 10000, NULL, 1, NULL);
    xTaskCreate(displayTask, "Display", 10000, NULL, 1, NULL);
}

void loop() {}
