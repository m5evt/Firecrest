/**************************************************************************/
/*! 
    @file     ssd1306.c
    @author   K. Townsend (microBuilder.eu)

    @section DESCRIPTION

    Driver for 128x64 OLED display based on the SSD1306 controller.

    This driver is based on the SSD1306 Library from Limor Fried
    (Adafruit Industries) at: https://github.com/adafruit/SSD1306  
    
    @section LICENSE

    Software License Agreement (BSD License)

    Copyright (c) 2010, microBuilder SARL
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    3. Neither the name of the copyright holders nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    
    MODIFIED by M5EVT 2015-2020 for Firecrest tcvr 
*/
/**************************************************************************/
#include <string.h>

#include "ssd1306.h"
#include "firecrest.h"


//static void ssd1306DrawPixel(uint8_t x, uint8_t y);
//static void ssd1306ClearPixel(uint8_t x, uint8_t y);
//static uint8_t ssd1306GetPixel(uint8_t x, uint8_t y);
static void ssd1306SendByte(uint8_t byte);

#define CMD(c)        do { sbi(SSD1306_CS_PORT, SSD1306_CS_PIN); \
                           cbi(SSD1306_DC_PORT, SSD1306_DC_PIN); \
                           cbi(SSD1306_CS_PORT, SSD1306_CS_PIN); \
                           ssd1306SendByte(c); \
                           sbi(SSD1306_CS_PORT, SSD1306_CS_PIN); \
                         } while (0);
#define DATA(c)       do { sbi(SSD1306_CS_PORT, SSD1306_CS_PIN); \
                           sbi(SSD1306_DC_PORT, SSD1306_DC_PIN); \
                           cbi(SSD1306_CS_PORT, SSD1306_CS_PIN); \
                           ssd1306SendByte(c); \
                           sbi(SSD1306_CS_PORT, SSD1306_CS_PIN); \
                         } while (0);

//###TEMP loaded splash screen in buffer
/*
static uint8_t OLEDbuffer[SSD1306_LCDHEIGHT * SSD1306_LCDWIDTH / 8] = { 
0x1 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x80 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x80 , 0x80 , 0x80 , 0x80 , 0x80 , 0x80 , 0x80 , 0x80 , 0x80 , 0x80 , 0x80 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x80 , 0x80 , 0x80 , 0x80 , 0x80 , 0x80 , 0x80 , 0x80 , 0x80 , 0x80 , 0x80 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x80 , 0x80 , 0x80 , 0x80 , 0x80 , 0x80 , 0x80 , 0x80 , 0x80 , 0x80 , 0x80 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x80 , 0x80 , 0x80 , 0x80 , 0x80 , 0x80 , 0x80 , 0x80 , 0x80 , 0x80 , 0x80 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0xfc , 0x84 , 0x84 , 0x44 , 0x38 , 0x0 , 0x0 , 0x4 , 0xfc , 0x4 , 0x0 , 0x0 , 0x4 , 0x4 , 0x4 , 0xfc , 0x4 , 0x4 , 0x4 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0xff , 0xff , 0x0 , 0x0 , 0x0 , 0xff , 0xff , 0x1 , 0x1 , 0x1 , 0x1 , 0x1 , 0x1 , 0x1 , 0x1 , 0x1 , 0xff , 0xff , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0xff , 0xff , 0x1 , 0x1 , 0x1 , 0x1 , 0x1 , 0x1 , 0x1 , 0x1 , 0x1 , 0xff , 0xff , 0x0 , 0x0 , 0x0 , 0xff , 0xff , 0x1 , 0x1 , 0x1 , 0x1 , 0x1 , 0x1 , 0x1 , 0x1 , 0x1 , 0x1 , 0x1 , 0x0 , 0x0 , 0x0 , 0xff , 0xff , 0x1 , 0x1 , 0x1 , 0x1 , 0x1 , 0x1 , 0x1 , 0x1 , 0x1 , 0xff , 0xff , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0xf , 0x0 , 0x1 , 0x2 , 0xc , 0x0 , 0x0 , 0x8 , 0xf , 0x8 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0xf , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0xff , 0xef , 0x0 , 0x0 , 0x0 , 0xef , 0xff , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0xff , 0xef , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0xef , 0xff , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0xff , 0xef , 0x0 , 0x0 , 0x0 , 0xef , 0xff , 0x18 , 0x18 , 0x18 , 0x18 , 0x18 , 0x18 , 0x18 , 0x18 , 0x18 , 0xf8 , 0xe8 , 0x0 , 0x0 , 0x0 , 0xef , 0xff , 0x18 , 0x18 , 0x18 , 0x18 , 0x18 , 0x18 , 0x18 , 0x18 , 0x18 , 0xff , 0xef , 0x0 , 0x0 , 0x0 , 0x78 , 0xfc , 0x6 , 0x6 , 0x6 , 0x6 , 0x6 , 0xfc , 0x78 , 0x0 , 0x0 , 0x78 , 0xfc , 0x6 , 0x6 , 0x6 , 0x6 , 0x6 , 0xfc , 0x78 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0xf0 , 0xd0 , 0xd0 , 0xd0 , 0x10 , 0xd0 , 0xd0 , 0xd0 , 0xf0 , 0xf0 , 0xd0 , 0x30 , 0xf0 , 0x30 , 0xd0 , 0xf0 , 0xf0 , 0xf0 , 0x0 , 0x0 , 
0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0xff , 0x7f , 0x0 , 0x0 , 0x0 , 0x7f , 0xff , 0xc0 , 0xc0 , 0xc0 , 0xc0 , 0xc0 , 0xc0 , 0xc0 , 0xc0 , 0xc0 , 0xff , 0x7f , 0x0 , 0x0 , 0xe0 , 0xe0 , 0xe0 , 0x0 , 0x0 , 0x7f , 0xff , 0xc0 , 0xc0 , 0xc0 , 0xc0 , 0xc0 , 0xc0 , 0xc0 , 0xc0 , 0xc0 , 0xff , 0x7f , 0x0 , 0x0 , 0x0 , 0x7f , 0xff , 0xc0 , 0xc0 , 0xc0 , 0xc0 , 0xc0 , 0xc0 , 0xc0 , 0xc0 , 0xc0 , 0xff , 0x7f , 0x0 , 0x0 , 0x0 , 0x7f , 0xff , 0xc0 , 0xc0 , 0xc0 , 0xc0 , 0xc0 , 0xc0 , 0xc0 , 0xc0 , 0xc0 , 0xff , 0x7f , 0x0 , 0x0 , 0x0 , 0x7b , 0xff , 0xc3 , 0xc3 , 0xc3 , 0xc3 , 0xc3 , 0xff , 0x7b , 0x0 , 0x0 , 0x7b , 0xff , 0xc3 , 0xc3 , 0xc3 , 0xc3 , 0xc3 , 0xff , 0x7b , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x7f , 0x7f , 0x7f , 0x7f , 0x70 , 0x7f , 0x7f , 0x7f , 0x7f , 0x7f , 0x77 , 0x79 , 0x7e , 0x79 , 0x77 , 0x7f , 0x7f , 0x7f , 0x0 , 0x0

};
*/
/**************************************************************************/
/* Private Methods                                                        */
/**************************************************************************/

/**************************************************************************/
/*! 
    @brief Simulates an SPI write using GPIO

    @param[in]  byte
                The byte to send
*/
/**************************************************************************/
void ssd1306SendByte(uint8_t byte) {
  int8_t i; 

  // Make sure clock pin starts high
  sbi(SSD1306_SCLK_PORT, SSD1306_SCLK_PIN);

  // Write from MSB to LSB
  for (i=7; i>=0; i--) {
    // Set clock pin low
    cbi(SSD1306_SCLK_PORT, SSD1306_SCLK_PIN);

    // Set data pin high or low depending on the value of the current bit
    byte & (1 << i) ? sbi(SSD1306_SDAT_PORT, SSD1306_SDAT_PIN) : cbi(SSD1306_SDAT_PORT, SSD1306_SDAT_PIN);
    // Set clock pin high
    sbi(SSD1306_SCLK_PORT, SSD1306_SCLK_PIN);
  }
}

/**************************************************************************/
/*!
    @brief  Draws a single graphic character using the supplied font
*/
/**************************************************************************/

/**************************************************************************/
/* Public Methods                                                         */
/**************************************************************************/

/**************************************************************************/
/*! 
    @brief Initialises the SSD1306 LCD display
*/
/**************************************************************************/
void ssd1306Init(uint8_t vccstate) {
	// Reset the LCD
	sbi(SSD1306_RST_PORT, SSD1306_RST_PIN);
	yackbeat();
	cbi(SSD1306_RST_PORT, SSD1306_RST_PIN);
	yackbeat();
	yackbeat();  
	sbi(SSD1306_RST_PORT, SSD1306_RST_PIN);
  
  // Init sequence for 128x32 OLED module
  CMD(SSD1306_DISPLAYOFF);                    // 0xAE
  CMD(SSD1306_SETDISPLAYCLOCKDIV);            // 0xD5
  CMD(0b1000001);                                  // the suggested ratio 0x80

  CMD(SSD1306_SETMULTIPLEX);                  // 0xA8
  CMD(0x1F);
  CMD(SSD1306_SETDISPLAYOFFSET);              // 0xD3
  CMD(0x0);                                   // no offset
  CMD(SSD1306_SETSTARTLINE | 0x0);            // line #0
  CMD(SSD1306_CHARGEPUMP);                    // 0x8D
  if (vccstate == SSD1306_EXTERNALVCC) { 
    CMD(0x10);
  }
  else { 
    CMD(0x14); 
  }
  
  CMD(SSD1306_MEMORYMODE);                    // 0x20
  CMD(0x00);                                  // 0x0 act like ks0108
  CMD(SSD1306_SEGREMAP | 0x1);
  CMD(SSD1306_COMSCANDEC);
  CMD(SSD1306_SETCOMPINS);                    // 0xDA
  CMD(0x02);
  CMD(SSD1306_SETCONTRAST);                   // 0x81
  CMD(0x00);
  CMD(SSD1306_SETPRECHARGE);                  // 0xd9
  if (vccstate == SSD1306_EXTERNALVCC) { 
    CMD(0x22); 
  }
  else { 
    CMD(0xF1); 
  }
  CMD(SSD1306_SETVCOMDETECT);                 // 0xDB
  CMD(0x40);
  CMD(SSD1306_DISPLAYALLON_RESUME);           // 0xA4
  CMD(SSD1306_NORMALDISPLAY);                 // 0xA6

	CMD(SSD1306_DISPLAYON);//--turn on oled panel
}

/**************************************************************************/
/*! 
    @brief Draws a single pixel in image buffer

    @param[in]  x
                The x position (0..127)
    @param[in]  y
                The y position (0..63)
*/
/**************************************************************************/
/*
void ssd1306DrawPixel(uint8_t x, uint8_t y) {
  if ((x >= SSD1306_LCDWIDTH) || (y >= SSD1306_LCDHEIGHT)) return;

  OLEDbuffer[x+ (y/8)*SSD1306_LCDWIDTH] |=  (1 << (y&7));
}
*/
/**************************************************************************/
/*! 
    @brief Clears a single pixel in image buffer

    @param[in]  x
                The x position (0..127)
    @param[in]  y
                The y position (0..63)
*/
/**************************************************************************/
/*
void ssd1306ClearPixel(uint8_t x, uint8_t y) {
  if ((x >= SSD1306_LCDWIDTH) || (y >= SSD1306_LCDHEIGHT)) return;

  OLEDbuffer[x+ (y/8)*SSD1306_LCDWIDTH] &= ~(1 << (y&7)); 
}
*/

/**************************************************************************/
/*! 
    @brief Gets the value (1 or 0) of the specified pixel from the buffer

    @param[in]  x
                The x position (0..127)
    @param[in]  y
                The y position (0..63)

    @return     1 if the pixel is enabled, 0 if disabled
*/
/**************************************************************************/
/*
uint8_t ssd1306GetPixel(uint8_t x, uint8_t y) {
  if ((x >= SSD1306_LCDWIDTH) || (y >=SSD1306_LCDHEIGHT)) return 0;
  return OLEDbuffer[x+ (y/8)*SSD1306_LCDWIDTH] & (1 << y%8) ? 1 : 0;
}
*/

/**************************************************************************/
/*! 
    @brief Clears the screen
*/
/**************************************************************************/
void ssd1306ClearScreen() {
  memset(OLEDbuffer, 0, 512);
}

/**************************************************************************/
/*! 
    @brief Renders the contents of the pixel buffer on the LCD
*/
/**************************************************************************/
void ssd1306Refresh(void) {
  CMD(SSD1306_COLUMNADDR);
  CMD(0);   // Column start address (0 = reset)
  CMD(SSD1306_LCDWIDTH-1); // Column end address (127 = reset)

  CMD(SSD1306_PAGEADDR);
  CMD(0); // Page start address (0 = reset)
  CMD(3); // Page end address

  uint16_t i;
  for (i=0; i<512; i++) {
    DATA(OLEDbuffer[i]);
  }
}
