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

#ifndef HELPER_h
#define HELPER_h

#include <avr/io.h>

#define DDR(x)	_DDR(x)
#define _DDR(x) DDR##x
#define PORT(x) _PORT(x)
#define _PORT(x) PORT##x
#define PIN(x)	_PIN(x)
#define _PIN(x) PIN##x

#define PIN_PIN(port,pin) ((*(volatile struct bits*)&PIN(port)).b##pin)
#define PIN_DDR(port,pin) ((*(volatile struct bits*)&DDR(port)).b##pin)

#define pin_init_output(port, bit) do {	\
	PIN_DDR(port, bit) = 1;		\
} while (0)

// 			Access bits like variables:
struct bits {
  uint8_t b0:1;
  uint8_t b1:1;
  uint8_t b2:1;
  uint8_t b3:1;
  uint8_t b4:1;
  uint8_t b5:1;
  uint8_t b6:1;
  uint8_t b7:1;
} __attribute__((__packed__));

#define SBIT_(port,pin) ((*(volatile struct bits*)&port).b##pin)

#define sbi(var, mask)   ((var) |= (uint8_t)(1 << mask))
#define cbi(var, mask)   ((var) &= (uint8_t)~(1 << mask))

#define	SBIT(x,y)	SBIT_(x,y)
#define CHECKBIT(ADDRESS,BIT) (ADDRESS & (1<<BIT))

#endif
