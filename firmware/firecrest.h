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
 
#ifndef FIRECREST_H
#define FIRECREST_H

#include <avr/eeprom.h>
#include "helper.h"
#include "yack.h"
/*
#define vFWD		SBIT(PINA, 0) 
#define vREV 		SBIT(PINA, 1) 
#define vBAT 		SBIT(PINA, 2) 
#define txVTC		SBIT(PORTA, 3) 
#define txON		SBIT(PORTA, 4) 
#define dit 		SBIT(PORTA, 5) 
#define dah 		SBIT(PORTA, 6) 
#define rotSEL		SBIT(PINA, 7)

#define RFSWMINS	SBIT(PORTB, 0) 
#define RFSWPLUS	SBIT(PORTB, 1) 
#define indtxLED2	SBIT(PORTB, 2) 
#define heartLED1	SBIT(PORTB, 3) 
#define dispPWR		SBIT(PORTB, 4) 
//#define mosi		SBIT(PORTB, 5) 
//#define miso		SBIT(PORTB, 6) 
//#define sck		SBIT(PORTB, 7) 

//#define 			SBIT(PORTC, 0) 		// i2c
//#define 			SBIT(PORTC, 1)	// i2c 

#define VOLCS		SBIT(PORTC, 3) 
#define FILCS		SBIT(PORTC, 4) 
#define VOLUD		SBIT(PORTC, 5) 
#define rotB		SBIT(PINC, 6) 
#define rotA		SBIT(PINC, 7) 

//#define rx0		SBIT(PORTD, 0)
//#define tx0		SBIT(PORTD, 1)
#define uartPWR		SBIT(PORTD, 2)
//#define uartPWR	SBIT(PORTD, 3)	//Reserved
#define dispDC		SBIT(PORTD, 4)
#define dispRST		SBIT(PORTD, 5)
#define dispCS		SBIT(PORTD, 6)
//#define sidetone	SBIT(PORTD, 7)	// Reserved

*/

//********Yack
#define	KEYINP	PINA
#define KEYTX		SBIT(PORTD, 2) 
#define DITPIN	SBIT(PINA, 5) 								
#define DAHPIN	SBIT(PINA, 6) 
#define BTNPIN	SBIT(PINA, 7) 

//********Tx
#define rxMUTE SBIT(PORTD, 6)

//********General system settings
#define ROTA		SBIT(PINC, 7) 	//Rotary encoder B
#define ROTB		SBIT(PINC, 6) 	//Rotary encoder A
#define ROTSEL	SBIT(PINC, 2)	//Main push button

#define RFSWMINS	SBIT(PORTB, 0) 
#define RFSWPLUS	SBIT(PORTB, 1) 

#define VOLCS		SBIT(PORTC, 3) 
#define FILCS		SBIT(PORTC, 4) 
#define VOLUD		SBIT(PORTC, 5) 

#define BANDA		SBIT(PORTD, 3) 	
#define BANDB		SBIT(PORTD, 4) 	

#define STPIN		SBIT(PORTD, 5)

#define ddrPORTA	0b10011000
#define ddrPORTB	0b11111111
#define ddrPORTC	0b00111011
#define ddrPORTD	0b11111111

//OLED Defs
#define SSD1306_CS_PORT 	PORTB
#define SSD1306_CS_PIN		4
#define SSD1306_DC_PORT 	PORTB
#define SSD1306_DC_PIN		2
#define SSD1306_RST_PORT	PORTB
#define SSD1306_RST_PIN		3

#define SSD1306_SCLK_PORT 	PORTB
#define SSD1306_SCLK_PIN  	7
#define SSD1306_SDAT_PORT 	PORTB
#define SSD1306_SDAT_PIN  	5

#define NARROW	0
#define WIDE		1

#define BAND1		10
#define BAND2		14

#define BAND1_UPPER 	10150000
#define BAND1_LOWER		10100000
#define BAND1_F			  10118000

#define BAND2_UPPER	  14100000	
#define BAND2_LOWER   14000000	
#define BAND2_F			  14062000

// If changing this, must change timer in init micro
#define TXOFFSET  650

#define MAX_CALLSIGN_LEN 15

char callStr[15];

/*    
extern volatile unsigned int txKeyed = 0;    
extern uint8_t enableMemoryReplay = 0;		
extern uint8_t battery_state = 0;
extern volatile unsigned int txKeyed;
*/


#define		MAGPAT			0xA5    // If this number is found in EEPROM, content assumed valid

typedef struct {
	char e_call[MAX_CALLSIGN_LEN];
  uint8_t e_kmod;
  uint8_t e_kwpm;
  uint8_t e_filt;
  uint8_t e_band;
  uint8_t e_replay_time;
  uint16_t e_increment;
} eesave_t;

struct systemSettings {
	uint16_t increment;
	uint32_t freq;
	uint32_t ritFreq;
	uint32_t upperFreq;
	uint32_t lowerFreq;
	uint16_t volume;
	uint8_t filterMode;
	uint8_t autoReplay;
	uint8_t band;
	uint8_t keyMode;
};

struct menuStateMachine {
	uint8_t currentState;
	uint8_t txState;
	uint8_t pbCounterOn;
	uint8_t pbCounterOff;
};

enum menuStateDescript {
	Frequency,
	RIT,
	Vol,
	FSTEP,
	Tune,
	M1P,
	M1R,
	Wpm,
	KMOD,
	Fil,
	MAR,
	Band
};       

enum menuDetailsDescript {
    OffTest,
    On,
    Rec,
    Narrow,
    Wide,
    IA,
    IB,
    Str,
    Ul
};  

/*************************************************************************
//Function Prototypes
*************************************************************************/
extern uint16_t getVSWR(void);
extern void transistion_to_rx(void);
extern void transition_to_tx(void);
extern void service_vswr(void);
extern void disable_memory_replay(void);
#endif
