#ifndef SI5351A_H
#define SI5351A_H

#include <inttypes.h>

#define SI5351_OUTPUT_ENABLE_CTRL			3
#define SI_CLK0_CONTROL	16			// Register definitions
#define SI_CLK1_CONTROL	17
#define SI_CLK2_CONTROL	18
#define SI_SYNTH_PLL_A	26
#define SI_SYNTH_PLL_B	34
#define SI_SYNTH_MS_0		42
#define SI_SYNTH_MS_1		50
#define SI_SYNTH_MS_2		58
#define SI_PLL_RESET		177

#define SI_R_DIV_1		0b00000000			// R-division ratio definitions
#define SI_R_DIV_2		0b00010000
#define SI_R_DIV_4		0b00100000
#define SI_R_DIV_8		0b00110000
#define SI_R_DIV_16		0b01000000
#define SI_R_DIV_32		0b01010000
#define SI_R_DIV_64		0b01100000
#define SI_R_DIV_128	0b01110000

#define SI_CLK_SRC_PLL_A	0b00000000
#define SI_CLK_SRC_PLL_B	0b00100000

#define XTAL_FREQ	27000000			// Crystal frequency

enum si5351_clock {SI5351_CLK0, SI5351_CLK1, SI5351_CLK2, SI5351_CLK3,
	                 SI5351_CLK4, SI5351_CLK5, SI5351_CLK6, SI5351_CLK7};

extern void si5351aSetFrequency(uint32_t frequency);
extern void si5351aSetInitFrequency(uint32_t frequency);
extern void si5351aSetInitTXFrequency(uint32_t frequency);
extern void si5351aSetTXFrequency(uint32_t frequency);
extern void si5351_clock_enable(enum si5351_clock clk, uint8_t enable);

#endif //SI5351A_H
