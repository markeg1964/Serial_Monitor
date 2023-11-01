#include <Arduino.h>


/************************************************
 * By: Mark E. Gunnison
 * This program prints serial data to a cheep TFT screen using an ESP8266.
 * The code could easily be changed to run on an EPS32 or Arduino
 * I'm using the TFT_eSPI library.
 * Much of the code could be deleted if you fix the baud rate and font.
 * Sorry about limited comments.  I did this in a hurry and did not clean it up.
 ***************************************************/

// Following is a sections showing the changes needed to the User_Setup.h file in the TFT_eSPI library.
// For 8266
// #define ILI9341_DRIVER       // Generic driver for common displays
/************************************************
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15  // Chip select control pin
#define TFT_DC   16  // Data Command control pin
//#define TFT_RST   4  // Reset pin (could connect to RST pin)
#define TFT_RST  -1  // Set TFT_RST to -1 if display RESET is connected to ESP32 board RST

// For ESP32 Dev board (only tested with GC9A01 display)
// The hardware SPI can be mapped to any pins

//#define TFT_MOSI 15 // In some display driver board, it might be written as "SDA" and so on.
//#define TFT_SCLK 14
//#define TFT_CS   5  // Chip select control pin
//#define TFT_DC   27  // Data Command control pin
//#define TFT_RST  33  // Reset pin (could connect to Arduino RESET pin)
//#define TFT_BL   22  // LED back-light

#define TOUCH_CS 5     // Chip select pin (T_CS) of touch screen
***************************************************/

#include <TFT_eSPI.h>       // Hardware-specific library
#include <SPI.h>
#include <Wire.h>
#include "FS.h"
#include <SD.h>

//#define ESP8266

//#include "Extensions/Sprite.h"

void drawSetupScreen();
void setupScreen();
void drawBaudScreen();
void setupBaudScreen();

#define LED 2
#define DEL 2

int ledDelay = DEL;   //Delay to keep LED on while reading chars

TFT_eSPI tft = TFT_eSPI();

int screenBottom;

TFT_eSPI_Button button_font_1;
TFT_eSPI_Button button_font_2;
TFT_eSPI_Button button_font_3;
TFT_eSPI_Button button_orient_L;
TFT_eSPI_Button button_orient_P;
TFT_eSPI_Button button_baud;
TFT_eSPI_Button button_save;
TFT_eSPI_Button button_up;
TFT_eSPI_Button button_down;

#define BUTTON_H 40
#define BUTTON_W 100
#define BUTTON_C_OFF    TFT_YELLOW
#define BUTTON_C_ON     TFT_GREEN
#define BUTTON_C_B      TFT_WHITE
#define BUTTON_C_TEXT   TFT_BLACK
int row_font = 35;
int row_orent = 120;

int screen_font = 2;
int screen_orent = 1;

#define MAX_BAUD 10
int baud_rate[MAX_BAUD] = {110, 300, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200};
int baud_selected = 5;

#define CALIBRATION_FILE "/TouchCalData2"

// Set REPEAT_CAL to true instead of false to run calibration
// again, otherwise it will only be done once.
// Repeat calibration if you change the screen rotation.
#define REPEAT_CAL false
//------------------------------------------------------------------------------------------
void touch_calibrate()
{
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  // check file system exists
  if (!SPIFFS.begin()) {
    Serial.println("Formating file system");
    SPIFFS.format();
    SPIFFS.begin();
  }

  // check if calibration file exists and size is correct
  if (SPIFFS.exists(CALIBRATION_FILE)) {
    if (REPEAT_CAL)
    {
      // Delete if we want to re-calibrate
      SPIFFS.remove(CALIBRATION_FILE);
    }
    else
    {
      fs::File f = SPIFFS.open(CALIBRATION_FILE, "r");
      //File f = SPIFFS.open(CALIBRATION_FILE, "r");
      if (f) {
        if (f.readBytes((char *)calData, 14) == 14)
          calDataOK = 1;
        f.close();
      }
    }
  }

  if (calDataOK && !REPEAT_CAL) {
    // calibration data valid
    tft.setTouch(calData);
  } else {
    // data not valid so recalibrate
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.println("Touch corners as indicated");

    tft.setTextFont(1);
    tft.println();

    if (REPEAT_CAL) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.println("Set REPEAT_CAL to false to stop this running again!");
    }

    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("Calibration complete!");

    // store data
    fs::File f = SPIFFS.open(CALIBRATION_FILE, "w");
    //File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f) {
      f.write((const unsigned char *)calData, 14);
      f.close();
    }
  }
}

//------------------------------------------------------------------------------------------


void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);

  pinMode(LED, OUTPUT);    
  digitalWrite(LED, HIGH);

  tft.begin();
  // Calibrate the touch screen and retrieve the scaling factors
  //touch_calibrate();
  tft.setRotation(1); 
  touch_calibrate();
  tft.setTextFont(2);  //1 2 4 
  screenBottom  = tft.getViewportHeight();

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); // Set the font colour and the background colour

  tft.println("Reading RX...");

}

void loop() {
  
  uint16_t x, y;


  if (tft.getTouch(&x, &y))
  {
    drawSetupScreen();
    setupScreen();
    Serial.begin(baud_rate[baud_selected]);
    if (screen_orent==1)
      tft.setRotation(1); 
    else
      tft.setRotation(0); 
    tft.setTextFont(screen_font);
    tft.setTextColor(TFT_WHITE, TFT_BLACK); 
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0,0);
    tft.println("Reading RX...");
  }
  

  if (Serial.available())
  {

    if (tft.getCursorY() > screenBottom)  // Text off bottom of desipay
    {
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0,0);
    }

    digitalWrite(LED, HIGH);    // Flash LED on read
    ledDelay = DEL;

    char ch = Serial.read();
    tft.print(ch);    
    
    //tft.print( Serial.read() );    // This returns char values

  }


  if (ledDelay < 0)   // Wait DEL loops before turning off LED
  {
    digitalWrite(LED, LOW);
    ledDelay = 0;
  }

  ledDelay --;

}


/******************************************************/
/************* Draw Screen to set layout **************/
/******************************************************/
void drawSetupScreen()
{

  char str[100];
  //int row;

  tft.fillScreen( TFT_BLACK );
  tft.setRotation(1); 
  //TFT_eSPI_Button button_font_1;
  //TFT_eSPI_Button button_font_2;
  //TFT_eSPI_Button button_font_3;
  //TFT_eSPI_Button button_orient_L;
  //TFT_eSPI_Button button_orient_P;
  //TFT_eSPI_Button button_save;

  tft.setTextColor( TFT_WHITE, TFT_BLACK );
  tft.setTextFont(4);
  tft.setTextSize(1);

  strcpy( str, "Text Size:");
  tft.drawString(str,5,10);
  //row = 35;

  strcpy( str, "1");
  if (screen_font==1)
    button_font_1.initButtonUL(&tft, 5, row_font, BUTTON_W, BUTTON_H, BUTTON_C_B, BUTTON_C_ON, BUTTON_C_TEXT, str, 1);
    else
    button_font_1.initButtonUL(&tft, 5, row_font, BUTTON_W, BUTTON_H, BUTTON_C_B, BUTTON_C_OFF, BUTTON_C_TEXT, str, 1);
  button_font_1.drawButton();

  strcpy( str, "2");
  if (screen_font==2)
    button_font_2.initButtonUL(&tft, 110, row_font, BUTTON_W, BUTTON_H, BUTTON_C_B, BUTTON_C_ON, BUTTON_C_TEXT, str, 1);
    else
    button_font_2.initButtonUL(&tft, 110, row_font, BUTTON_W, BUTTON_H, BUTTON_C_B, BUTTON_C_OFF, BUTTON_C_TEXT, str, 1);
  button_font_2.drawButton();

  strcpy( str, "3");
  if (screen_font==4)
    button_font_3.initButtonUL(&tft, 215, row_font, BUTTON_W, BUTTON_H, BUTTON_C_B, BUTTON_C_ON, BUTTON_C_TEXT, str, 1);
    else
    button_font_3.initButtonUL(&tft, 215, row_font, BUTTON_W, BUTTON_H, BUTTON_C_B, BUTTON_C_OFF, BUTTON_C_TEXT, str, 1);
  button_font_3.drawButton();

  strcpy( str, "Orentation:");
  tft.setTextColor( TFT_WHITE, TFT_BLACK );
  tft.drawString(str,5,95);
  //row = 120;

  strcpy( str, "Landscape");
  if (screen_orent==1)
    button_orient_L.initButtonUL(&tft, 5, row_orent, BUTTON_W+50, BUTTON_H, BUTTON_C_B, BUTTON_C_ON, BUTTON_C_TEXT, str, 1);
    else
    button_orient_L.initButtonUL(&tft, 5, row_orent, BUTTON_W+50, BUTTON_H, BUTTON_C_B, BUTTON_C_OFF, BUTTON_C_TEXT, str, 1);
  button_orient_L.drawButton();

  strcpy( str, "Portrait");
  if (screen_orent==2)
    button_orient_P.initButtonUL(&tft, 160, row_orent, BUTTON_W+50, BUTTON_H, BUTTON_C_B, BUTTON_C_ON, BUTTON_C_TEXT, str, 1);
    else
    button_orient_P.initButtonUL(&tft, 160, row_orent, BUTTON_W+50, BUTTON_H, BUTTON_C_B, BUTTON_C_OFF, BUTTON_C_TEXT, str, 1);
  button_orient_P.drawButton();

  strcpy( str, "EXIT");
  //row = 190;
  button_save.initButtonUL(&tft, 110-70, 190, BUTTON_W, BUTTON_H, BUTTON_C_B, BUTTON_C_OFF, BUTTON_C_TEXT, str, 1);
  button_save.drawButton();

  strcpy( str, "Baud");
  //row = 190;
  button_baud.initButtonUL(&tft, 110+70, 190, BUTTON_W, BUTTON_H, BUTTON_C_B, BUTTON_C_OFF, BUTTON_C_TEXT, str, 1);
  button_baud.drawButton();


  //delay(5000);

}

/*************************************************/
/************* Screen to set layout **************/
/*************************************************/
void setupScreen()
{

char str[100];
uint16_t touch_x=0, touch_y=0;
bool touched = false;
bool exit_loop = false;


  while (!exit_loop)
  {

    touched = tft.getTouch( &touch_x, &touch_y);

    if (touched && button_font_1.contains(touch_x, touch_y))
      button_font_1.press(true);
    else
      button_font_1.press(false);

    if (touched && button_font_2.contains(touch_x, touch_y))
      button_font_2.press(true);
    else
      button_font_2.press(false);

    if (touched && button_font_3.contains(touch_x, touch_y))
      button_font_3.press(true);
    else
      button_font_3.press(false);

    if (touched && button_orient_L.contains(touch_x, touch_y))
      button_orient_L.press(true);
    else
      button_orient_L.press(false);

    if (touched && button_orient_P.contains(touch_x, touch_y))
      button_orient_P.press(true);
    else
      button_orient_P.press(false);

    if (touched && button_baud.contains(touch_x, touch_y))
      button_baud.press(true);
    else
      button_baud.press(false);

    if (touched && button_save.contains(touch_x, touch_y))
      button_save.press(true);
    else
      button_save.press(false);


    if (button_font_1.justPressed())
      button_font_1.drawButton(true);
    if (button_font_1.justReleased())
    {
      screen_font = 1;
      button_font_1.drawButton(false);
      strcpy( str, "1");
      button_font_1.initButtonUL(&tft, 5, row_font, BUTTON_W, BUTTON_H, BUTTON_C_B, BUTTON_C_ON, BUTTON_C_TEXT, str, 1);
      strcpy( str, "2");
      button_font_2.initButtonUL(&tft, 110, row_font, BUTTON_W, BUTTON_H, BUTTON_C_B, BUTTON_C_OFF, BUTTON_C_TEXT, str, 1);
      strcpy( str, "3");
      button_font_3.initButtonUL(&tft, 215, row_font, BUTTON_W, BUTTON_H, BUTTON_C_B, BUTTON_C_OFF, BUTTON_C_TEXT, str, 1);
      button_font_1.drawButton();
      button_font_2.drawButton();
      button_font_3.drawButton();
    }


    if (button_font_2.justPressed())
      button_font_2.drawButton(true);
    if (button_font_2.justReleased())
    {
      screen_font = 2;
      button_font_2.drawButton(false);
      strcpy( str, "1");
      button_font_1.initButtonUL(&tft, 5, row_font, BUTTON_W, BUTTON_H, BUTTON_C_B, BUTTON_C_OFF, BUTTON_C_TEXT, str, 1);
      strcpy( str, "2");
      button_font_2.initButtonUL(&tft, 110, row_font, BUTTON_W, BUTTON_H, BUTTON_C_B, BUTTON_C_ON, BUTTON_C_TEXT, str, 1);
      strcpy( str, "3");
      button_font_3.initButtonUL(&tft, 215, row_font, BUTTON_W, BUTTON_H, BUTTON_C_B, BUTTON_C_OFF, BUTTON_C_TEXT, str, 1);
      button_font_1.drawButton();
      button_font_2.drawButton();
      button_font_3.drawButton();
    }

    if (button_font_3.justPressed())
      button_font_3.drawButton(true);
    if (button_font_3.justReleased())
    {
      screen_font = 4;
      button_font_3.drawButton(false);
      strcpy( str, "1");
      button_font_1.initButtonUL(&tft, 5, row_font, BUTTON_W, BUTTON_H, BUTTON_C_B, BUTTON_C_OFF, BUTTON_C_TEXT, str, 1);
      strcpy( str, "2");
      button_font_2.initButtonUL(&tft, 110, row_font, BUTTON_W, BUTTON_H, BUTTON_C_B, BUTTON_C_OFF, BUTTON_C_TEXT, str, 1);
      strcpy( str, "3");
      button_font_3.initButtonUL(&tft, 215, row_font, BUTTON_W, BUTTON_H, BUTTON_C_B, BUTTON_C_ON, BUTTON_C_TEXT, str, 1);
      button_font_1.drawButton();
      button_font_2.drawButton();
      button_font_3.drawButton();
    }


    if (button_orient_L.justPressed())
      button_orient_L.drawButton(true);
    if (button_orient_L.justReleased())
    {
      screen_orent = 1;
      
      button_orient_L.drawButton(false);
      strcpy( str, "Landscape");
      button_orient_L.initButtonUL(&tft, 5, row_orent, BUTTON_W+50, BUTTON_H, BUTTON_C_B, BUTTON_C_ON, BUTTON_C_TEXT, str, 1);
      strcpy( str, "Portrait");
      button_orient_P.initButtonUL(&tft, 160, row_orent, BUTTON_W+50, BUTTON_H, BUTTON_C_B, BUTTON_C_OFF, BUTTON_C_TEXT, str, 1);
      button_orient_L.drawButton();
      button_orient_P.drawButton();

    }


    if (button_orient_P.justPressed())
      button_orient_P.drawButton(true);
    if (button_orient_P.justReleased())
    {
      screen_orent = 2;
      
      button_orient_P.drawButton(false);
      strcpy( str, "Landscape");
      button_orient_L.initButtonUL(&tft, 5, row_orent, BUTTON_W+50, BUTTON_H, BUTTON_C_B, BUTTON_C_OFF, BUTTON_C_TEXT, str, 1);
      strcpy( str, "Portrait");
      button_orient_P.initButtonUL(&tft, 160, row_orent, BUTTON_W+50, BUTTON_H, BUTTON_C_B, BUTTON_C_ON, BUTTON_C_TEXT, str, 1);
      button_orient_L.drawButton();
      button_orient_P.drawButton();

    }

    if (button_baud.justPressed())
      button_baud.drawButton(true);
    if (button_baud.justReleased())
    {
      drawBaudScreen();
      setupBaudScreen();
      exit_loop = true;
      button_baud.drawButton(false);
    }


    if (button_save.justPressed())
      button_save.drawButton(true);
    if (button_save.justReleased())
    {
      exit_loop = true;
      button_save.drawButton(false);
    }


  }
}

/******************************************************/
/************* Draw Screen to set baud rate************/
/******************************************************/
void drawBaudScreen()
{

  char str[100];
  int row;

  tft.fillScreen( TFT_BLACK );
  tft.setRotation(1); 

  tft.setTextColor( TFT_WHITE, TFT_BLACK );
  tft.setTextFont(4);
  tft.setTextSize(1);

  strcpy( str, "Baud Rate:");
  tft.drawString(str,5,10);

  row = row_font;

  strcpy( str, "+");
  button_up.initButtonUL(&tft, 110, row, BUTTON_W, BUTTON_H, BUTTON_C_B, BUTTON_C_OFF, BUTTON_C_TEXT, str, 1);
  button_up.drawButton();

  tft.setTextColor( TFT_WHITE, TFT_BLACK );
  row += 48;
  sprintf(str, "   %8d            ", baud_rate[baud_selected]);
  tft.drawString( str, 100, row);

  row += 30;
  strcpy( str, "-");
  button_down.initButtonUL(&tft, 110, row, BUTTON_W, BUTTON_H, BUTTON_C_B, BUTTON_C_OFF, BUTTON_C_TEXT, str, 1);
  button_down.drawButton();

  strcpy( str, "EXIT");
  //row = 190;
  button_save.initButtonUL(&tft, 110, 190, BUTTON_W, BUTTON_H, BUTTON_C_B, BUTTON_C_OFF, BUTTON_C_TEXT, str, 1);
  button_save.drawButton();

  //delay(5000);

}


void setupBaudScreen()
{

char str[100];
uint16_t touch_x=0, touch_y=0;
bool touched = false;
bool exit_loop = false;
  int row = 30;


  while (!exit_loop)
  {

    touched = tft.getTouch( &touch_x, &touch_y);

    if (touched && button_up.contains(touch_x, touch_y))
      button_up.press(true);
    else
      button_up.press(false);

    if (touched && button_down.contains(touch_x, touch_y))
      button_down.press(true);
    else
      button_down.press(false);

    if (touched && button_save.contains(touch_x, touch_y))
      button_save.press(true);
    else
      button_save.press(false);

    row = row_font;

    if (button_up.justPressed())
      button_up.drawButton(true);
    if (button_up.justReleased())
    {
      baud_selected++;
      if (baud_selected==MAX_BAUD)
        baud_selected = 0;
      button_up.drawButton(false);
      strcpy( str, "+");
      button_up.initButtonUL(&tft, 110, row, BUTTON_W, BUTTON_H, BUTTON_C_B, BUTTON_C_OFF, BUTTON_C_TEXT, str, 1);
      button_up.drawButton();
    }


    tft.setTextColor( TFT_WHITE, TFT_BLACK );
    row += 48;
    sprintf(str, "   %8d            ", baud_rate[baud_selected]);
    tft.drawString( str, 100, row);


    row += 30;
    if (button_down.justPressed())
      button_down.drawButton(true);
    if (button_down.justReleased())
    {
      baud_selected--;
      if (baud_selected < 0)
        baud_selected = MAX_BAUD-1;
      button_down.drawButton(false);
      strcpy( str, "-");
      button_down.initButtonUL(&tft, 110, row, BUTTON_W, BUTTON_H, BUTTON_C_B, BUTTON_C_OFF, BUTTON_C_TEXT, str, 1);
      button_down.drawButton();
    }




    if (button_save.justPressed())
      button_save.drawButton(true);
    if (button_save.justReleased())
    {
      exit_loop = true;
      button_save.drawButton(false);
    }



  }

}



