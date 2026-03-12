//BlackJack-OS for MESHCORE created by Robert Proaps
#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <RadioLib.h>
#include "utilities.h"
#include "BJOS_SplashscreenBGR565.h"
#include "Cursor.h"


//ST7789 LCD Controller SPI Command Table
typedef struct {
  uint8_t cmd;
  uint8_t data[14];
  uint8_t len;
} lcd_cmd_t;

lcd_cmd_t lcd_st7789v[] = {
  { 0x01, { 0 }, 0 | 0x80 },
  { 0x11, { 0 }, 0 | 0x80 },
  { 0x3A, { 0X05 }, 1 },
  { 0x36, { 0x55 }, 1 },
  { 0xB2, { 0x0C, 0x0C, 0X00, 0X33, 0X33 }, 5 },
  { 0xB7, { 0X75 }, 1 },
  { 0xBB, { 0X1A }, 1 },
  { 0xC0, { 0X2C }, 1 },
  { 0xC2, { 0X01 }, 1 },
  { 0xC3, { 0X13 }, 1 },
  { 0xC4, { 0X20 }, 1 },
  { 0xC6, { 0X0F }, 1 },
  { 0xD0, { 0XA4, 0XA1 }, 2 },
  { 0xD6, { 0XA1 }, 1 },
  { 0xE0, { 0XD0, 0X0D, 0X14, 0X0D, 0X0D, 0X09, 0X38, 0X44, 0X4E, 0X3A, 0X17, 0X18, 0X2F, 0X30 }, 14 },
  { 0xE1, { 0XD0, 0X09, 0X0F, 0X08, 0X07, 0X14, 0X37, 0X44, 0X4D, 0X38, 0X15, 0X16, 0X2C, 0X3E }, 14 },
  { 0x21, { 0 }, 0 },  //invertDisplay
  { 0x29, { 0 }, 0 },
  { 0x2C, { 0 }, 0 },
};

TFT_eSPI tft;  //TFT LCD Object

//Lora and Meshcore Info for Radio Setup
struct LoRa_Config {
    float       frequency;      //MHz
    float       bandwidth;      //kHz
    uint8_t     sf;             //Spreading Factor
    uint8_t     cr;             //Coding Rate Denominator (4/cr)
    uint8_t     syncWord;       
    int8_t      txPower;        //dBm
    uint16_t    preambleLen;
};

//MeshCore US/Canada Preset
const LoRa_Config MESHCORE_US = {
    .frequency      =   910.525,
    .bandwidth      =   62.5,
    .sf             =   7,
    .cr             =   5,
    .syncWord       =   0x2B,
    .txPower        =   17,
    .preambleLen    =   8
};

// LoRa Radio Object
// IMPORTANT: RadioLib must use the HSPI bus via an explicit SPIClass instance (hspi).
// TFT_eSPI claims HSPI internally via USE_HSPI_PORT in User_Setup.h.
// Passing the default SPI object here will target FSPI, putting the radio on a different
// peripheral than the display despite sharing the same physical pins, causing init failure.
// Both devices must reference the same SPIClass instance (hspi) to share the bus correctly.

SPIClass hspi(HSPI);
SX1262 LoRaRadio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN, hspi, SPISettings(2000000, MSBFIRST, SPI_MODE0)); //LoRa Radio Objuect

//Cursor Object
Cursor cursor(tft, TFT_WHITE, TFT_BLACK);

void setup() {
  Serial.begin(115200); //Open A Serial Port
  systemStartup();  //Initialize the System
}

void loop() {
  // put your main code here, to run repeatedly:
  cursor.update();
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
  hspi.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI);

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
  Serial.println("TFT begun");

  pinMode(BOARD_BL_PIN, OUTPUT);  //Turn on the LCD backlight at brightness 8.
  setBrightness(8);

  //Show Splashscreen
  delay(500);
  tft.pushImage(0, 0, SPLASH_WIDTH, SPLASH_HEIGHT, (const uint16_t*)BJOS_SplashscreenBGR_img);

  // Reassert SPI CS Lines
  digitalWrite(BOARD_SDCARD_CS, HIGH);
  digitalWrite(RADIO_CS_PIN, HIGH);
  digitalWrite(BOARD_TFT_CS, HIGH);

  //Setup LoRa Radio
  int radioState = LoRaRadio.begin(MESHCORE_US.frequency, MESHCORE_US.bandwidth, MESHCORE_US.sf, MESHCORE_US.cr, MESHCORE_US.syncWord, MESHCORE_US.txPower, MESHCORE_US.preambleLen);
  if (radioState != RADIOLIB_ERR_NONE) {
    Serial.print("Radio initialization failure: ");
    Serial.println(radioState);
    Serial.println("Halting");
    while(true);
  }

  Serial.println("Radio Initialized.");
  
  //Initialize the Cursor Routine
  Serial.println("Initializing Cursor Subsystem.");
  cursor.begin();
  cursor.draw();

  Serial.println("Startup Complete!");
}

// LCD Backlight Control
// LilyGo  T-Deck  control backlight chip has 16 levels of adjustment range
// The adjustable range is 0~15, 0 is the minimum brightness, 16 is the maximum brightness
void setBrightness(uint8_t value) {
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