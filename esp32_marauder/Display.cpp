#include "Display.h"
#include "lang_var.h"

#ifdef HAS_SCREEN

Display::Display()
#ifdef HAS_CYD_TOUCH
  : touchscreenSPI(VSPI),
    touchscreen(XPT2046_CS, XPT2046_IRQ)
#endif
{
}

int8_t Display::menuButton(uint16_t *x, uint16_t *y, bool pressed, bool check_hold) {
  #ifdef HAS_ILI9341
    for (uint8_t b = BUTTON_ARRAY_LEN; b < BUTTON_ARRAY_LEN + 3; b++) {
      if (pressed && this->key[b].contains(*x, *y)) {
        this->key[b].press(true);  // tell the button it is pressed
      } else {
        this->key[b].press(false);  // tell the button it is NOT pressed
      }
    }

    for (uint8_t b = BUTTON_ARRAY_LEN; b < BUTTON_ARRAY_LEN + 3; b++) {
      if (!check_hold) {
        if ((this->key[b].justReleased()) && (!pressed)) {
          return b - BUTTON_ARRAY_LEN;
        }
      }
      else {
        if ((this->key[b].isPressed())) {
          return b - BUTTON_ARRAY_LEN;
        }
      }
    }

  #endif

  return -1;
}

uint8_t Display::updateTouch(uint16_t *x, uint16_t *y, uint16_t threshold) {
  #ifdef HAS_ILI9341
    if (!this->headless_mode) {
      #if defined(HAS_CAP_TOUCH)
        {
          uint16_t raw_x, raw_y;
          if (!ft6336_read_raw(&raw_x, &raw_y)) return 0;

          uint8_t rot = this->tft.getRotation();
          if (rot == 3) {
            // Landscape-flipped (packet monitor): tft.width()=TFT_HEIGHT=480, tft.height()=TFT_WIDTH=320
            // raw_y → screen_x (inverted): TFT_HEIGHT-1 - raw_y
            // raw_x → screen_y
            *x = (raw_y < TFT_HEIGHT) ? (uint16_t)(TFT_HEIGHT - 1 - raw_y) : 0;
            *y = (raw_x < TFT_WIDTH)  ? raw_x : (uint16_t)(TFT_WIDTH  - 1);
          } else {
            // Portrait (rotation 0): direct mapping
            *x = (raw_x < TFT_WIDTH)  ? raw_x : (uint16_t)(TFT_WIDTH  - 1);
            *y = (raw_y < TFT_HEIGHT) ? raw_y : (uint16_t)(TFT_HEIGHT - 1);
          }
          return 1;
        }

      #elif defined(HAS_CYD_TOUCH)
        if (this->touchscreen.tirqTouched() && this->touchscreen.touched()) {
          TS_Point p = this->touchscreen.getPoint();

          uint8_t rot = this->tft.getRotation();

          switch (rot) {
            case 0: // Standard Portrait
              *x = map(p.x, 200, 3700, 1, TFT_WIDTH);
              *y = map(p.y, 240, 3800, 1, TFT_HEIGHT);
              break;
            case 1:
              *x = map(p.y, 143, 3715, 0, TFT_HEIGHT);
              *y = map(p.x, 3786, 216, 0, TFT_WIDTH);
              break;
            case 2:
              *x = map(p.x, 3700, 200, 1, TFT_WIDTH);
              *y = map(p.y, 3800, 240, 1, TFT_HEIGHT);
              break;
            case 3:
              *x = map(p.y, 3800, 240, 1, TFT_WIDTH);
              *y = map(p.x, 200, 3700, 1, TFT_HEIGHT);
              break;
          }
          return 1;
        }
        else
          return 0;

      #else
        return this->tft.getTouch(x, y, threshold);
      #endif
    }
    else
      return !this->headless_mode;
  #endif

  return 0;
}

bool Display::isTouchHeld(uint16_t threshold) {
  static unsigned long touchStartTime = 0;
  static bool touchHeld = false;
  uint16_t x, y;

  if (this->updateTouch(&x, &y, threshold)) {
    // Touch detected
    if (touchStartTime == 0) {
      touchStartTime = millis();  // First touch timestamp
    } else if (!touchHeld && millis() - touchStartTime >= 1000) {
      touchHeld = true;  // Held for at least 1000ms
      return true;
    }
  } else {
    // Touch released
    touchStartTime = 0;
    touchHeld = false;
  }

  return false;
}

void Display::init() {
  tft.init();

  #if defined(HAS_DUAL_BAND) && !defined(MARAUDER_MINI_V3) && defined(TFT_BL)
    digitalWrite(TFT_BL, HIGH);
  #endif
}

void Display::setCalData(bool landscape) {
  #if !defined(HAS_CAP_TOUCH)  // cap touch targets use ft6336_init() instead
    #ifndef HAS_CYD_TOUCH
    if (!landscape) {
      #ifdef TFT_SHIELD
        uint16_t calData[5] = { 275, 3494, 361, 3528, 4 }; // tft.setRotation(0); // Portrait with TFT Shield
      #elif defined(MARAUDER_CYD_3_5_INCH)
        uint16_t calData[5] = { 239, 3560, 262, 3643, 4 };
      #elif defined(MARAUDER_V8)
        //uint16_t calData[5] = { 351, 3279, 214, 3394, 2 };
        uint16_t calData[5] = { 312, 3431, 191, 3456, 2 };
      #elif defined(TFT_DIY)
        uint16_t calData[5] = { 339, 3470, 237, 3438, 2 }; // tft.setRotation(0); // Portrait with DIY TFT
      #endif
      #ifdef HAS_ILI9341
        tft.setTouch(calData);
      #endif
    }
    else {
      #ifdef TFT_SHIELD
        uint16_t calData[5] = { 391, 3491, 266, 3505, 7 }; // Landscape TFT Shield
      #elif defined(MARAUDER_CYD_3_5_INCH)
        uint16_t calData[5] = { 272, 3648, 234, 3565, 7 };
      #elif defined(MARAUDER_V8)
        uint16_t calData[5] = { 213, 3396, 350, 3275, 1 };
      #else if defined(TFT_DIY)
        uint16_t calData[5] = { 213, 3469, 320, 3446, 1 }; // Landscape TFT DIY
      #endif
      #ifdef HAS_ILI9341
        tft.setTouch(calData);
      #endif
    }
    #endif
  #endif // !HAS_CAP_TOUCH
}

// Function to prepare the display and the menus
void Display::RunSetup() {
  run_setup = false;

  // Need to declare new
  display_buffer = new LinkedList<String>();

  #ifdef SCREEN_BUFFER
    screen_buffer = new LinkedList<String>();
  #endif

  #ifdef HAS_CYD_TOUCH
    this->touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    this->touchscreen.begin(touchscreenSPI);
    this->touchscreen.setRotation(0);
  #endif
  
  tft.init();

  tft.setRotation(SCREEN_ORIENTATION);

  tft.setCursor(0, 0);

  #ifdef HAS_ILI9341
    #if !defined(HAS_CYD_TOUCH) && !defined(HAS_CAP_TOUCH)
      this->setCalData();
    #elif defined(HAS_CAP_TOUCH)
      ft6336_init();   // also calls Wire.begin
    #endif
  #endif

  clearScreen();

  #ifdef KIT
    pinMode(KIT_LED_BUILTIN, OUTPUT);
  #endif

  #ifdef MARAUDER_REV_FEATHER
    pinMode(7, OUTPUT);

    delay(10);

    digitalWrite(7, HIGH);
  #endif
}

void Display::drawFrame()
{
  tft.drawRect(FRAME_X, FRAME_Y, FRAME_W, FRAME_H, TFT_BLACK);
}

void Display::tftDrawRedOnOffButton() {
  tft.fillRect(REDBUTTON_X, REDBUTTON_Y, REDBUTTON_W, REDBUTTON_H, TFT_RED);
  tft.fillRect(GREENBUTTON_X, GREENBUTTON_Y, GREENBUTTON_W, GREENBUTTON_H, TFT_DARKGREY);
  drawFrame();
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(text03, GREENBUTTON_X + (GREENBUTTON_W / 2), GREENBUTTON_Y + (GREENBUTTON_H / 2));
  this->SwitchOn = false;
}

void Display::tftDrawGreenOnOffButton() {
  tft.fillRect(GREENBUTTON_X, GREENBUTTON_Y, GREENBUTTON_W, GREENBUTTON_H, TFT_GREEN);
  tft.fillRect(REDBUTTON_X, REDBUTTON_Y, REDBUTTON_W, REDBUTTON_H, TFT_DARKGREY);
  drawFrame();
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(text04, REDBUTTON_X + (REDBUTTON_W / 2) + 1, REDBUTTON_Y + (REDBUTTON_H / 2));
  this->SwitchOn = true;
}

void Display::tftDrawGraphObjects(byte x_scale)
{
  // Graph objects — uses HEIGHT_1 (landscape height) and WIDTH_1 (landscape width)
  // so this works correctly for all display sizes including 320x480 (Pancake).
  tft.fillRect(11, 5, x_scale+1, HEIGHT_1/2, TFT_BLACK); // positive half
  tft.fillRect(11, HEIGHT_1/2+1, x_scale+1, HEIGHT_1/2-1, TFT_BLACK); // negative half
  tft.drawFastVLine(10, 5, HEIGHT_1-10, TFT_WHITE); // y axis
  tft.drawFastHLine(10, HEIGHT_1-1, WIDTH_1-10, TFT_WHITE); // x axis
  tft.setTextColor(TFT_YELLOW); tft.setTextSize(1);
  tft.setCursor(3, 6); tft.print("+"); // top of y axis
  tft.setCursor(3, HEIGHT_1-12); tft.print("0"); // bottom of y axis
}

void Display::tftDrawEapolColorKey(bool filter)
{
  //Display color key
  tft.setTextSize(1); tft.setTextColor(TFT_WHITE);
  tft.fillRect(14, 0, 15, 8, TFT_CYAN); tft.setCursor(30, 0); tft.println(" - EAPOL"); 
  if (filter) {
    uint16_t y = tft.getCursorY();
    tft.setCursor(14, y);
    tft.println("Filter Active");
  }
}

void Display::tftDrawColorKey()
{
  //Display color key
  tft.setTextSize(1); tft.setTextColor(TFT_WHITE);
  tft.fillRect(14, 0, 15, 8, TFT_GREEN); tft.setCursor(30, 0); tft.print(" - Beacons"); 
  tft.fillRect(14, 8, 15, 8, TFT_RED); tft.setCursor(30, 8); tft.print(" - Deauths");
  tft.fillRect(14, 16, 15, 8, TFT_BLUE); tft.setCursor(30, 16); tft.print(" - Probes");
}

void Display::tftDrawXScaleButtons(byte x_scale) {
  tft.drawFastVLine(234, 0, 20, TFT_WHITE);
  tft.setCursor(208, 21); tft.setTextColor(TFT_WHITE); tft.setTextSize(1); tft.print("X Scale:"); tft.print(x_scale);

  key[X_MINUS_INDEX].initButton(&tft, // x - box
                        220,
                        10,
                        20,
                        20,
                        TFT_BLACK,
                        TFT_CYAN,
                        TFT_BLACK,
                        "-",
                        2);
  key[X_PLUS_INDEX].initButton(&tft, // x + box
                        249,
                        10,
                        20,
                        20,
                        TFT_BLACK,
                        TFT_CYAN,
                        TFT_BLACK,
                        "+",
                        2);

  key[X_PLUS_INDEX].setLabelDatum(1, 5, MC_DATUM);
  key[X_MINUS_INDEX].setLabelDatum(1, 5, MC_DATUM);

  key[X_PLUS_INDEX].drawButton();
  key[X_MINUS_INDEX].drawButton();
}

void Display::tftDrawYScaleButtons(byte y_scale)
{
  tft.drawFastVLine(290, 0, 20, TFT_WHITE);
  tft.setCursor(265, 21); tft.setTextColor(TFT_WHITE); tft.setTextSize(1); tft.print("Y Scale:"); tft.print(y_scale);

  key[Y_MINUS_INDEX].initButton(&tft, // y - box
                        276,
                        10,
                        20,
                        20,
                        TFT_BLACK,
                        TFT_MAGENTA,
                        TFT_BLACK,
                        "-",
                        2);
  key[Y_PLUS_INDEX].initButton(&tft, // y + box
                        305,
                        10,
                        20,
                        20,
                        TFT_BLACK,
                        TFT_MAGENTA,
                        TFT_BLACK,
                        "+",
                        2);

  key[Y_MINUS_INDEX].setLabelDatum(1, 5, MC_DATUM);
  key[Y_PLUS_INDEX].setLabelDatum(1, 5, MC_DATUM);

  key[Y_MINUS_INDEX].drawButton();
  key[Y_PLUS_INDEX].drawButton();
}

void Display::tftDrawChannelScaleButtons(int set_channel, bool lnd_an) {
  if (lnd_an) {
    tft.drawFastVLine(178, 0, 20, TFT_WHITE);
    tft.setCursor(145, 21); tft.setTextColor(TFT_WHITE); tft.setTextSize(1); tft.print(text10); tft.print(set_channel);

    key[CHAN_MINUS_INDEX].initButton(&tft, // channel - box
                          164,
                          10,
                          EXT_BUTTON_WIDTH - (EXT_BUTTON_WIDTH * 0.33),
                          EXT_BUTTON_WIDTH - (EXT_BUTTON_WIDTH * 0.33),
                          TFT_BLACK,
                          TFT_BLUE,
                          TFT_BLACK,
                          "-",
                          2);
    key[CHAN_PLUS_INDEX].initButton(&tft, // channel + box
                          193,
                          10,
                          EXT_BUTTON_WIDTH - (EXT_BUTTON_WIDTH * 0.33),
                          EXT_BUTTON_WIDTH - (EXT_BUTTON_WIDTH * 0.33),
                          TFT_BLACK,
                          TFT_BLUE,
                          TFT_BLACK,
                          "+",
                          2);
  }

  else {
    key[CHAN_MINUS_INDEX].initButton(&tft, // channel - box
                          (EXT_BUTTON_WIDTH / 2) * 6,
                          (STATUS_BAR_WIDTH * 2) + (EXT_BUTTON_WIDTH / 2), // x, y, w, h, outline, fill, text
                          EXT_BUTTON_WIDTH,
                          EXT_BUTTON_WIDTH,
                          TFT_BLACK, // Outline
                          TFT_BLUE, // Fill
                          TFT_BLACK, // Text
                          "-",
                          1);
    key[CHAN_PLUS_INDEX].initButton(&tft, // channel + box
                          (EXT_BUTTON_WIDTH / 2) * 10,
                          (STATUS_BAR_WIDTH * 2) + (EXT_BUTTON_WIDTH / 2), // x, y, w, h, outline, fill, text
                          EXT_BUTTON_WIDTH,
                          EXT_BUTTON_WIDTH,
                          TFT_BLACK, // Outline
                          TFT_BLUE, // Fill
                          TFT_BLACK, // Text
                          "+",
                          1);
  }

  key[CHAN_MINUS_INDEX].setLabelDatum(1, 5, MC_DATUM);
  key[CHAN_PLUS_INDEX].setLabelDatum(1, 5, MC_DATUM);

  key[CHAN_MINUS_INDEX].drawButton();
  key[CHAN_PLUS_INDEX].drawButton();
}

void Display::tftDrawChanHopButton(bool lnd_an, bool en) {
  if (lnd_an) {
    if (!en) {
      key[CHAN_HOP_INDEX].initButton(&tft, // Exit box
                        137,
                        10, // x, y, w, h, outline, fill, text
                        EXT_BUTTON_WIDTH,
                        EXT_BUTTON_WIDTH,
                        TFT_ORANGE, // Outline
                        TFT_RED, // Fill
                        TFT_BLACK, // Text
                        "X",
                        2);
    } else {
      key[CHAN_HOP_INDEX].initButton(&tft, // Exit box
                        137,
                        10, // x, y, w, h, outline, fill, text
                        EXT_BUTTON_WIDTH,
                        EXT_BUTTON_WIDTH,
                        TFT_WHITE, // Outline
                        TFT_GREEN, // Fill
                        TFT_BLACK, // Text
                        "O",
                        2);
    }
  }

  else {
    if (!en) {
      key[CHAN_HOP_INDEX].initButton(&tft, // Exit box
                        (EXT_BUTTON_WIDTH / 2) * 14,
                        (STATUS_BAR_WIDTH * 2) + (EXT_BUTTON_WIDTH / 2), // x, y, w, h, outline, fill, text
                        EXT_BUTTON_WIDTH,
                        EXT_BUTTON_WIDTH,
                        TFT_ORANGE, // Outline
                        TFT_RED, // Fill
                        TFT_BLACK, // Text
                        "HOP",
                        1);
    } else {
      key[CHAN_HOP_INDEX].initButton(&tft, // Exit box
                        (EXT_BUTTON_WIDTH / 2) * 14,
                        (STATUS_BAR_WIDTH * 2) + (EXT_BUTTON_WIDTH / 2), // x, y, w, h, outline, fill, text
                        EXT_BUTTON_WIDTH,
                        EXT_BUTTON_WIDTH,
                        TFT_WHITE, // Outline
                        TFT_GREEN, // Fill
                        TFT_BLACK, // Text
                        "HOP",
                        1);
    }
  }

  key[CHAN_HOP_INDEX].setLabelDatum(1, 5, MC_DATUM);

  key[CHAN_HOP_INDEX].drawButton();
}

void Display::tftDrawExitScaleButtons(bool lnd_an) {
  //tft.drawFastVLine(178, 0, 20, TFT_WHITE);
  //tft.setCursor(145, 21); tft.setTextColor(TFT_WHITE); tft.setTextSize(1); tft.print("Channel:"); tft.print(set_channel);

  if (lnd_an) {
    key[EXIT_BUTTON_INDEX].initButton(&tft, // Exit box
                      137,
                      10,
                      EXT_BUTTON_WIDTH - (EXT_BUTTON_WIDTH * 0.33),
                      EXT_BUTTON_WIDTH - (EXT_BUTTON_WIDTH * 0.33),
                      TFT_ORANGE,
                      TFT_RED,
                      TFT_BLACK,
                      "X",
                      2);
  }

  else {
    key[EXIT_BUTTON_INDEX].initButton(&tft, // Exit box
                      EXT_BUTTON_WIDTH,
                      (STATUS_BAR_WIDTH * 2) + (EXT_BUTTON_WIDTH / 2), // x, y, w, h, outline, fill, text
                      EXT_BUTTON_WIDTH,
                      EXT_BUTTON_WIDTH,
                      TFT_ORANGE, // Outline
                      TFT_RED, // Fill
                      TFT_BLACK, // Text
                      "X",
                      1);
  }

  key[EXIT_BUTTON_INDEX].setLabelDatum(1, 5, MC_DATUM);

  key[EXIT_BUTTON_INDEX].drawButton();
}

void Display::twoPartDisplay(String center_text)
{
  tft.setTextColor(TFT_BLACK, TFT_YELLOW);
  tft.fillRect(0,16,HEIGHT_1,144, TFT_YELLOW);
  //tft.drawCentreString(center_text,120,82,1);
  tft.setTextWrap(true);
  tft.setFreeFont(NULL);
  //showCenterText(center_text, 82);
  //tft.drawCentreString(center_text,120,82,1);
  tft.setCursor(0, 82);
  tft.println(center_text);
  tft.setFreeFont(MENU_FONT);
  tft.setTextWrap(false);
}

void Display::touchToExit()
{
  tft.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
  tft.fillRect(0,32,HEIGHT_1,16, TFT_LIGHTGREY);
  tft.drawCentreString(text11,TFT_WIDTH / 2,32,2);
}


// Function to just draw the screen black
void Display::clearScreen()
{
  //Serial.println(F("clearScreen()"));
  #ifndef MARAUDER_V7
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0);
  #elif defined(MARAUDER_MINI) || defined(MARAUDER_MINI_V3)
    tft.fillRect(0, 0, TFT_WIDTH, TFT_HEIGHT, TFT_BLACK);
    tft.setCursor(0, 0);
  #else
    tft.fillRect(0, 0, TFT_WIDTH, TFT_HEIGHT, TFT_BLACK);
    tft.setCursor(0, 0);
  #endif
}

#ifdef SCREEN_BUFFER
void Display::scrollScreenBuffer(bool down) {
  // Scroll screen normal direction (Up)
  if (!down) {
    this->screen_buffer->shift();
  }
}
#endif

void Display::processAndPrintString(TFT_eSPI& tft, const String& originalString) {
  // Define colors
  uint16_t text_color = TFT_GREEN; // Default text color
  uint16_t background_color = TFT_BLACK; // Default background color

  String new_string = originalString;

  // Check for color macros at the start of the string
  if (new_string.startsWith(";")) {
    if (new_string.startsWith(RED_KEY)) {
      text_color = TFT_RED;
      new_string.remove(0, strlen(RED_KEY)); // Remove the macro
    } else if (new_string.startsWith(GREEN_KEY)) {
      text_color = TFT_GREEN;
      new_string.remove(0, strlen(GREEN_KEY)); // Remove the macro
    } else if (new_string.startsWith(CYAN_KEY)) {
      text_color = TFT_CYAN;
      new_string.remove(0, strlen(CYAN_KEY)); // Remove the macro
    } else if (new_string.startsWith(WHITE_KEY)) {
      text_color = TFT_WHITE;
      new_string.remove(0, strlen(WHITE_KEY)); // Remove the macro
    } else if (new_string.startsWith(MAGENTA_KEY)) {
      text_color = TFT_MAGENTA;
      new_string.remove(0, strlen(MAGENTA_KEY)); // Remove the macro
    }
  }

  String spaces = String(' ', STANDARD_FONT_CHAR_LIMIT);

  // Set text color and print the string — wrap disabled so long strings clip at edge
  tft.setTextWrap(false);
  tft.setTextColor(text_color, background_color);
  tft.print(new_string + spaces);
  tft.setTextWrap(true); // restore default for other callers
}

void Display::displayBuffer(bool do_clear)
{
  if (this->display_buffer->size() > 0)
  {

    int print_count = 2;

    while ((display_buffer->size() > 0) && (print_count > 0))
    {
      // Freeze adding to display buffer
      if (display_buffer->size() > DISPLAY_BUFFER_LIMIT)
        this->printing = true;

      /*#ifndef SCREEN_BUFFER
        xPos = 0;
        if ((display_buffer->size() > 0) && (!loading))
        {
          //printing = true;
          delay(print_delay_1);
          yDraw = scroll_line(TFT_RED);
          tft.setCursor(xPos, yDraw);
          tft.setTextColor(TFT_GREEN, TFT_BLACK);
          tft.print(display_buffer->shift());
          //printing = false;
          delay(print_delay_2);
        }
        if (!tteBar)
          blank[(18+(yStart - TOP_FIXED_AREA) / TEXT_HEIGHT)%19] = xPos;
        else
          blank[(18+(yStart - TOP_FIXED_AREA_2) / TEXT_HEIGHT)%19] = xPos;
      #else*/
        xPos = 0;
        if (this->screen_buffer->size() >= MAX_SCREEN_BUFFER)
          this->scrollScreenBuffer();

        screen_buffer->add(display_buffer->shift());

        // Force consistent font/size regardless of what menu drawing left active.
        // FreeMono9pt7b (12px/char) left over from menu would cause severe wrapping.
        tft.setFreeFont(NULL);
        tft.setTextSize(1);

        for (int i = 0; i < this->screen_buffer->size(); i++) {
          #if defined(HAS_TOUCH) || defined(HAS_CAP_TOUCH)
            // Start below status bar (16px) + scan header row (16px) + ext buttons (30px) + 2px pad = 64
            tft.setCursor(xPos, (STATUS_BAR_WIDTH * 2) + EXT_BUTTON_WIDTH + 2 + (i * TEXT_HEIGHT));
          #else
            tft.setCursor(xPos, (i * 12) + (TFT_HEIGHT / 6));
          #endif

          this->processAndPrintString(tft, this->screen_buffer->get(i));
        }
      //#endif

      print_count--;
    }

    this->printing = false;
  }
}

void Display::showCenterText(String text, int y)
{
  tft.setCursor((SCREEN_WIDTH - (text.length() * (6 * BANNER_TEXT_SIZE))) / 2, y);
  tft.println(text);
}


/*void Display::initScrollValues(bool tte)
{
  yDraw = YMAX - BOT_FIXED_AREA - TEXT_HEIGHT;

  xPos = 0;

  if (!tte)
  {
    yStart = TOP_FIXED_AREA;

    yArea = YMAX - TOP_FIXED_AREA - BOT_FIXED_AREA;
  }
  else
  {
    yStart = TOP_FIXED_AREA_2;

    yArea = YMAX - TOP_FIXED_AREA_2 - BOT_FIXED_AREA;
  }

  for(uint8_t i = 0; i < 18; i++) blank[i] = 0;
}*/



// Function to execute hardware scroll for TFT screen
/*int Display::scroll_line(uint32_t color) {
  int yTemp = yStart; // Store the old yStart, this is where we draw the next line
  // Use the record of line lengths to optimise the rectangle size we need to erase the top line

  // Check if we have the "touch to exit bar"
  if (!tteBar)
  {
    tft.fillRect(0,yStart,blank[(yStart-TOP_FIXED_AREA)/TEXT_HEIGHT],TEXT_HEIGHT, color);
  
    // Change the top of the scroll area
    yStart+=TEXT_HEIGHT;
    // The value must wrap around as the screen memory is a circular buffer
    if (yStart >= YMAX - BOT_FIXED_AREA) yStart = TOP_FIXED_AREA + (yStart - YMAX + BOT_FIXED_AREA);
  }
  else
  {
    tft.fillRect(0,yStart,blank[(yStart-TOP_FIXED_AREA_2)/TEXT_HEIGHT],TEXT_HEIGHT, color);
  
    // Change the top of the scroll area
    yStart+=TEXT_HEIGHT;
    // The value must wrap around as the screen memory is a circular buffer
    if (yStart >= YMAX - BOT_FIXED_AREA) yStart = TOP_FIXED_AREA_2 + (yStart - YMAX + BOT_FIXED_AREA);
  }
  // Now we can scroll the display
  scrollAddress(yStart);
  return  yTemp;
}*/


// Function to setup hardware scroll for TFT screen
/*void Display::setupScrollArea(uint16_t tfa, uint16_t bfa) {
  #ifdef HAS_ILI9341
    #ifdef HAS_ST7796
      tft.writecommand(0x33);
    #elif defined(HAS_ST7789)
      tft.writecommand(ST7789_VSCRDEF); // Vertical scroll definition
    #else
      tft.writecommand(ILI9341_VSCRDEF);
    #endif
    tft.writedata(tfa >> 8);           // Top Fixed Area line count
    tft.writedata(tfa);
    tft.writedata((YMAX-tfa-bfa)>>8);  // Vertical Scrolling Area line count
    tft.writedata(YMAX-tfa-bfa);
    tft.writedata(bfa >> 8);           // Bottom Fixed Area line count
    tft.writedata(bfa);
  #endif
}*/


/*void Display::scrollAddress(uint16_t vsp) {
  #ifdef HAS_ILI9341
    #ifdef HAS_ST7789
      tft.writecommand(ST7789_VSCRDEF); // Vertical scroll definition
    #elif defined(HAS_ST7796)
      tft.writecommand(0x33);
    #else
      tft.writecommand(ILI9341_VSCRDEF);
    #endif
    tft.writedata(vsp>>8);
    tft.writedata(vsp);
  #endif
}*/

void Display::updateBanner(String msg)
{
  this->buildBanner(msg, current_banner_pos);
}


void Display::buildBanner(String msg, int xpos)
{
  int h = TEXT_HEIGHT;

  this->tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH, TEXT_HEIGHT, TFT_BLACK);
  this->tft.setFreeFont(NULL);           // Font 4 selected
  this->tft.setTextSize(BANNER_TEXT_SIZE);           // Font size scaling is x1
  this->tft.setTextColor(TFT_WHITE, TFT_BLACK);  // Black text, no background colour
  this->showCenterText(msg, STATUS_BAR_WIDTH);
}

#endif
