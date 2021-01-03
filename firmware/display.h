/* 
 * Copyright 2021 m5evt
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <inttypes.h>
#include <avr/pgmspace.h>

#define NUMSTATES 		12

#define ICONWIDTHbits 	44
#define ICONROWSbytes 	2

#define TXRXWIDTHbits 	19
#define TXRXROWSbytes 	2

#define	BIGNUMbits		15
#define BIGNUMbytes		3

#define	BIGDOTbits		4
#define BIGDOTbytes		3

#define SMALLNUMbits	11
#define SMALLNUMbytes	3

#define batWIDTHbits	25
#define BATbytes		2

#define mainWIDTHbits	73
#define MAINbytes		3

#define mainSmallWIDTHbits 50	
#define MAINsmallbytes 3

#define vswrWIDTHbits 16
#define vswrBytes 4


extern void displayFreq(uint32_t freq, uint8_t bat_level);
extern void displayFreqSmall(uint32_t rxFreq, uint32_t ritFreq);
extern void drawBat(uint8_t charge);
extern void drawMainArea(int item);
extern void drawMainAreaSmall(int item);
extern void displayChangeStateIcon(uint8_t iconNum);
extern void drawIntNumber(uint16_t number);
extern void clearMainArea(void);
extern void drawVSWR(uint8_t number);

#endif
