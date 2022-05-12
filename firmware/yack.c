/*!
 
 @file      yack.c
 @brief     CW Keyer library
 @author    Jan Lategahn DK3LJ jan@lategahn.com (C) 2011; modified by Jack Welch AI4SV;
            MODIFIED by M5EVT 2015-2020 for Firecrest tcvr
 
 @version   0.75
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
 @date      15.10.2010  - Created
 @date      03.10.2013  - Last update
 
 @todo      Make the delay dependent on T/C 1 

 MODIFIED by M5EVT 2015-2020 for Firecrest tcvr
*/ 
#include <avr/io.h> 
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <stdint.h>
#include <string.h>

#include "firecrest.h"
#include "yack.h"

// Forward declaration of private functions
static char morsechar(byte buffer);
static void keylatch(void);
static void yackchar(char c);
static void yackerror(void);

// Enumerations
enum FSMSTATE { 
  IDLE,   //!< Not keyed, waiting for paddle
  KEYED,  //!< Keyed, waiting for duration of current element
  IEG     //!< In Inter-Element-Gap 
};   

// Module local definitions
static		byte	yackflags;		// Permanent (stored) status of module flags
static		byte	volflags=0;		// Temporary working flags (volatile)
static 		word	ctcvalue;		  // Pitch
static    byte  farnsworth;   // Additional Farnsworth pause

// Flash data

//! Morse code table in Flash

//! Encoding: Each byte is read from the left. 0 stands for a dot, 1
//! stands for a dash. After each played element the content is shifted
//! left. Playback stops when the leftmost bit contains a "1" and the rest
//! of the bits are all zero.
//!
//! Example: A = .-
//! Encoding: 01100000
//!           .-
//!             | This is the stop marker (1 with all trailing zeros)
const byte morse[] PROGMEM = {
	0b11111100, // 0
	0b01111100, // 1
	0b00111100, // 2
	0b00011100, // 3
	0b00001100, // 4
	0b00000100, // 5
	0b10000100, // 6
	0b11000100, // 7
	0b11100100, // 8
	0b11110100, // 9
	0b01100000, // A
	0b10001000, // B
	0b10101000, // C
	0b10010000, // D
	0b01000000, // E
	0b00101000, // F
	0b11010000, // G
	0b00001000, // H
	0b00100000, // I                                
	0b01111000, // J
	0b10110000, // K
	0b01001000, // L
	0b11100000, // M
	0b10100000, // N
	0b11110000, // O
	0b01101000, // P
	0b11011000, // Q
	0b01010000, // R
	0b00010000, // S
	0b11000000, // T
	0b00110000, // U
	0b00011000, // V
	0b01110000, // W
	0b10011000, // X
	0b10111000, // Y
	0b11001000, // Z
	0b00110010, // ?
	0b10001100, // =
	0b01010110, // .
	0b00010110, // SK
	0b01010100, // AR
	0b10010100  // /
};

// The special characters at the end of the above table can not be decoded
// without a small table to define their content. # stands for SK, $ for AR

// To add new characters, add them in the code table above at the end and below
// Do not forget to increase the legth of the array..
const char spechar[6] PROGMEM = "?=.#$/";

// Functions

// ***************************************************************************
// Control functions
// ***************************************************************************
/*! 
 @brief     Sets all yack parameters to standard values

 This function resets all YACK EEPROM settings to their default values as 
 stored in the .h file. It sets the dirty flag and calls the save routine
 to write the data into EEPROM immediately.
*/
void yackreset(void) {
	ctcvalue=DEFCTC; // Initialize to 700 Hz
	wpmcnt=(1200/YACKBEAT)/wpm; // default speed
	yackflags = FLAGDEFAULT;  
	volflags |= DIRTYFLAG;
}

/*! 
 @brief     Initializes the YACK library
 
 This function initializes the keyer hardware according to configurations in the .h file.
 Then it attempts to read saved configuration settings from EEPROM. If not possible, it
 will reset all values to their defaults.
 This function must be called once before the remaining fuctions can be used.
*/
void yackinit(void) {
	yackreset();
	yackinhibit(OFF);
}

/*! 
 @brief     Saves all permanent settings to EEPROM
 
 To save EEPROM write cycles, writing only happens when the flag DIRTYFLAG is set.
 After writing the flag is cleared
 
 @callergraph
 
 */
/*
void yacksave (void)
{	
	if(volflags & DIRTYFLAG) // Dirty flag set?
	{	
		

		eeprom_write_word(&ctcstor, ctcvalue);
		eeprom_write_byte(&wpmstor, wpm);
		eeprom_write_byte(&flagstor, yackflags);
        eeprom_write_byte(&fwstor, farnsworth);
		
		volflags &= ~DIRTYFLAG; // Clear the dirty flag
	}
}
*/

/*! 
 @brief     Inhibits keying during command phases
 This function is used to inhibit and re-enable TX keying (if configured) and enforce the internal 
 sidetone oscillator to be active so that the user can communicate with the keyer.
 @param mode   ON inhibits keying, OFF re-enables keying 
 */
void yackinhibit(byte mode) {
	if (mode) {
		volflags &= ~(TXKEY | SIDETONE);
		volflags |= SIDETONE;
	}
	
	else {
		volflags &= ~(TXKEY | SIDETONE);
		volflags |= (yackflags & (TXKEY | SIDETONE));
    key(UP);
	}
}

/*! 
 @brief     Saves user defined settings
 
 The routine using this library is given the opportunity to save up to two 16 bit sized
 values in EEPROM. In case of the sample main function this is used to store the beacon interval 
 timer value. The routine is not otherwise used by the library.
 
 @param func    States if the data is retrieved (READ) or written (WRITE) to EEPROM
 @param nr      1 or 2 (Number of user storage to access)
 @param content The 16 bit word to write. Not used in read mode.
 @return        The content of the retrieved value in read mode.
 
 */
/*
word yackuser (byte func, byte nr, word content)
{
	if (func == READ) {
		if (nr == 1) 
			return (eeprom_read_word(&user1));
		else if (nr == 2)
			return (eeprom_read_word(&user2));
	}
	
	if (func == WRITE) {
		if (nr == 1)
			eeprom_write_word(&user1, content);
		else if (nr == 2)
			eeprom_write_word(&user2, content);
	}

    return (FALSE);
}
*/

/*! 
 @brief     Retrieves the current WPM speed
 
 This function delivers the current WPM speed. 

 @return        Current speed in WPM
 
 */
word yackwpm(void) {
  return wpm; 
}

/*! 
 @brief     Increases or decreases the current WPM speed
 The amount of increase or decrease is in amounts of wpmcnt. Those are close to real
 WPM in a 10ms heartbeat but can significantly differ at higher heartbeat speeds.
 @param dir     UP (faster) or DOWN (slower)
 */
void yackspeed(byte dir, byte mode) {
  if (mode == FARNSWORTH) {
    if ((dir == UP) && (farnsworth > 0)) farnsworth--;
        
    if ((dir == DOWN) && (farnsworth < MAXFARN)) farnsworth++;
  }
  else { // WPMSPEED 
    if ((dir == UP) && (wpm < MAXWPM)) wpm++;
        
    if ((dir == DOWN) && (wpm > MINWPM)) wpm--;
        
    wpmcnt=(1200/YACKBEAT)/wpm; // Calculate beats
	}
	
	volflags |= DIRTYFLAG; // Set the dirty flag	
    
  yackplay(DIT);
  yackdelay(IEGLEN);	// Inter Element gap  
  yackplay(DAH);
  yackdelay(ICGLEN);	// Inter Character gap  
}

/*! 
 @brief     Heartbeat delay
 Several functions in the keyer are timing dependent. The most prominent example is the
 yackiambic function that implements the IAMBIC keyer finite state machine.
 The same expects to be called in intervals of YACKBEAT milliseconds. How this is 
 implemented is left to the user. In a more complex application this would be done
 using an interrupt or a timer. For simpler cases this is a busy wait routine
 that delays exactly YACKBEAT ms.
 */
void yackbeat(void) {
	while((TIFR1 & (1<<OCF1A)) == 0) {} 	// Wait for Timeout
	TIFR1 |= (1<<OCF1A);       		// Reset output compare flag
}

/*! 
 @brief     Increases or decreases the sidetone pitch
 Changes are done not in Hz but in ctc control values. This is to avoid extensive 
 calculations at runtime. As is all calculations are done by the preprocessor.
 @param dir     UP or DOWN
 
 */
/*
void yackpitch (byte dir)
{
	if (dir == UP)
		ctcvalue--;
	if (dir == DOWN)
		ctcvalue++;
	
	if (ctcvalue < MAXCTC)
		ctcvalue = MAXCTC;
	
	if (ctcvalue > MINCTC)
		ctcvalue = MINCTC;
	
	volflags |= DIRTYFLAG; // Set the dirty flag	
}
*/
/*! 
 @brief     Activates Tuning mode
 This produces a solid keydown for TUNEDURATION seconds. After this the TX is unkeyed.
 The same can be achieved by presing either the DIT or the DAH contact or the control key.
*/
void yacktune(void) {
	word timer = YACKSECS(TUNEDURATION);
	
	uint16_t i=0;

	yackinhibit(OFF);
    
  // Setup to be ready to key the tx
  //rxMUTE = 1;
	//cli();
	//si5351_clock_enable(SI5351_CLK2, 1);
	//sei();    
    
  transition_to_tx();    
    
	key(DOWN);
	while(timer && (DITPIN != 0) && (DAHPIN != 0)) {
		timer--;
		yackbeat();
		i++;
		if (i==50) {
			//clearMainArea();
      //displayChangeStateIcon(Tune);
			//vswr = getVSWR();
			//drawVSWR(vswr);
      
      service_vswr();
      
			//ssd1306Refresh();
			i=0;
		}
	}
	
	key(UP);
    
  //cli();
	//si5351_clock_enable(SI5351_CLK2, 0);					
	//sei();	
  
  transistion_to_rx();
    
    
  //rxMUTE = 0;
}

/*! 
 @brief     Sets the keyer mode (e.g. IAMBIC A)
 This allows to set the content of the two mode bits in yackflags. Currently only
 two modes are supported, IAMBIC A and IAMBIC B.
 @param mode    IAMBICA or IAMBICB
 @return    TRUE is all was OK, FALSE if configuration lock prevented changes
 */
void yackmode (byte mode) {
	yackflags &= ~MODE;
	yackflags |= mode;
	
	volflags |= DIRTYFLAG; // Set the dirty flag	
}

/*! 
 @brief     Query feature flags
 
 @param flag A byte which indicate which flags are to be queried 
 @return     0 if the flag(s) were clear, >0 if flag(s) were set
 
 */
byte yackflag(byte flag) {
  return yackflags & flag;
}

/*! 
 @brief     Toggle feature flags
 When passed one (or more) flags, this routine flips the according bit in yackflags and
 thereby enables or disables the corresponding feature.
 @param flag    A byte where any bit to toggle is set e.g. SIDETONE 
 @return    TRUE if all was OK, FALSE if configuration lock prevented changes
 
 */
void yacktoggle(byte flag) {
  yackflags ^= flag;      // Toggle the feature bit
  volflags |= DIRTYFLAG;  // Set the dirty flag	
}

/*! 
 @brief     Creates a series of 8 dits
 The error prosign (8 dits) can not be encoded in our coding table. A call to this
 function produces it..
 */
void yackerror(void) {
	byte i;
	
	for (i=0;i<8;i++) {
		yackplay(DIT);
		yackdelay(DITLEN);
	}
	yackdelay(DAHLEN);
}

// ***************************************************************************
// CW Playback related functions
// ***************************************************************************
/*! 
 @brief     Keys the transmitter and produces a sidetone
 .. but only if the corresponding functions (TXKEY and SIDETONE) have been set in
 the feature register. This function also handles a request to invert the keyer line
 if necessary (TXINV bit).
 This is a private function.
 @param mode    UP or DOWN
 */
void key(byte mode) {
    if (mode == DOWN) {
      if (volflags & SIDETONE) // Are we generating a Sidetone?
        {
			// enable timer overflow interrupt for both Timer0 and Timer1
			TIMSK0=(1<<OCIE0A);
			/*
			TCCR0A = _BV(WGM01);  // Mode = CTC    
			//TIMSK0=(1<<TOIE0);
			// set timer0 counter initial value to 0
			TCNT0=0x00;
			// start timer0 with /1024 prescaler
			TCCR0B = (1 << WGM01) | (0<<CS02) | (1<<CS01) | (1<<CS00);
    	
			OCR0A = 200;
			*/
        }
        
        if (volflags & TXKEY) // Are we keying the TX?
        {
            if (yackflags & TXINV) // Do we need to invert keying?
                KEYTX = 0;
            else
                KEYTX = 1;
        }

    }
    
    if (mode == UP) 
    {
        if (volflags & SIDETONE) // Sidetone active?
        {
            TIMSK0=(0<<OCIE0A);
        }
        
        if (volflags & TXKEY) // Are we keying the TX?
        {
            if (yackflags & TXINV) // Do we need to invert keying?
                KEYTX = 1;
            else
                KEYTX = 0;
        }
    }   
}

/*! 
 @brief     Produces an active waiting delay for n dot counts
 
 This is used during the playback functions where active waiting is needed
 
 @param n   number of dot durations to delay (dependent on current keying speed!
 
 */
void yackdelay(byte n) {	
	byte i=n;
	byte x;
	
	while (i--) {
		x=wpmcnt;
		while (x--)    
			yackbeat();
	}
}

/*! 
 @brief     Key the TX / Sidetone for the duration of a dit or a dah
 @param i   DIT or DAH
 */
void yackplay(byte i) {
  key(DOWN); 
	switch (i) {
		case DAH:
			yackdelay(DAHLEN);
			break;
			
		case DIT:
			yackdelay(DITLEN);
			break;
	}
  key(UP);	
}

/*! 
 @brief     Send a character in morse code
 
 This function translates a character passed as parameter into morse code using the 
 translation table in Flash memory. It then keys transmitter / sidetone with the characters
 elements and adds all necessary gaps (as if the character was part of a longer word).
 
 If the character can not be translated, nothing is sent.
 
 If a space is received, an interword gap is sent.
  
 @param c   The character to send
*/
void yackchar(char c) {
	byte	code=0x80; // 0x80 is an empty morse character (just eoc bit set)
	byte 	i; // a counter
	
	// First we need to map the actual character to the encoded morse sequence in
	// the array "morse"
	if(c>='0' && c<='9') // Is it a numerical digit?
		code = pgm_read_byte(&morse[c-'0']); // Find it in the beginning of array
    
	if(c>='a' && c<='z') // Is it a character?
		code = pgm_read_byte(&morse[c-'a'+10]); // Find it from position 10
	
	if(c>='A' && c<='Z') // Is it a character in upper case?
		code = pgm_read_byte(&morse[c-'A'+10]); // Same as above
	
	// Last we need to handle special characters. There is a small char
	// array "spechar" which contains the characters for the morse elements
	// at the end of the "morse" array (see there!)
	for(i=0;i<sizeof(spechar);i++) // Read through the array
		if (c == pgm_read_byte(&spechar[i])) // Does it contain our character
			code = pgm_read_byte(&morse[i+36]); // Map it to morse code
	
	if(c==' ') // Do they want us to transmit a space (a gap of 7 dots)
		yackdelay(IWGLEN-ICGLEN); // ICG was already played after previous char
	else
	{
  		while (code != 0x80) // Stop when EOC bit has reached MSB
  		{
			//if (yackctrlkey(FALSE)) // Stop playing if someone pushes key
			//	return;
			
     		if (code & 0x80) 	// MSB set ?
       			yackplay(DAH);      // ..then play a dash
     		else				// MSB cleared ?
       			yackplay(DIT);		// .. then play a dot
			
     		yackdelay(IEGLEN);	// Inter Element gap  
            
     		code = code << 1;	// Shift code on position left (to next element)
  		}
		
  		yackdelay(ICGLEN - IEGLEN); // IEG was already played after element
	}
	
}

/*! 
 @brief     Sends a 0-terminated string in CW which resides in Flash
 
 Reads character by character from flash, translates into CW and keys the transmitter
 and/or sidetone depending on feature bit settings.
 
 @param p   Pointer to string location in FLASH 
 
 */
void yackstring(char *p) {
	uint8_t myStringLen;
	
	myStringLen = strlen(p);
	
	uint8_t i;
	
	i = 0;
	
	while((i<myStringLen) && ((DITPIN != 0) && (DAHPIN != 0))) {
		yackchar(p[i]);            // Play the read character		
		i++;
		// i points successively to a[0], a[1], ... until a '\0' is observed.
	}
}

/*! 
 @brief     Sends a number in CW
 
 Transforms a number up to 65535 into its digits and sends them in CW
 
 @param n   The number to send
 
 */
void yacknumber(word n) {
  char buffer[5];
  byte i = 0;
	
	while (n) // Until nothing left
	{
		buffer[i++] = n%10+'0'; // Store rest of division by 10
		n /= 10;                // Divide by 10
	}
	
  while (i) {
    yackchar(buffer[--i]);
  }
    
  yackchar (' ');
}

// ***************************************************************************
// CW Keying related functions
// ***************************************************************************
/*! 
 @brief     Latches the status of the DIT and DAH paddles
 
 If either DIT or DAH are keyed, this function sets the corresponding bit in 
 volflags. This is used by the IAMBIC keyer to determine which element needs to 
 be sounded next.
 
 This is a private function.

 */
static void keylatch(void) {
	byte	swap;	 // Status of swap flag
	
	swap    = ( yackflags & PDLSWAP);

	//if (!( KEYINP & (1<<DITPIN))) {
	if (DITPIN == 0) volflags |= (swap?DAHLATCH:DITLATCH);
	
	//if (!( KEYINP & (1<<DAHPIN)))
	if (DAHPIN == 0) volflags |= (swap?DITLATCH:DAHLATCH);	
}

/*! 
 @brief     Scans for the Control key
 
 This function is regularly called at different points in the program. In a normal case
 it terminates instantly. When the command key is found to be closed, the routine idles
 until it is released again and returns a TRUE return value.
 
 If, during the period where the contact was closed one of the paddles was closed too,
 the wpm speed is changed and the keypress not interpreted as a Control request. 

 @param mode    TRUE if caller has taken care of command key press, FALSE if not
 @return        TRUE if a press of the command key is not yet handled. 
 
 @callergraph
 
 */
 
/*! 
 @brief     Reverse maps a combination of dots and dashes to a character
 
 This routine is passed a sequence of dots and dashes in the format we use for morse
 character encoding (see top of this file). It looks up the corresponding character in
 the Flash table and returns it to the caller. 
 
 This is a private function.
 
 @param buffer    A character in YACK CW notation
 @return          The mapped character or /0 if no match was found  
 
 */
static char morsechar(byte buffer) {
	byte i;
	
	for(i=0;i<sizeof(morse);i++)
	{
		if (pgm_read_byte(&morse[i]) == buffer)
		{
			if (i < 10) return ('0' + i); 		// First 10 chars are digits
			if (i < 36) return ('A' + i - 10); 	// Then follow letters
			return (pgm_read_byte(&spechar[i - 36])); // Then special chars
		}
	}
	return '\0';
}

/*! 
 @brief     Handles EEPROM stored CW messages (macros)
 
 When called in RECORD mode, the function records a message up to 100 characters and stores it in 
 EEPROM. The routine stops recording when timing out after DEFTIMEOUT seconds. Recording
 can be aborted using the control key. If more than 100 characters are recorded, the error prosign
 is sounded and recording starts from the beginning. After recording and timing out the message is played
 back once before it is stored. To erase a message, do not key one.
 
 When called in PLAY mode, the message is just played back. Playback can be aborted using the command
 key.
 
 @param     function    RECORD or PLAY
 @param     msgnr       1 or 2
 @return    TRUE if all OK, FALSE if lock prevented message recording
 
 */
void yackmessage(byte function, byte msgnr) {
	unsigned char	rambuffer[RBSIZE];  // Storage for the message
	unsigned char	c;					// Work character
	
	word			extimer = 0;		// Detects end of message (10 sec)
	
	byte 			i = 0;       		// Pointer into RAM buffer
	byte 			n;					// Generic counter

	uint8_t current_wpm;
	
	if (function == RECORD) {
		// Delay for 1200 yack beats to get ready to enter message
		for (uint16_t i = 0; i < 200; i++) {
			yackbeat();
		}
		yackinhibit(ON);
		yackstring("e e e e t");		
		
		// Slow down to 12 wpm to record
    current_wpm = wpm;
		wpm = 17;
		wpmcnt=(1200/YACKBEAT)/wpm; // Calculate speed	
					


		extimer = YACKSECS(DEFTIMEOUT);	// 5 Second until message end
	   	while(extimer--)	// Continue until we waited 10 seconds
   		{
			
			if ((c = yackiambic(ON))) // Check for a character from the key
			{
				rambuffer[i++] = c; // Add that character to our buffer
				extimer = YACKSECS(DEFTIMEOUT); // Reset End of message timer
			}
			
			if (i>=RBSIZE) // End of buffer reached?
			{
				yackerror();
				i = 0;
			}
			
			yackbeat(); // 10 ms heartbeat
		}	
		
		// Extimer has expired. Message has ended
		
		
		
		if(i) // Was anything received at all?
		{
			wpm = current_wpm;
			wpmcnt=(1200/YACKBEAT)/wpm; // Calculate speed	
												
			rambuffer[--i] = 0; // Add a \0 end marker over last space
			
			//Clear out the callsign buffer
			memset(callStr, 0, sizeof callStr);
			
			// Replay the message
			for (n=0;n<i;n++) {
				yackchar(rambuffer[n]);
				callStr[n] = rambuffer[n];
			}
			// Add a \0 end marker over last space
			callStr[n+1] = 0;

		}
		else
			yackerror();
	}
	
	yackinhibit(OFF);
}

/*! 
 @brief     Finite state machine for the IAMBIC keyer
 
 If IAMBIC (squeeze) keying is requested, this routine, which usually terminates
 immediately needs to be called in regular intervals of YACKBEAT milliseconds.
 
 This can happen though an outside busy waiting loop or a counter mechanism.
 
 @param ctrl    ON if the keyer should recognize when a word ends. OFF if not.
 @return        The character if one was recognized, /0 if not
 
 */
char yackiambic(byte ctrl) {	
	static enum FSMSTATE	fsms = IDLE;	// FSM state indicator
	static 		word		timer;			// A countdown timer
	static		byte		lastsymbol;		// The last symbol sent
	static		byte		buffer = 0;		// A place to store a sent char
	static		byte		bcntr = 0;		// Number of elements sent
	static		byte		iwgflag = 0;	// Flag: Are we in interword gap?
  static    byte    ultimem = 0;    // Buffer for last keying status
	char		retchar;		// The character to return to caller

	// This routine is called every YACKBEAT ms. It starts with idle mode where
	// the morse key is polled. Once a contact close is sensed, the TX key is 
	// closed, the sidetone oscillator is fired up and the FSM progresses
	// to the next state (KEYED). There it waits for the timer to expire, 
	// afterwards progressing to IEG (Inter Element Gap).
	// Once the IEG has completed, processing returns to the IDLE state.
	
	// If the FSM remains in idle state long enough (one dash time), the
	// character is assumed to be complete and a decoding is attempted. If
	// succesful, the ascii code of the character is returned to the caller
	
	// If the FSM remains in idle state for another 4 dot times (7 dot times 
	// altogether), we assume that the word has ended. A space char
	// is transmitted in this case.
	
	if (timer) timer--; // Count down
	
	if (ctrl == OFF) iwgflag = 0; // No space detection
	
	switch (fsms) {	
		case IDLE:			
      keylatch();           
      // Handle latching logic for various keyer modes
      switch (yackflags & MODE) {
        case IAMBICA:
        case IAMBICB:
          // When the paddle keys are squeezed, we need to ensure that
          // dots and dashes are alternating. To do that, whe delete
          // any latched paddle of the same kind that we just sent.
          // However, we only do this ONCE
					
          volflags &= ~lastsymbol;
          lastsymbol = 0;
                                       
          break;                    
                 
        case ULTIMATIC:
          // Ultimatic logic: The last paddle to be active will be repeated indefinitely
          // In case the keyer is squeezed right out of idle mode, we just send a DAH 
          if ((volflags & SQUEEZED) == SQUEEZED) {
            if (ultimem) {
              volflags &= ~ultimem; // Opposite symbol from last one
            }
            else {
              volflags &= ~DITLATCH; // Reset the DIT latch
            }
          }
          else {
            ultimem = volflags & SQUEEZED; // Remember the last single key
          }

          break;
                            
        case DAHPRIO:            
          // If both paddles pressed, DAH is given priority
          if ((volflags & SQUEEZED) == SQUEEZED) {
            volflags &= ~DITLATCH; // Reset the DIT latch
          }
          break;
      }        
                
			// The following handles the inter-character gap. When there are
			// three (default) dot lengths of space after an element, the
			// character is complete and can be returned to caller
      // Have we idled for 3 dots and is there something to decode?
			if (timer == 0 && bcntr != 0) {
				buffer = buffer << 1;	  // Make space for the termination bit
				buffer |= 1;			  // The 1 on the right signals end
				buffer = buffer << (7-bcntr); // Shift to left justify
				retchar = morsechar(buffer); // Attempt decoding
				buffer = bcntr = 0;			// Clear buffer
				timer = (IWGLEN - ICGLEN) * wpmcnt;	// If 4 further dots of gap,
				// this might be a Word gap.
				iwgflag = 1;                // Signal we are waiting for IWG                          
				return (retchar);			// and return decoded char
			}
			
			// This handles the Inter-word gap. Already 3 dots have been
			// waited for, if 4 more follow, interpret this as a word end
			if (timer == 0 && iwgflag) { // Have we idled for 4+3 = 7 dots?
				iwgflag = 0;   // Clear Interword Gap flag
				return (' ');  // And return a space
			}
			
			// Now evaluate the latch and determine what to send next
			if ( volflags & (DITLATCH | DAHLATCH)) { // Anything in the latch?
				iwgflag = 0; // No interword gap if dit or dah
        bcntr++;	// Count that we will send something now
				buffer = buffer << 1; // Make space for the new character
				
        // Is it a dit?
				if (volflags & DITLATCH) {
					timer	= DITLEN * wpmcnt; // Duration = one dot time
					lastsymbol = DITLATCH; // Remember what we sent
				}
				else { 
          // must be a DAH then..
					timer	= DAHLEN * wpmcnt; // Duration = one dash time
					lastsymbol = DAHLATCH; // Remember
					buffer |= 1; // set LSB to remember dash
				}

				if (ctrl == OFF) {
          transition_to_tx();
					//rxMUTE = 1;
					//cli();
					//si5351_clock_enable(SI5351_CLK2, 1);
          //sei();
				}
				
				key(DOWN); // Switch on the side tone and TX
				
				volflags &= ~(DITLATCH | DAHLATCH); // Reset both latches
				fsms 	= KEYED; // Change FSM state
				// TODO check commenting of this
        //if (ctrl == OFF) txKeyed = 1;
        
        disable_memory_replay();
				//enableMemoryReplay = 0;
			}
			
			break;
			
		case KEYED:

			if ((yackflags & MODE) == IAMBICB) // If we are in IAMBIC B mode
				keylatch();                      // then latch here already 
						
			if(timer == 0) { // Done with sounding our element?
				key(UP); // Then cancel the side tone

				timer	= IEGLEN * wpmcnt; // One dot time for the gap
				fsms	= IEG; // Change FSM state
			}			
			break;
				
		case IEG:
			keylatch();	// Latch any paddle movements (both A and B)
			
			if(timer == 0) // End of gap reached?
			{
				fsms	= IDLE; // Change FSM state
				// The following timer determines what the IDLE state
        // accepts as character. Anything longer than 2 dots as gap will be
        // accepted for a character end.
				timer	= (ICGLEN - IEGLEN -1) * wpmcnt; 
				
				if (ctrl == OFF) {
					//txKeyed = 0;
					//rxMUTE = 0;
					//cli();
					//si5351_clock_enable(SI5351_CLK2, 0);					
					//sei();
          transistion_to_rx();
				}
			}
			break;		
	}
	return '\0'; // Nothing to return if not returned in above routine
}
