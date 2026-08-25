//                            USER DEFINED SETTINGS
//   Set driver type, fonts to be loaded, pins used and SPI control method etc
//
//   Marauder V8 — ESP32-C5 + ILI9341 240x320 display + XPT2046 touch
//
//   Confirmed pins (voltmeter + brute-force scan, April 2026):
//     TFT_MOSI  = GPIO7   FSPI MOSI (shared with SD + touch)
//     TFT_MISO  = GPIO2   FSPI MISO (shared with SD + touch)
//     TFT_SCLK  = GPIO6   FSPI SCLK (shared with SD + touch)
//     TFT_CS    = GPIO23
//     TFT_DC    = GPIO24
//     TFT_RST   = -1      Hardware tied to EN, no GPIO control needed
//     TFT_BL    = GPIO8   PWM backlight
//     TOUCH_CS  = GPIO3   XPT2046
//     SD_CS     = GPIO10


// ##################################################################################
//
// Section 1. Driver and display options
//
// ##################################################################################

#define ILI9341_DRIVER

// V8 panel uses RGB colour order — without this define RED displays as BLUE.
// TFT_BGR tells TFT_eSPI to pre-swap R/B bytes before sending, which cancels
// out the ILI9341's default MADCTL BGR=1 setting and gives correct colours.
#define TFT_RGB_ORDER TFT_BGR


// ##################################################################################
//
// Section 2. Pin definitions
//
// ##################################################################################

// FSPI bus — shared by TFT, SD card, and XPT2046 touch controller
#define TFT_MISO  2
#define TFT_MOSI  7
#define TFT_SCLK  6

// Display control
#define TFT_CS   23   // Chip select
#define TFT_DC   24   // Data / Command
#define TFT_RST  -1   // Reset tied to EN via hardware circuit — no GPIO needed

// Backlight — Display::init() calls digitalWrite(TFT_BL, HIGH) directly.
// Defining it here lets TFT_eSPI set the pin mode during tft.init().
#define TFT_BL    8
#define TFT_BACKLIGHT_ON HIGH

// XPT2046 resistive touch controller
#define TOUCH_CS  3


// ##################################################################################
//
// Section 3. Fonts
//
// ##################################################################################

#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font
#define LOAD_FONT2  // Font 2. Small 16 pixel high font
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font
//#define LOAD_FONT6  // Font 6. Large 48 pixel font (digits + punctuation)
//#define LOAD_FONT7  // Font 7. 7-segment 48 pixel font (digits + punctuation)
//#define LOAD_FONT8  // Font 8. Large 75 pixel font (digits + punctuation)
#define LOAD_GFXFF  // FreeFonts — 48 Adafruit_GFX free fonts FF1–FF48

#define SMOOTH_FONT


// ##################################################################################
//
// Section 4. SPI speed
//
// ##################################################################################

// 20 MHz is conservative and reliable for the ESP32-C5 FSPI peripheral.
// The ILI9341 supports up to 60 MHz writes in practice, but shared bus
// noise from the SD card can cause issues at higher speeds.
#define SPI_FREQUENCY       20000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000   // XPT2046 max is 2.5 MHz
