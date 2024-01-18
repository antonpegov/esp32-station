#include <Adafruit_SHT31.h>
#include <Arduino.h>
#include <SPI.h>
#include <TM1637TinyDisplay6.h>
#include <Wire.h>

#include "blinkers.h"
#include "display.h"
#include "helpers.h"

#define BUTTON1_Pin 35
#define BUTTON2_Pin 36
#define BUTTON3_Pin 37
#define LED1_Pin 20
#define LED1_High 1000
#define LED1_Low 5000
#define LED2_Pin 21
#define LED2_High 1500
#define LED2_Low 3000
#define DISPLAY_Refresh 3000
#define DISPLAY_Brightness 7

#define SERIAL_Baud 9600
#define TM1637_DIO_Pin 1
#define TM1637_CLK_Pin 2
#define SDA_Pin 15
#define SCL_Pin 16
#define SHT31_Address 0x44
// Define enum for all display modes
enum DisplayMode { Temperature, Humidity, Time };

int led1Count = 0;
int led2Count = 0;

SemaphoreHandle_t mutex;
Adafruit_SHT31 sht = Adafruit_SHT31();
TM1637TinyDisplay6 display(TM1637_CLK_Pin, TM1637_DIO_Pin);

void blinkLed1Task(void *parameter);
void blinkLed2Task(void *parameter);

#pragma region Tasks
void blinkLed1Task(void *parameter) {
    for (;;) blink(LED1_Pin, LED2_High, LED2_Low, ++led1Count, mutex);
}

void blinkLed2Task(void *parameter) {
    for (;;) blink(LED2_Pin, LED1_High, LED2_Low, ++led2Count, mutex);
}
#pragma endregion

void setup() {
    mutex = xSemaphoreCreateMutex();
    pinMode(LED1_Pin, OUTPUT);
    pinMode(LED2_Pin, OUTPUT);
    pinMode(BUTTON1_Pin, INPUT_PULLUP);
    pinMode(BUTTON2_Pin, INPUT_PULLUP);
    pinMode(BUTTON3_Pin, INPUT_PULLUP);
    Wire.begin(SDA_Pin, SCL_Pin);
    display.begin();
    display.setBrightness(DISPLAY_Brightness);
    Serial.begin(SERIAL_Baud);
    delay(1000);

    Serial.println("SHT31 test");
    if (!sht.begin(SHT31_Address)) { // Set to 0x45 for alternate i2c addr
        Serial.println("Couldn't find SHT31");
        while (1) delay(1);
    }

    Serial.print("Heater Enabled State: ");
    if (sht.isHeaterEnabled())
        Serial.println("ENABLED");
    else
        Serial.println("DISABLED");

    Serial.println(status("Starting Blink Tasks..."));
    xTaskCreate(blinkLed1Task, "Blink LED 1", 10000, NULL, 3, NULL);
    xTaskCreate(blinkLed2Task, "Blink LED 2", 10000, NULL, 3, NULL);
}

void loop() {
    float t = sht.readTemperature();
    float h = sht.readHumidity();
    char tempStr[10];

    if (!isnan(t)) { // check if 'is not a number'
        Serial.print("Temp *C = ");
        Serial.print(t);
        Serial.print("\t\t");
    } else {
        Serial.println("Failed to read temperature");
    }

    if (!isnan(h)) { // check if 'is not a number'
        Serial.print("Hum. % = ");
        Serial.println(h);
    } else {
        Serial.println("Failed to read humidity");
    }

    uint8_t data[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    char array[10];
    sprintf(array, "%f", t);
    Serial.printf("Array: %s\n", array);
    data[0] = display.encodeDigit(array[0] - '0');
    data[1] = display.encodeDigit(array[1] - '0') | 0x80;
    data[2] = display.encodeDigit(array[3] - '0');
    data[5] = display.encodeASCII(' ');
    data[4] = display.encodeASCII('°');
    data[5] = display.encodeASCII('C');
    display.setSegments(data);
    delay(DISPLAY_Refresh);
}