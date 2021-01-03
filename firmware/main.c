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

/********************************************************************************
Includes
********************************************************************************/
#include <avr/io.h>
#include <stdbool.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <inttypes.h>
#include <string.h> 
#include <stdio.h>
#include <math.h>
#include <avr/eeprom.h>

#include "helper.h"
#include "firecrest.h"
//VFO
#include "i2c.h"
#include "si5351a.h"


/*************************************************************************
//Defines
*************************************************************************/
// define CPU frequency in Mhz here if not defined in Makefile
#ifndef F_CPU
#define F_CPU 8000000UL
#endif

volatile unsigned int rotEncChange = 0;
volatile unsigned int storedRotA = 0;
volatile unsigned int storedRotB = 0;

volatile unsigned int storedRot = 0;
uint8_t battery_state;
uint8_t enableMemoryReplay;

uint8_t straight_key = 0;
uint16_t countDownMemoryTimer = 0;
uint8_t memoryReplayNow;
uint8_t sk_state = 0;
uint16_t sk_timer;

volatile uint8_t txKeyed;

uint8_t magic EEMEM = MAGPAT;	// Needs to contain 'A5' if mem is valid

// Settings for EEPROM
eesave_t ee_vars EEMEM;


const char batLowStr[] = " BAT LOW ";
char cqStr[] = "CQ CQ CQ de";

///============Function Prototypes=========/////////////////
static void initMicro(void);
//static void serviceEncoder(void);
static void serviceFreq(struct systemSettings *setting);
static void service_straight_key(void);
static uint8_t serviceMenu(struct menuStateMachine *state, struct systemSettings *setting);
static void serviceBattery(void);
static void filterUPone(void);
static void filterDOWNone(void);
static void volumeUPone(void);
static void volumeDOWNone(void);
static void memoryCQ(void);
static uint16_t serviceStep(uint8_t up, uint8_t down, uint16_t step);
static void ChangeBand(uint8_t band, struct systemSettings *setting);
static uint8_t serviceKeyerMode(uint8_t up, uint8_t down, uint8_t keyerMode);

/*************************************************************************
//Added from yack main.c
*************************************************************************/

#include "ssd1306.h"
#include "display.h"
#include "yack.h"

/*************************************************************************
//Main
*************************************************************************/
int main(void) {
	uint16_t i = 0;
	
	KEYTX = 0;
	rxMUTE = 1;
    
  callStr[0] = "M";   
  callStr[1] = "5";
  callStr[2] = "E";
  callStr[3] = "V";
  callStr[4] = "T";
  callStr[5] = "/";
  callStr[6] = "P";
  //callStr[7] = "\0";

  // EEPROM settings in RAM
  //struct 
  eesave_t ram_vars;
  
  // Read the EEPROM saved values  
	eeprom_read_block(&ram_vars, &ee_vars, sizeof(eesave_t));
    
  struct systemSettings mySettings;
	struct menuStateMachine myStateMachine;

	myStateMachine.currentState = 0;

	/**
	General AVR init
	**/
	initMicro();
  RFSWMINS = 0;
  RFSWPLUS = 0;
    
  // Is the EEPROM valid?
	uint8_t magval = eeprom_read_byte(&magic); // Retrieve magic value
  if (magval == MAGPAT) {
    //Move this into a function
    strncpy(callStr, ram_vars.e_call, 12);

    mySettings.keyMode = ram_vars.e_kmod;    
    serviceKeyerMode(0,0,mySettings.keyMode);
        
    wpm = ram_vars.e_kwpm;
    mySettings.band = ram_vars.e_band;
    mySettings.filterMode = ram_vars.e_filt;
    mySettings.autoReplay = ram_vars.e_replay_time;

  }
  else {
    // Corrupt EEPROM, load defaults
    mySettings.keyMode = 5;
    wpm = 16;
    // Wide filter
    mySettings.filterMode = 0;
    mySettings.autoReplay = 5;
    mySettings.band = 10;
  }
        
  ChangeBand(mySettings.band, &mySettings);
  mySettings.increment = 1000;       
    
	memoryReplayNow = 0;
	enableMemoryReplay = 0;
	
  myStateMachine.pbCounterOn = 0;	
  
	//OLED init and splash load
	ssd1306Init(SSD1306_EXTERNALVCC);
	ssd1306ClearScreen();
	ssd1306Refresh();
  sei(); 

	//VFO init
	i2c_init();
  si5351aSetInitFrequency(mySettings.freq);
  si5351aSetInitTXFrequency((mySettings.freq / 4) - TXOFFSET);
  si5351_clock_enable(SI5351_CLK2, 0);    

	//Keyer init
	yackinit();
    
	// Take pot to 0 value
	for (i=0;i<8;i++) {
		volumeDOWNone();
	}	
	
	for (i=0;i<4;i++) {
		volumeUPone();
		mySettings.volume = 4;	
	}
	
	// Take pot to NARROW	
	for (i=0;i<8;i++) {
		filterDOWNone();
	}
  
	// Make WIDE if saved setting says so
  if (mySettings.filterMode != NARROW) {
    for (i=0;i<8;i++) {
      filterUPone();
    }		    
  }
	//###
	myStateMachine.currentState = 0;

	//Initialise screen in state 0
	ssd1306ClearScreen();	
  serviceBattery();
	displayFreq(mySettings.freq, battery_state);
	ssd1306Refresh();
	
	//Mute: 1 = mute, 0 = audio
	//KEYTX = 0;
  rxMUTE = 0;
	
	//Send call sign welcome CW
	yackinhibit(ON);  //side tone greeting to confirm the unit is alive
	yackstring(callStr);
	yackinhibit(OFF);
		
	/********************************************
	Main program loop
	********************************************/
	// Run in this loop now until death 
	for(;;) {	
		if (txKeyed == 0) {
			i++;
			
			if (myStateMachine.currentState == 0) {
				serviceFreq(&mySettings);
			}
      else if ((myStateMachine.currentState > Vol) && (myStateMachine.currentState != M1R)) {
        // Option to exit the menu with a press of the key
        if ((DITPIN == 0) || (DAHPIN == 0)) {
          yackinhibit(OFF);	
          myStateMachine.currentState = 0;
          ssd1306ClearScreen();	
          displayFreq(mySettings.freq, battery_state);
          ssd1306Refresh();              
          yackdelay(5);
        }
      }
			
			if (enableMemoryReplay == 1) {
				countDownMemoryTimer--;
				if (countDownMemoryTimer == 0) {
					//myStateMachine.currentState == M1p;
					myStateMachine.currentState = 5;
					enableMemoryReplay = 0;
					memoryReplayNow = 1;			
				}
			}
			
			uint8_t write_eeprom = serviceMenu(&myStateMachine, &mySettings);
            
      if (write_eeprom == 1) {
        yackinhibit(ON);  //side tone greeting to confirm the unit is alive
        yackstring("T");
        yackinhibit(OFF);
                
        eeprom_update_byte(&magic, MAGPAT);
        strncpy(ram_vars.e_call, callStr, MAX_CALLSIGN_LEN);
        ram_vars.e_kmod = mySettings.keyMode;
        ram_vars.e_kwpm = wpm;
        ram_vars.e_filt = mySettings.filterMode;
        ram_vars.e_band = mySettings.band;
        ram_vars.e_replay_time = mySettings.autoReplay;
        eeprom_update_block(&ram_vars, &ee_vars, sizeof(eesave_t));
      } 
            
			// 1 second tick
			if (i == 200) {									
				serviceBattery();
				i = 0;
			}
		}
		
		if (battery_state == 4) {
		  // Battery is low, so disable tx
	    yackinhibit(ON);  
	    yackstring(batLowStr);
    }
	  else {
		  // Service keyer/transmitter code
      yackbeat();
      if (straight_key == 0) {
        yackiambic(OFF);
      }
      else {
        service_straight_key();
      }
    }
  }	
}

static void service_straight_key(void) {
  switch (sk_state)	{	
    // idle
    // TX not keyed, check if DITPIN == 0
    case 0:	
      {
        if (DITPIN == 0) {
          // Keyed after idle state
          //txKeyed = 1;
          sk_timer = 50;
          txKeyed = 1;
          
          transition_to_tx();
          
          //rxMUTE = 1;
          //cli();
          //si5351_clock_enable(SI5351_CLK2, 1);					
          //sei();  
                    
          key(DOWN);
          enableMemoryReplay = 0;                
          sk_state += 1;  
          break;
        }
        else {
          break;
        } 
      }

      // Gap between elements?
    case 1:
      {
        if (DITPIN == 0) {
          key(DOWN);
          sk_timer = 50;
          break;
        }
        else if (sk_timer > 0) {
          key(UP);                
          sk_timer--; // Count down   
          break;
        }
        // timer must be zero
        else {
          key(UP);
          sk_state = 2;
        }
      }     
    // End of tx 
    case 2:
      {
        key(UP);
        transistion_to_rx();
        //txKeyed = 0;
        //rxMUTE = 0;
        //yackbeat();
        //cli();
        //si5351_clock_enable(SI5351_CLK2, 0);					
        //sei(); 
        sk_state = 0;
      }    
  }
}       
 
/*************************************************************************
//Initialise
*************************************************************************/
static void initMicro(void) {
	DDRA = ddrPORTA; 
	DDRB = ddrPORTB;
	DDRC = ddrPORTC;
	DDRD = ddrPORTD;	
	// Set all Port D pins as HIGH
	PORTA = 0xF8;			//VSWR ADC low
	PORTB = 0xFF;
	PORTC = 0xFF;
	// TXKEY always off
	PORTD = 0xFB;

	//RFSWMINS = 0;
	//RFSWPLUS = 0;

	//Enable hardware interupt
	EICRA |= _BV(ISC21); //Trigger on falling edge of INT0
	PCICR |= _BV(PCIE2); // Pin Change Interrupt Enable 2, PCINT23..16 pin will cause an interrupt
	PCMSK2 |= _BV(PCINT23); 
	//EIMSK |= _BV(INT2);  //Enable INT0	
	
	/*
	CS02 CS01 CS00
	Description
	0 0 0 No clock source (Timer/Counter stopped)
	0 0 1 clk I/O /(No prescaling)
	0 1 0 clk I/O /8 (From prescaler)
	0 1 1 clk I/O /64 (From prescaler)
	1 0 0 clk I/O /256 (From prescaler)
	1 0 1 clk I/O /1024 (From prescaler)
	1 1 0 External clock source on T0 pin. Clock on falling edge.
	1 1 1 External clock source on T0 pin. Clock on rising edge.
	*/
	
	
	//ASSR=(0<EXCLK) | (0<AS2);
	// enable timer overflow interrupt for both Timer0 and Timer1
	// Timer 0 for sidetone
  TIMSK0=(1<<OCIE0A);
  TCCR0A = _BV(WGM01);  // Mode = CTC    
  //TIMSK0=(1<<TOIE0);
	// set timer0 counter initial value to 0
  TCNT0=0x00;
  // start timer0 with /64 prescaler
  TCCR0B = (1 << WGM01) | (0<<CS02) | (1<<CS01) | (1<<CS00);
  // 96 =  651 Hz
  // 100 = 625 Hz    	
  OCR0A = 96;
    
	//Timer1 for yack beat, prescale 64, count to 1250 then overflow 
	//using compare timer counter reg
	TIMSK1=(0<<TOIE1);
  TCNT1=0x00;
  // lets turn on 16 bit timer1 also with /64
  TCCR1B |=   (1 << WGM12) | (0 << CS12) | (1 << CS11) | (1 << CS10);
  //OCF1A = 1;
  OCR1A = 625;
}

/*************************************************************************
//Interrupts
*************************************************************************/
ISR(TIMER0_COMPA_vect ) {
	STPIN = ~STPIN;
}

ISR(PCINT2_vect ) {
	storedRotA = ROTA;
	storedRotB = ROTB;
	rotEncChange = 1;
	enableMemoryReplay = 0;
}	

/*************************************************************************
//Subroutines
*************************************************************************/

static void serviceFreq(struct systemSettings *setting) {
	uint32_t newFreq = 0;
	if (rotEncChange == 1) {
    if (storedRotA == storedRotB) {
      if ((setting->freq - setting->increment )> setting->lowerFreq) {			
        setting->freq -= setting->increment;
        setting->ritFreq -= setting->increment;
      }			
    }
    else if (storedRotA != storedRotB) {
      if ((setting->freq + setting->increment) < setting->upperFreq) {
        setting->freq += setting->increment;
        setting->ritFreq += setting->increment;
      }
    }
          
    //now update the new frequency
    newFreq = setting->freq;
      
    //cli();
    si5351aSetFrequency(newFreq);
    si5351aSetTXFrequency((newFreq / 4) - TXOFFSET);	
    //sei();
    displayFreq(newFreq, battery_state);
    ssd1306Refresh();

    rotEncChange = 0;
	}
}

static uint8_t serviceMenu(struct menuStateMachine *state, struct systemSettings *setting) {
	uint8_t click = 0;
	uint8_t inc = 0;
	uint8_t dec = 0;
	uint16_t newStep = 0;
	uint8_t i;
	uint8_t longclick = 0;
	uint8_t save = 0;
    
	if (ROTSEL == 0) {
		if (state->currentState == 0) {
			word timer = YACKSECS(1);

			while(timer && (ROTSEL == 0))  {
				timer--;
				yackbeat();
			}
			while (ROTSEL == 0) {};
			
			if (timer == 0) {
				longclick = 1;
				state->currentState = 3;
			}
			else {
				if (timer < 180) {
					click = 1;
				}				
			}
		}
		else {
			state->pbCounterOn += 1;
			if (state->pbCounterOn == 5) {
				click = 1;
			}
		}
	}
	else {
		state->pbCounterOn = 0;	
	}
		
	if (click == 1) {
		//Gone round the loop, but need to refresh to the current
		//frequency
		if ((state->currentState == 2) || (state->currentState == 11)) {
      yackinhibit(OFF);	
			state->currentState = 0;
			ssd1306ClearScreen();	
			displayFreq(setting->freq, battery_state);
			ssd1306Refresh();
		}
		else if (state->currentState == 1) {
			//In RIT, want to advance through menu, reset F back to non-RIT freq
			cli();
			si5351aSetFrequency(setting->freq);
			sei();
			state->currentState += 1;
		}
		else {
			state->currentState += 1;
		}		
	}	
	
	if (rotEncChange == 1) {
		if (storedRotA != storedRotB){
			inc = 1;
		}
		else if (storedRotA == storedRotB){
			dec = 1;
		}
		
		rotEncChange = 0;
	}
	    
	switch (state->currentState) {        
			case	Frequency: 	// 0
				{
				//
				break;
			}			
			case	RIT: 		// 1
				{
				if (inc == 1) {	
					setting->ritFreq += 40;
					si5351aSetFrequency(setting->ritFreq);
					clearMainArea();
					displayFreqSmall(setting->freq, setting->ritFreq);					
				}
				else if (dec == 1) {
					setting->ritFreq -= 40;
					si5351aSetFrequency(setting->ritFreq);
					clearMainArea();
					displayFreqSmall(setting->freq, setting->ritFreq);					
				}
				else if (click == 1) {
					clearMainArea();
					displayFreqSmall(setting->freq, setting->ritFreq);
				}
				break;
			}
			case  Vol: 		// 2
				{	
				if (inc == 1 && setting->volume != 9) {
					rxMUTE = 1;	
					volumeUPone();
					rxMUTE = 0;	
					setting->volume += 1;
				    clearMainArea();
				    drawIntNumber(setting->volume);	
				}
				else if (dec == 1 && setting->volume != 0) {
					rxMUTE = 1;
					volumeDOWNone();					
					rxMUTE = 0;	
					setting->volume -= 1;
					clearMainArea();
					drawIntNumber(setting->volume);						
					}
				else if (click == 1) {				
					clearMainArea();
					drawIntNumber(setting->volume);	
				}				
				break;
			}			
			case	FSTEP: 		// 3
				{			
				if (inc == 1 || dec == 1) {
					newStep = serviceStep(inc,dec,((setting->increment)/4));
					setting->increment = (newStep*4);		
					clearMainArea();								
					drawIntNumber((setting->increment)/4);
				}
				else if (longclick == 1) {		
          yackinhibit(ON);		
					clearMainArea();					
					drawIntNumber((setting->increment)/4);
				}
				break;
			}						
			case	Tune: 		// 4
				{
				if ((inc == 1) || (dec == 1)) {
					clearMainArea();
					drawMainArea(0);
          
					RFSWMINS = 0;
					RFSWPLUS = 0;                    
          yackbeat();
          RFSWMINS = 0;
					RFSWPLUS = 1;
          yackbeat();
					RFSWMINS = 0;
					RFSWPLUS = 0;                     
                    
          yacktune();
                    
          RFSWMINS = 1;
					RFSWPLUS = 0;
          yackbeat();
          RFSWMINS = 0;
					RFSWPLUS = 0;  
                    
          yackinhibit(OFF);	
          state->currentState = 0;
          ssd1306ClearScreen();	
          displayFreq(setting->freq, battery_state);
          ssd1306Refresh();    
          yackdelay(5);
				}		
				else if (click == 1) {
					clearMainArea();
					drawMainArea(0);
				}
				
				break;
			}		
			case  M1P: 		// 5			
				{	
				if ((inc == 1) || (dec == 1) || memoryReplayNow == 1) {
					clearMainArea();				
					memoryReplayNow = 0;

					drawMainArea(1);
					//ssd1306Refresh();
					displayChangeStateIcon(state->currentState);					
					ssd1306Refresh();
          if (inc == 1) {
            sprintf(cqStr, "CQ CQ CQ de");
          }
          else if (dec == 1) {
            sprintf(cqStr, "CQ SOTA CQ SOTA de");
          }
                    
					memoryCQ();	
					
					//###Put this into a seperate function
					state->currentState = 0;
					ssd1306ClearScreen();	
					displayFreq(setting->freq, battery_state);
					ssd1306Refresh();
					if (setting->autoReplay != 0) {
						enableMemoryReplay = 1;
						countDownMemoryTimer = 200 * setting->autoReplay;
					}
				}					
				else if (click ==1) {	
					clearMainArea();								
					drawMainArea(0);
				}			
				break;
			}	
			case  M1R: 		// 6
				{
				if ((inc == 1) || (dec == 1)) {
					clearMainArea();
					drawMainArea(2);
					//ssd1306Refresh();
					displayChangeStateIcon(state->currentState);					
					ssd1306Refresh();
	
					yackmessage(RECORD,1); 	
	
					//yackinhibit(ON);
					//yackmessage(PLAY,1);

						
					//###Put this into a seperate function
					state->currentState = 0;
					ssd1306ClearScreen();	
					displayFreq(setting->freq, battery_state);
					ssd1306Refresh();
					return 1;
				}					
				else if (click == 1) {				
					drawMainArea(0);
				}			
				break;
			}
			case  Wpm: 		// 7
				{		
				if (inc == 1 && wpm < MAXWPM) {
					wpm += 1;
					wpmcnt=(1200/YACKBEAT)/wpm; // Calculate speed	
					clearMainArea();
					drawIntNumber(wpm);
                    save = 1;                    
				}
				else if (dec == 1 && wpm > MINWPM) {
					wpm -= 1;
					wpmcnt=(1200/YACKBEAT)/wpm; // Calculate speed
					clearMainArea();
					drawIntNumber(wpm);
                    save = 1;
					}
				else if (click == 1) {	
					clearMainArea();
					drawIntNumber(wpm);								
					break;
				}						
		
				break;
			}	
			case  KMOD: 		// 8
				{
				if (inc == 1 || dec == 1) {
					setting->keyMode = serviceKeyerMode(inc,dec,setting->keyMode);				
					clearMainArea();	
					drawMainAreaSmall(setting->keyMode);
          save = 1;								
				}
				else if (click == 1) {		
					clearMainArea();		
					drawMainAreaSmall(setting->keyMode);
				}

				break;
			}
			case	Fil: 		// 8
				{			
				if (inc == 1 || dec == 1) {
					if (setting->filterMode == WIDE) {
						rxMUTE = 1;	
						for (i=0;i<8;i++) {
							filterDOWNone();
						}	
						rxMUTE = 0;	
						setting->filterMode = NARROW;
					}
					else if (setting->filterMode == NARROW) {
						rxMUTE = 1;	
						for (i=0;i<8;i++) {
							filterUPone();
						}					
						rxMUTE = 0;	
						setting->filterMode = WIDE;
					}	
                    save = 1;
					clearMainArea();
					drawMainAreaSmall(setting->filterMode + Narrow);					
				}
				
				else if (click == 1) {				
					clearMainArea();
					drawMainAreaSmall(setting->filterMode + Narrow);	
				}	

				break;
			}
			/*
            case	Batv: 		// 10
				{
				if (inc == 1 && setting->batLow <120) {
					setting->batLow += 1;
					clearMainArea();	
					drawBatLowNum(setting->batLow);									
				}
				else if (dec == 1 && setting->batLow != 0) {
					setting->batLow -= 1;
					clearMainArea();	
					drawBatLowNum(setting->batLow);						
					}
				else {	
					clearMainArea();								
					drawBatLowNum(setting->batLow);
				}			
				
				break;
			}
      */
			case	MAR: 		// 10
				{	
				if (inc == 1 && (setting->autoReplay < 10)) {
					setting->autoReplay += 1;
				    clearMainArea();
					drawIntNumber(setting->autoReplay);	
                    save = 1;			
				}
				else if (dec == 1 && (setting->autoReplay != 0)) {
					setting->autoReplay -= 1;
				    clearMainArea();
					drawIntNumber(setting->autoReplay);					
                    save = 1;
				}
				else if (click == 1) {	
				    clearMainArea();
					drawIntNumber(setting->autoReplay);
				}	
				break;
			}
			case	Band: 		// 11
				{	
				if (inc == 1 || dec == 1) {
					if (setting->band == BAND1) {
						setting->band = BAND2;
						ChangeBand(setting->band, &*setting);
					}
					else if (setting->band == BAND2) {
						setting->band = BAND1;
						ChangeBand(BAND1, &*setting);												
					}	
                    save = 1;
					clearMainArea();	
					drawIntNumber(setting->band);
				}
					

				else if (click == 1) {		
					clearMainArea();	
					drawIntNumber(setting->band);	
				}	
				break;   
			} 
		}		
	if ((inc == 1 || dec == 1) || (click == 1) || (longclick == 1)) {
		displayChangeStateIcon(state->currentState);
		ssd1306Refresh();		
	}
  
  return save;
}

uint16_t getVSWR(void) {
	//Vrev PA0 ADC0 
	//Vfwd PA1 ADC1
	uint16_t measVfwd;
	uint16_t measVrev;
	
	//****Vfwd
	//ADMUX |= (0<<MUX2) | (0<<MUX1) | (1<<MUX0); // switch ADC to 0000 ADC1
	ADMUX = 0b00000001;
	//ADCSRA |= (1 << ADATE);  // Set ADC to Free-Running Mode 
	ADCSRA |= (1 << ADEN);  // Enable ADC 
	ADCSRA |= (1 << ADSC);  // Start A2D Conversions 
	while (ADCSRA & (1<<ADSC));
	
	
	ADCSRA |= (1 << ADEN);  // Enable ADC 
	ADCSRA |= (1 << ADSC);  // Start A2D Conversions 
	while (ADCSRA & (1<<ADSC));
	
	uint8_t theLowADC = ADCL;
	uint16_t theTenBitResults = ADCH<<8 | theLowADC;
	
	float batteryVol;
	
	batteryVol = ((5 * (float)theTenBitResults)/1023);
	
	batteryVol = batteryVol * 1000;

	measVfwd = (uint16_t)batteryVol;

	//********************
	// V reflected
	//********************
	ADMUX = 0b00000000;
	ADCSRA |= (1 << ADEN);  // Enable ADC 
	ADCSRA |= (1 << ADSC);  // Start A2D Conversions 
	while (ADCSRA & (1<<ADSC));
	
	
	ADCSRA |= (1 << ADEN);  // Enable ADC 
	ADCSRA |= (1 << ADSC);  // Start A2D Conversions 
	while (ADCSRA & (1<<ADSC));
		
	theLowADC = ADCL;
	theTenBitResults = ADCH<<8 | theLowADC;

	batteryVol = ((5 * (float)theTenBitResults)/1023);
	
	batteryVol = batteryVol * 1000;

	measVrev = (uint16_t)batteryVol;
	
	// (Vf + Vr) / (Vf - Vr)
	float measSWR;
	
	uint16_t displaySWR;
	
	measSWR = ((float)measVfwd + (float)measVrev) / ((float)measVfwd - (float)measVrev);
	measSWR = measSWR * 10;
	measSWR = measSWR + 0.5;
	
	displaySWR = (uint16_t)measSWR;

	return displaySWR;
}

static void serviceBattery(void) {
	//Get ADC value from PA2/ADC2
	//Check if it is less than low bat warning
  // if it is audible beep
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Set ADC prescalar to 128 - 125KHz sample rate @ 16MHz 
	//ADCSRB |= (0 << ADTS0) | (0 << ADTS1) | (0 << ADTS2);
   
	ADMUX |= (0 << REFS0) | (0 << REFS1); // Set ADC reference to AVCC 
	ADMUX |= (0 << ADLAR); // Left adjust ADC result to allow easy 8 bit reading 
	ADMUX |= (0<<MUX4) |(0<<MUX3) | (0<<MUX2) | (1<<MUX1) | (0<<MUX0); // switch ADC to 0010 ADC2

	//ADCSRA |= (1 << ADATE);  // Set ADC to Free-Running Mode 
	ADCSRA |= (1 << ADEN);  // Enable ADC 
	ADCSRA |= (1 << ADSC);  // Start A2D Conversions 
	while (ADCSRA & (1<<ADSC));
	
	uint8_t theLowADC = ADCL;
	uint16_t theTenBitResults = ADCH<<8 | theLowADC;
	    
	if (theTenBitResults < 405 ) {
		battery_state = 4;
    }
	else if (theTenBitResults < 426 ) {
		battery_state = 3;
    }    
	else if (theTenBitResults < 433 ) {
		battery_state = 2;
  }        
	else if (theTenBitResults < 447 ) {
		battery_state = 1;
    }                 
  else {
    battery_state = 0;
  }
}

static uint16_t serviceStep(uint8_t up, uint8_t down, uint16_t step) {
  uint16_t newStep = step;
		
  if (up == 1) {
    switch(step) {        
      case	1: 		
        newStep = 10;
        break;
      case	10: 
        newStep = 50;
        break;
      case	50: 
        newStep = 100;
        break;
      case	100: 
        newStep = 250;
        break;
      case	250: 
        newStep = 1;
        break;								
    }
  }
  else {
    switch(step) {        
      case	1: 	
        newStep = 250;
        break;
      case	10: 
        newStep = 1;
        break;
      case	50: 
        newStep = 10;
        break;
      case	100: 
        newStep = 50;
        break;
      case	250: 
        newStep = 100;
        break;								
    }
  }
		
  return newStep;
}

static void ChangeBand(uint8_t band, struct systemSettings *setting) {
	switch(band) {        
    case	BAND2: 	
      BANDA = 0;
			BANDB = 0;
			BANDA = 1;
			BANDB = 0;
			yackbeat();	
			BANDA = 0;
			BANDB = 0;
			setting->freq = ((BAND2_F + TXOFFSET) * 4);
			setting->ritFreq = ((BAND2_F + TXOFFSET) * 4);
			setting->upperFreq = ((BAND2_UPPER*4) + (TXOFFSET * 4));
			setting->lowerFreq = ((BAND2_LOWER*4) + (TXOFFSET * 4));			
			break;
    case	BAND1: 
      BANDA = 0;
			BANDB = 0;
			BANDA = 0;
			BANDB = 1;
			yackbeat();	
			BANDA = 0;
			BANDB = 0;
			setting->freq = ((BAND1_F + TXOFFSET) * 4);
			setting->ritFreq = ((BAND1_F + TXOFFSET) * 4);
      setting->upperFreq = ((BAND1_UPPER*4) + (TXOFFSET * 4));
			setting->lowerFreq = ((BAND1_LOWER*4) + (TXOFFSET * 4));				
			break;							
  }	
  rotEncChange = 1;
  serviceFreq(&*setting);
}

static uint8_t serviceKeyerMode(uint8_t up, uint8_t down, uint8_t keyerMode) {
  uint8_t newMode;
  newMode = keyerMode;
		
  if (up == 1) {
    switch(keyerMode) {        
      case	5: 		
        newMode = 6;
        straight_key = 0;
        yackinhibit(OFF);                    
        yackmode(IAMBICB);
        break;
      case	6: 
        // Straight Key
				newMode = 7;
        yackinhibit(ON);
        straight_key = 1;
        break;
      case	7: 
        newMode = 8;
        straight_key = 0;
        yackinhibit(OFF);
        yackmode(ULTIMATIC);
        break;
      case	8: 
        newMode = 5;
        straight_key = 0;
        yackinhibit(OFF);
        yackmode(IAMBICA);
        break;							
    }
  }
  else if (down == 1) {
    switch(keyerMode) {        
      case	8: 
        // Straight key
        newMode = 7;
        yackinhibit(ON);
        straight_key = 1;
        break;
      case	7: 
        newMode = 6;
        yackinhibit(OFF);
        straight_key = 0;
        yackmode(IAMBICB);
        break;
      case	6: 
        newMode = 5;
        straight_key = 0;
        yackinhibit(OFF);
        yackmode(IAMBICA);
        break;
      case	5: 
        newMode = 8;
        straight_key = 0;
        yackinhibit(OFF);                    
        yackmode(ULTIMATIC);
        break;								
			}
  }
  else {
    switch(keyerMode) {        
      case	7: 
        // Straight key
        newMode = 7;
        yackinhibit(ON);
        straight_key = 1;
        break;
      case	6: 
        newMode = 6;
        yackinhibit(OFF);
        straight_key = 0;
        yackmode(IAMBICB);
        break;
      case	5: 
        newMode = 5;
        straight_key = 0;
        yackinhibit(OFF);
        yackmode(IAMBICA);
        break;
      case	8: 
        newMode = 8;
        straight_key = 0;
        yackinhibit(OFF);                    
        yackmode(ULTIMATIC);
        break;								
    }            
  }
		
  return newMode;
}

/*************************************************************************
//Volume:
* MCP4023T-202E/CH
* Filter:
* MCP4023T-503E/CH
*************************************************************************/
static void filterUPone(void) {
	//No EEPROM write, assumes up one is 1/8 of full volume
	uint8_t i;
	VOLUD = 1;
	yackbeat();
	FILCS = 0;	
	yackbeat();	
	for (i=0;i<8;i++) {	
		VOLUD = 1;	
		yackbeat();
		VOLUD = 0;				
		yackbeat();		
	}	
	VOLUD = 1;	
	FILCS = 1;		
}

static void filterDOWNone(void) {
	//No EEPROM write, assumes up down is 1/8 of full volume
	uint8_t i;
	VOLUD = 0;
	yackbeat();
	FILCS = 0;	
	yackbeat();	
	for (i=0;i<8;i++) {	
		VOLUD = 1;	
		yackbeat();
		VOLUD = 0;				
		yackbeat();		
	}	
	FILCS = 1;		
}

static void volumeUPone(void) {
	//No EEPROM write, assumes up one is 1/8 of full volume
	uint8_t i;
	VOLUD = 1;
	yackbeat();
	VOLCS = 0;	
	yackbeat();	
	for (i=0;i<8;i++) {	
		VOLUD = 1;	
		yackbeat();
		VOLUD = 0;				
		yackbeat();		
	}	
	VOLUD = 0;	
	VOLCS = 1;		
}

static void volumeDOWNone(void) {
	//No EEPROM write, assumes 'one' is 1/8 of full volume
	uint8_t i;
	VOLUD = 0;
	yackbeat();
	VOLCS = 0;	
	yackbeat();	
	for (i=0;i<8;i++) {	
		VOLUD = 1;	
		yackbeat();
		VOLUD = 0;				
		yackbeat();		
	}	
	VOLCS = 1;	
}

static void memoryCQ(void) {
	yackinhibit(OFF);
	
  //rxMUTE = 1;
	//cli();
	//si5351_clock_enable(SI5351_CLK2, 1);
	//sei();    
  transition_to_tx();
  
	char str1[50];
	
	//char cqStr[] = "CQ CQ CQ de"; // 
	const char overStr[] PROGMEM = "K"; // 

	sprintf(str1, "%s %s %s %s %s", cqStr, callStr, callStr, overStr);
	
	yackstring(str1);
	
	//yackinhibit(ON);
	//txKeyed = 0;
	//rxMUTE = 0;
	//cli();
	//si5351_clock_enable(SI5351_CLK2, 0);					
	//sei();	
  transistion_to_rx();    
}

void transistion_to_rx(void) {
  txKeyed = 0;
  rxMUTE = 0;
  cli();
  si5351_clock_enable(SI5351_CLK2, 0);					
  sei();
}

void transition_to_tx(void) {
  txKeyed = 1;  
  rxMUTE = 1;
  cli();
  si5351_clock_enable(SI5351_CLK2, 1);
  sei(); 
}

void service_vswr(void) {
  clearMainArea();
  displayChangeStateIcon(Tune);
  uint8_t vswr = getVSWR();
  drawVSWR(vswr);  
}

void disable_memory_replay(void) {
  enableMemoryReplay = 0;  
}
