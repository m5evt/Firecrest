/**************************************************************************/
/*! 
    @file     ssd1306.h
    @author   K. Townsend (microBuilder.eu)
    @date     22 March 2010
    @version  0.10

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
#ifndef SSD1306_H
#define SSD1306_H

#include <inttypes.h>

#define SSD1306_LCDWIDTH                  128
#define SSD1306_LCDHEIGHT                 32

// Commands
#define SSD1306_SETCONTRAST               0xFF
#define SSD1306_DISPLAYALLON_RESUME       0xA4
#define SSD1306_DISPLAYALLON              0xA5
#define SSD1306_NORMALDISPLAY             0xA6
#define SSD1306_INVERTDISPLAY             0xA7
#define SSD1306_DISPLAYOFF                0xAE
#define SSD1306_DISPLAYON                 0xAF
#define SSD1306_SETDISPLAYOFFSET          0xD3
#define SSD1306_SETCOMPINS                0xDA
#define SSD1306_SETVCOMDETECT             0xDB
#define SSD1306_SETDISPLAYCLOCKDIV        0xD5
#define SSD1306_SETPRECHARGE              0xD9
#define SSD1306_SETMULTIPLEX              0xA8
#define SSD1306_SETLOWCOLUMN              0x00
#define SSD1306_SETHIGHCOLUMN             0x10
#define SSD1306_SETSTARTLINE              0x40
#define SSD1306_MEMORYMODE                0x20
#define SSD1306_COLUMNADDR                0x21
#define SSD1306_PAGEADDR                  0x22
#define SSD1306_COMSCANINC                0xC0
#define SSD1306_COMSCANDEC                0xC8
#define SSD1306_SEGREMAP                  0xA0
#define SSD1306_CHARGEPUMP                0x8D
#define SSD1306_EXTERNALVCC               0x1
#define SSD1306_SWITCHCAPVCC              0x2

uint8_t OLEDbuffer[SSD1306_LCDHEIGHT * SSD1306_LCDWIDTH / 8];

// Initialisation/Config Prototypes
extern void ssd1306Init(uint8_t vccstate);
extern void ssd1306ClearScreen(void);
extern void ssd1306Refresh(void);

#endif
