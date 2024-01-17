#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
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
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

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

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;)
            ; // Don't proceed, loop forever
    }

    Serial.println(status("Display initialized..."));
    display.clearDisplay();

    // Draw a single pixel in white
    display.drawPixel(10, 10, SSD1306_WHITE);
    display.display();
    delay(20000);
    showLogo();
    delay(2000);
    showText();
}

void loop() {}

#define LOGO_HEIGHT 16
#define LOGO_WIDTH 16
static const unsigned char PROGMEM logo_bmp[] = {
    0b00000000, 0b11000000, 0b00000001, 0b11000000, 0b00000001, 0b11000000, 0b00000011,
    0b11100000, 0b11110011, 0b11100000, 0b11111110, 0b11111000, 0b01111110, 0b11111111,
    0b00110011, 0b10011111, 0b00011111, 0b11111100, 0b00001101, 0b01110000, 0b00011011,
    0b10100000, 0b00111111, 0b11100000, 0b00111111, 0b11110000, 0b01111100, 0b11110000,
    0b01110000, 0b01110000, 0b00000000, 0b00110000};

void showLogo() {
    display.clearDisplay();

    display.drawBitmap((display.width() - LOGO_WIDTH) / 2,
                       (display.height() - LOGO_HEIGHT) / 2, logo_bmp, LOGO_WIDTH,
                       LOGO_HEIGHT, 1);
    display.display();
    delay(1000);
}

void showText() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Hello, world!");
    display.display();
    delay(2000);
}
