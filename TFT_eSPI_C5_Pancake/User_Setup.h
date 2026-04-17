// ============================================================
// TFT_eSPI User Setup for Pancake (ESP32-C5 + ILI9341)
//
// HOW TO USE:
//   Copy this file's contents into your TFT_eSPI library folder,
//   replacing (or backing up) the existing User_Setup.h.
//   Library folder is usually:
//     ~/Documents/Arduino/libraries/TFT_eSPI/User_Setup.h
//
// The Pancake display is a 320x480 ILI9341 panel mounted in
// landscape orientation via swap_xy + mirror = 480x320.
// TFT_eSPI setRotation(3) or (1) will give you 480x320.
// ============================================================

#define USER_SETUP_INFO "Pancake ESP32-C5 ILI9341"

// Driver
#define ILI9341_DRIVER

// Some ILI9341 panels ship with inverted color polarity.
// This sends the INVON command at init to correct white→black inversion.
#define TFT_INVERSION_ON

// Physical panel dimensions (portrait native)
#define TFT_WIDTH   320
#define TFT_HEIGHT  480

// SPI pins - matching pancake main.c exactly
#define TFT_MOSI    24
#define TFT_MISO     4
#define TFT_SCLK    23
#define TFT_CS       5
#define TFT_DC       3
#define TFT_RST      2

// No backlight GPIO (LED tied to 3V3 on Pancake)
// #define TFT_BL  -1

// SPI clock
// FIX (noise): Reduced write clock from 20 MHz to 10 MHz.
// The display and SD card share MOSI/MISO/CLK on the ESP32-C5.
// At 20 MHz the ILI9341 reads back its own MISO line during write-
// only transactions, and any glitch from the SD card (even with its
// CS held high) can corrupt data.  10 MHz gives enough margin.
// If you need maximum speed and your layout is clean, try 16000000.
#define SPI_FREQUENCY       15000000
#define SPI_READ_FREQUENCY  15000000
#define SPI_TOUCH_FREQUENCY  2500000

// Fonts to include - add more as needed
#define LOAD_GLCD    // Font 1
#define LOAD_FONT2   // Font 2
#define LOAD_FONT4   // Font 4
#define LOAD_FONT6   // Font 6 (digits only)
#define LOAD_FONT7   // Font 7 (digits only)
#define LOAD_FONT8   // Font 8 (digits only)
#define LOAD_GFXFF   // FreeFonts

#define SMOOTH_FONT
