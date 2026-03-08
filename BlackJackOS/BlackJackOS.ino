//BlackJack-OS for MESHCORE created by Robert Proaps
#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include "utilities.h"
#include "BJOS_Splashscreen.h"

//ST7789 LCD Controller SPI Command Table
typedef struct {
    uint8_t cmd;
    uint8_t data[14];
    uint8_t len;
} lcd_cmd_t;

lcd_cmd_t lcd_st7789v[] = {
    {0x01, {0}, 0 | 0x80},
    {0x11, {0}, 0 | 0x80},
    {0x3A, {0X05}, 1},
    {0x36, {0x55}, 1},
    {0xB2, {0x0C, 0x0C, 0X00, 0X33, 0X33}, 5},
    {0xB7, {0X75}, 1},
    {0xBB, {0X1A}, 1},
    {0xC0, {0X2C}, 1},
    {0xC2, {0X01}, 1},
    {0xC3, {0X13}, 1},
    {0xC4, {0X20}, 1},
    {0xC6, {0X0F}, 1},
    {0xD0, {0XA4, 0XA1}, 2},
    {0xD6, {0XA1}, 1},
    {0xE0, {0XD0, 0X0D, 0X14, 0X0D, 0X0D, 0X09, 0X38, 0X44, 0X4E, 0X3A, 0X17, 0X18, 0X2F, 0X30}, 14},
    {0xE1, {0XD0, 0X09, 0X0F, 0X08, 0X07, 0X14, 0X37, 0X44, 0X4D, 0X38, 0X15, 0X16, 0X2C, 0X3E}, 14},
    {0x21, {0}, 0}, //invertDisplay
    {0x29, {0}, 0},
    {0x2C, {0}, 0},
};

TFT_eSPI tft; //TFT LCD Object

void setup() {
  Serial.begin(115200);
  delay(100);
  systemStartup();



}

void loop() {
  // put your main code here, to run repeatedly:

}

//Functions

//Startup Procedure
void systemStartup() {
  //This function should run once in the setup function.
  Serial.println("System Starting Up");
  //Turn on the power rails to all board perephreals, including LCD, Touchscreen, SD Card, LoRa, and Trackball.
  Serial.println("Powering On-Board Periphreals");
  pinMode(BOARD_POWERON, OUTPUT);
  digitalWrite(BOARD_POWERON, HIGH);

  //Disable all SPI devices during startup (Active LOW).
  Serial.println("Disabling SPI Devices.");
  pinMode(BOARD_SDCARD_CS, OUTPUT);
  pinMode(RADIO_CS_PIN, OUTPUT);
  pinMode(BOARD_TFT_CS, OUTPUT);

  digitalWrite(BOARD_SDCARD_CS, HIGH);
  digitalWrite(RADIO_CS_PIN, HIGH);
  digitalWrite(BOARD_TFT_CS, HIGH);

  //Define SPI Bus.
  Serial.println("Defining SPI Bus.");
  pinMode(BOARD_SPI_MISO, INPUT_PULLUP);
  SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI);

  //Initialize the screen and turn on the backlight.
  Serial.println("Screen Initializing.");
  tft.begin();

  #if 0
    for (uint8_t i = 0; i < (sizeof(lcd_st7789v) / sizeof(lcd_cmd_t)); i++) {
        tft.writecommand(lcd_st7789v[i].cmd);
        for (int j = 0; j < (lcd_st7789v[i].len & 0x7f); j++) {
            tft.writedata(lcd_st7789v[i].data[j]);
        }

        if (lcd_st7789v[i].len & 0x80) {
            delay(120);
        }
    }
  #endif

  tft.setRotation(1);
  pinMode(BOARD_BL_PIN, OUTPUT);
  setBrightness(8);

  Serial.println("Startup Complete!");
  //Show Splashscreen
  tft.fillScreen(TFT_RED);
  delay(100);
  tft.pushImage(0, 0, SPLASH_WIDTH, SPLASH_HEIGHT, (const uint16_t*)BJOS_Splashscreen_img);
}

// LCD Backlight Control
// LilyGo  T-Deck  control backlight chip has 16 levels of adjustment range
// The adjustable range is 0~15, 0 is the minimum brightness, 16 is the maximum brightness
void setBrightness(uint8_t value)
{
    static uint8_t level = 0;
    static uint8_t steps = 16;
    if (value == 0) {
        digitalWrite(BOARD_BL_PIN, 0);
        delay(3);
        level = 0;
        return;
    }
    if (level == 0) {
        digitalWrite(BOARD_BL_PIN, 1);
        level = steps;
        delayMicroseconds(30);
    }
    int from = steps - level;
    int to = steps - value;
    int num = (steps + to - from) % steps;
    for (int i = 0; i < num; i++) {
        digitalWrite(BOARD_BL_PIN, 0);
        digitalWrite(BOARD_BL_PIN, 1);
    }
    level = value;
}