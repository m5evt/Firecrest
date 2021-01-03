/* ********************************************************************
 Program	: yack.h
 Author		: Jan Lategahn DK3LJ modified by Jack Welch AI4SV
          : MODIFIED by M5EVT 2015-2020 for Firecrest tcvr
 Purpose	: definition of keyer hardware
 Created	: 15.10.2010
 Update		: 03.10.2013
 Version	: 0.75
 
 Changelog
 ---------
 Version		Date		Change
 ----------------------------------------------------------------------
  Todo
 ---- 
 *********************************************************************/
// The following defines the meaning of status bits in the yackflags and volflags 
// global variables

// Definition of the yackflags variable. These settings get stored in EEPROM when changed.
#ifndef YACK_H
#define YACK_H

#define		NOTUSED1		0b00000001  // Available
#define   CONFLOCK		0b00000010  // Configuration locked down
#define		MODE			  0b00001100  // 2 bits to define keyer mode (see next section)
#define		SIDETONE		0b00010000  // Set if the chip must produce a sidetone
#define		TXKEY			  0b00100000  // Set if the chip keys the transmitter
#define		TXINV			  0b01000000  // Set if TX key line is active low
#define		PDLSWAP			0b10000000  // Set if DIT and DAH are swapped

#define		IAMBICA			0b00000000  // IAMBIC A mode
#define		IAMBICB			0b00000100  // IAMBIC B mode (default)
#define		ULTIMATIC		0b00001000  // Ultimatic Mode
#define		DAHPRIO			0b00001100  // Always give DAH priority

#define		FLAGDEFAULT		IAMBICA | TXKEY | SIDETONE

// Definition of volflags variable. These flags do not get stored in EEPROM.
#define		DITLATCH		0b00000001  // Set if DIT contact was closed
#define   DAHLATCH		0b00000010  // Set if DAH contact was closed
#define   SQUEEZED    0b00000011  // DIT and DAH = squeezed
#define		DIRTYFLAG		0b00000100  // Set if cfg data was changed and needs storing
#define   CKLATCH     0b00001000  // Set if the command key was pressed at some point
#define		VSCOPY      0b00110000  // Copies of Sidetone and TX flags from yackflags


// The following defines timing constants. In the default version the keyer is set to operate in
// 10ms heartbeat intervals. If a higher resolution is required, this can be changed to a faster
// beat

// YACK heartbeat frequency (in ms)
#define		YACKBEAT		5
#define		YACKSECS(n)		(n*(1000/YACKBEAT)) // Beats in n seconds
#define		YACKMS(n)		(n/YACKBEAT) // Beats in n milliseconds

// These values limit the speed that the keyer can be set to
#define		MAXWPM			50  
#define		MINWPM			10
#define		DEFWPM			12

// Farnsworth parameters
#define     FARNSWORTH      1
#define     WPMSPEED        0
#define     MAXFARN         255

#define		WPMCALC(n)		((1200/YACKBEAT)/n) // Calculates number of beats in a dot 

#define		DITLEN			1	// Length of a dot
#define		DAHLEN			3	// Length of a dash
#define		IEGLEN			1	// Length of inter-element gap
#define		ICGLEN			3	// Length of inter-character gap
#define		IWGLEN			7	// Length of inter-word gap

// Duration of various internal timings in seconds
#define		TUNEDURATION	10  // Duration of tuning keydown (in seconds)
#define   DEFTIMEOUT    15   // Default timeout 5 seconds
#define   MACTIMEOUT    15  // Timeout after playing back a macro

// The following defines various parameters in relation to the pitch of the sidetone

// CTC mode prescaler.. If changing this, ensure that ctc config
// is adapted accordingly
#define		PRESCALE		8
#define		CTCVAL(n)		((F_CPU/n/2/PRESCALE)-1) // Defines how to compute CTC setting for
                                                     // a given frequency

// Default sidetone frequency
#define		DEFFREQ			600     // Default sidetone frequency
#define		MAXFREQ			1500    // Maximum frequency
#define		MINFREQ			400     // Minimum frequenc

#define		MAXCTC			CTCVAL(MAXFREQ) // CTC values for the above three values
#define		MINCTC			CTCVAL(MINFREQ) 
#define		DEFCTC			CTCVAL(DEFFREQ)

// The following are various definitions in use throughout the program
#define		RBSIZE			100     // Size of each of the two EEPROM buffers


#define		DIT				1
#define		DAH       2

#define		UP				1
#define		DOWN			2

#define		ON				1
#define		OFF				0

#define		RECORD		1
#define		PLAY			2

#define		READ			1
#define		WRITE			2

#define		TRUE      1
#define		FALSE     0


// Generic functionality
#define 	SETBIT(ADDRESS,BIT)     (ADDRESS |= (1<<BIT))
#define 	CLEARBIT(ADDRESS,BIT)   (ADDRESS &= ~(1<<BIT))

typedef		uint8_t         byte;
typedef		uint16_t        word;

word	wpmcnt;			  // Speed
byte  wpm;          // Real wpm

// Forward declarations of public functions
extern void yackinit (void);
extern void yackstring(char *p);
extern char yackiambic(byte ctrl);
extern void yacktune(void);
extern void yackmode(uint8_t mode);
extern void yackinhibit(uint8_t mode);
extern void yacktoggle(byte flag);
extern byte yackflag(byte flag);
extern void yackmessage(byte function, byte msgnr);
extern void yackreset(void);
extern void yacknumber(word n);
extern word yackwpm(void);
extern void yackplay(byte i);
extern void yackspeed (byte dir, byte mode);

extern void yackbeat(void);
void yackdelay(byte n);
void key(byte mode);

#endif
