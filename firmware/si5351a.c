// 
// Author: Hans Summers, 2015
// Website: http://www.hanssummers.com
//
// A very very simple Si5351a demonstration
// using the Si5351a module kit http://www.hanssummers.com/synth
// Please also refer to SiLabs AN619 which describes all the registers to use
//
#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "i2c.h"
#include "si5351a.h"

//static void si5351aOutputOff(uint8_t clk);
static uint8_t si5351_write(uint8_t, uint8_t);
static uint8_t si5351_read(uint8_t, uint8_t *);
//
// Set up specified PLL with mult, num and denom
// mult is 15..90
// num is 0..1,048,575 (0xFFFFF)
// denom is 0..1,048,575 (0xFFFFF)
//
void setupPLL(uint8_t pll, uint8_t mult, uint32_t num, uint32_t denom) {
	uint32_t P1;					// PLL config register P1
	uint32_t P2;					// PLL config register P2
	uint32_t P3;					// PLL config register P3

	P1 = (uint32_t)(128 * ((float)num / (float)denom));
	P1 = (uint32_t)(128 * (uint32_t)(mult) + P1 - 512);
	P2 = (uint32_t)(128 * ((float)num / (float)denom));
	P2 = (uint32_t)(128 * num - denom * P2);
	P3 = denom;

	i2cSendRegister(pll + 0, (P3 & 0x0000FF00) >> 8);
	i2cSendRegister(pll + 1, (P3 & 0x000000FF));
	i2cSendRegister(pll + 2, (P1 & 0x00030000) >> 16);
	i2cSendRegister(pll + 3, (P1 & 0x0000FF00) >> 8);
	i2cSendRegister(pll + 4, (P1 & 0x000000FF));
	i2cSendRegister(pll + 5, ((P3 & 0x000F0000) >> 12) | ((P2 & 0x000F0000) >> 16));
	i2cSendRegister(pll + 6, (P2 & 0x0000FF00) >> 8);
	i2cSendRegister(pll + 7, (P2 & 0x000000FF));
}

//
// Set up MultiSynth with integer divider and R divider
// R divider is the bit value which is OR'ed onto the appropriate register, it is a #define in si5351a.h
//
void setupMultisynth(uint8_t synth, uint32_t divider, uint8_t rDiv) {
	uint32_t P1;					// Synth config register P1
	uint32_t P2;					// Synth config register P2
	uint32_t P3;					// Synth config register P3

	P1 = 128 * divider - 512;
	P2 = 0;							// P2 = 0, P3 = 1 forces an integer value for the divider
	P3 = 1;

	i2cSendRegister(synth + 0,   (P3 & 0x0000FF00) >> 8);
	i2cSendRegister(synth + 1,   (P3 & 0x000000FF));
	i2cSendRegister(synth + 2,   ((P1 & 0x00030000) >> 16) | rDiv);
	i2cSendRegister(synth + 3,   (P1 & 0x0000FF00) >> 8);
	i2cSendRegister(synth + 4,   (P1 & 0x000000FF));
	i2cSendRegister(synth + 5,   ((P3 & 0x000F0000) >> 12) | ((P2 & 0x000F0000) >> 16));
	i2cSendRegister(synth + 6,   (P2 & 0x0000FF00) >> 8);
	i2cSendRegister(synth + 7,   (P2 & 0x000000FF));
}

//
// Switches off Si5351a output
// Example: si5351aOutputOff(SI_CLK0_CONTROL);
// will switch off output CLK0
//
/*
static void si5351aOutputOff(uint8_t clk) {
	i2cSendRegister(clk, 0x80);		// Refer to SiLabs AN619 to see bit values - 0x80 turns off the output stage
}
*/

void si5351_clock_enable(enum si5351_clock clk, uint8_t enable) {
	uint8_t reg_val;

	if(si5351_read(SI5351_OUTPUT_ENABLE_CTRL, &reg_val) != 0)
	{
		return;
	}

	if(enable == 1)
	{
		reg_val &= ~(1<<(uint8_t)clk);
	}
	else
	{
		reg_val |= (1<<(uint8_t)clk);
	}

	si5351_write(SI5351_OUTPUT_ENABLE_CTRL, reg_val);
}

// Set CLK0 output ON and to the specified frequency
// Frequency is in the range 1MHz to 150MHz
// Example: si5351aSetFrequency(10000000);
// will set output CLK0 to 10MHz
//
// This example sets up PLL A
// and MultiSynth 0
// and produces the output on CLK0
//
void si5351aSetInitFrequency(uint32_t frequency) {
	uint32_t pllFreq;
	uint32_t xtalFreq = XTAL_FREQ;
	uint32_t l;
	float f;
	uint8_t mult;
	uint32_t num;
	uint32_t denom;
	uint32_t divider;

	divider = 900000000 / frequency;// Calculate the division ratio. 900,000,000 is the maximum internal 
									// PLL frequency: 900MHz
	if (divider % 2) divider--;		// Ensure an even integer division ratio

	pllFreq = divider * frequency;	// Calculate the pllFrequency: the divider * desired output frequency

	mult = pllFreq / xtalFreq;		// Determine the multiplier to get to the required pllFrequency
	l = pllFreq % xtalFreq;			// It has three parts:
	f = l;							// mult is an integer that must be in the range 15..90
	f *= 1048575;					// num and denom are the fractional parts, the numerator and denominator
	f /= xtalFreq;					// each is 20 bits (range 0..1048575)
	num = f;						// the actual multiplier is  mult + num / denom
	denom = 1048575;				// For simplicity we set the denominator to the maximum 1048575

									// Set up PLL A with the calculated multiplication ratio
	setupPLL(SI_SYNTH_PLL_A, mult, num, denom);
									// Set up MultiSynth divider 0, with the calculated divider. 
									// The final R division stage can divide by a power of two, from 1..128. 
									// reprented by constants SI_R_DIV1 to SI_R_DIV128 (see si5351a.h header file)
									// If you want to output frequencies below 1MHz, you have to use the 
									// final R division stage
	setupMultisynth(SI_SYNTH_MS_0, divider, SI_R_DIV_1);
									// Reset the PLL. This causes a glitch in the output. For small changes to 
									// the parameters, you don't need to reset the PLL, and there is no glitch
	i2cSendRegister(SI_PLL_RESET, 0xA0);	
									// Finally switch on the CLK0 output (0x4F)
									// and set the MultiSynth0 input to be PLL 								
	i2cSendRegister(SI_CLK0_CONTROL, 0x4f | SI_CLK_SRC_PLL_A);
}

void si5351aSetInitTXFrequency(uint32_t frequency) {
	uint32_t pllFreq;
	uint32_t xtalFreq = XTAL_FREQ;
	uint32_t l;
	float f;
	uint8_t mult;
	uint32_t num;
	uint32_t denom;
	uint32_t divider;

	divider = 900000000 / frequency;// Calculate the division ratio. 900,000,000 is the maximum internal 
									// PLL frequency: 900MHz
	if (divider % 2) divider--;		// Ensure an even integer division ratio

	pllFreq = divider * frequency;	// Calculate the pllFrequency: the divider * desired output frequency

	mult = pllFreq / xtalFreq;		// Determine the multiplier to get to the required pllFrequency
	l = pllFreq % xtalFreq;			// It has three parts:
	f = l;							// mult is an integer that must be in the range 15..90
	f *= 1048575;					// num and denom are the fractional parts, the numerator and denominator
	f /= xtalFreq;					// each is 20 bits (range 0..1048575)
	num = f;						// the actual multiplier is  mult + num / denom
	denom = 1048575;				// For simplicity we set the denominator to the maximum 1048575

									// Set up PLL A with the calculated multiplication ratio
	setupPLL(SI_SYNTH_PLL_B, mult, num, denom);
									// Set up MultiSynth divider 0, with the calculated divider. 
									// The final R division stage can divide by a power of two, from 1..128. 
									// reprented by constants SI_R_DIV1 to SI_R_DIV128 (see si5351a.h header file)
									// If you want to output frequencies below 1MHz, you have to use the 
									// final R division stage
	setupMultisynth(SI_SYNTH_MS_2, divider, SI_R_DIV_1);
									// Reset the PLL. This causes a glitch in the output. For small changes to 
									// the parameters, you don't need to reset the PLL, and there is no glitch
	i2cSendRegister(SI_PLL_RESET, 0xA0);	
									// Finally switch on the CLK0 output (0x4F)
									// and set the MultiSynth0 input to be PLL 								
	i2cSendRegister(SI_CLK2_CONTROL, 0x4f | SI_CLK_SRC_PLL_B);
}

void si5351aSetTXFrequency(uint32_t frequency) {
	uint32_t pllFreq;
	uint32_t xtalFreq = XTAL_FREQ;
	uint32_t l;
	float f;
	uint8_t mult;
	uint32_t num;
	uint32_t denom;
	uint32_t divider;

	divider = 900000000 / frequency;// Calculate the division ratio. 900,000,000 is the maximum internal 
									// PLL frequency: 900MHz
	if (divider % 2) divider--;		// Ensure an even integer division ratio

	pllFreq = divider * frequency;	// Calculate the pllFrequency: the divider * desired output frequency

	mult = pllFreq / xtalFreq;		// Determine the multiplier to get to the required pllFrequency
	l = pllFreq % xtalFreq;			// It has three parts:
	f = l;							// mult is an integer that must be in the range 15..90
	f *= 1048575;					// num and denom are the fractional parts, the numerator and denominator
	f /= xtalFreq;					// each is 20 bits (range 0..1048575)
	num = f;						// the actual multiplier is  mult + num / denom
	denom = 1048575;				// For simplicity we set the denominator to the maximum 1048575

									// Set up PLL A with the calculated multiplication ratio
	setupPLL(SI_SYNTH_PLL_B, mult, num, denom);
									// Set up MultiSynth divider 0, with the calculated divider. 
									// The final R division stage can divide by a power of two, from 1..128. 
									// reprented by constants SI_R_DIV1 to SI_R_DIV128 (see si5351a.h header file)
									// If you want to output frequencies below 1MHz, you have to use the 
									// final R division stage
	setupMultisynth(SI_SYNTH_MS_2, divider, SI_R_DIV_1);
									// Reset the PLL. This causes a glitch in the output. For small changes to 
									// the parameters, you don't need to reset the PLL, and there is no glitch
	//i2cSendRegister(SI_PLL_RESET, 0xA0);	
									// Finally switch on the CLK0 output (0x4F)
									// and set the MultiSynth0 input to be PLL 								
	i2cSendRegister(SI_CLK2_CONTROL, 0x4f | SI_CLK_SRC_PLL_B);
}

void si5351aSetFrequency(uint32_t frequency) {
	uint32_t pllFreq;
	uint32_t xtalFreq = XTAL_FREQ;
	uint32_t l;
	float f;
	uint8_t mult;
	uint32_t num;
	uint32_t denom;
	uint32_t divider;

	divider = 900000000 / frequency;// Calculate the division ratio. 900,000,000 is the maximum internal 
									// PLL frequency: 900MHz
	if (divider % 2) divider--;		// Ensure an even integer division ratio

	pllFreq = divider * frequency;	// Calculate the pllFrequency: the divider * desired output frequency

	mult = pllFreq / xtalFreq;		// Determine the multiplier to get to the required pllFrequency
	l = pllFreq % xtalFreq;			// It has three parts:
	f = l;							// mult is an integer that must be in the range 15..90
	f *= 1048575;					// num and denom are the fractional parts, the numerator and denominator
	f /= xtalFreq;					// each is 20 bits (range 0..1048575)
	num = f;						// the actual multiplier is  mult + num / denom
	denom = 1048575;				// For simplicity we set the denominator to the maximum 1048575

									// Set up PLL A with the calculated multiplication ratio
	setupPLL(SI_SYNTH_PLL_A, mult, num, denom);
									// Set up MultiSynth divider 0, with the calculated divider. 
									// The final R division stage can divide by a power of two, from 1..128. 
									// reprented by constants SI_R_DIV1 to SI_R_DIV128 (see si5351a.h header file)
									// If you want to output frequencies below 1MHz, you have to use the 
									// final R division stage
	setupMultisynth(SI_SYNTH_MS_0, divider, SI_R_DIV_1);
									// Reset the PLL. This causes a glitch in the output. For small changes to 
									// the parameters, you don't need to reset the PLL, and there is no glitch
	//i2cSendRegister(SI_PLL_RESET, 0xA0);	
									// Finally switch on the CLK0 output (0x4F)
									// and set the MultiSynth0 input to be PLL A
	i2cSendRegister(SI_CLK0_CONTROL, 0x4f | SI_CLK_SRC_PLL_A);
}

static uint8_t si5351_write(uint8_t addr, uint8_t data) {
	uint8_t stts;
	
	stts = i2c_start();
	if(stts != I2C_START)
	{
		i2c_stop();
		return 1;
	}
	stts = i2c_write(I2C_WRITE);
	if(stts != I2C_SLA_W_ACK)
	{
		i2c_stop();
		return 1;
	}
	stts = i2c_write(addr);
	if(stts != I2C_DATA_ACK)
	{
		i2c_stop();
		return 1;
	}
	stts = i2c_write(data);
	if(stts != I2C_DATA_ACK)
	{
		i2c_stop();
		return 1;
	}
	i2c_stop();
	return 0;
}

static uint8_t si5351_read(uint8_t addr, uint8_t *data) {
	uint8_t stts;
	
	stts = i2c_start();
	if(stts != I2C_START)
	{
		i2c_stop();
		return 1;
	}
	
	stts = i2c_write(I2C_WRITE);
	if(stts != I2C_SLA_W_ACK)
	{
		i2c_stop();
		return 1;
	}
	
	stts = i2c_write(addr);
	if(stts != I2C_DATA_ACK)
	{
		i2c_stop();
		return 1;
	}

	stts = i2c_start();
	if(stts != I2C_START_RPT)
	{
		i2c_stop();
		return 1;
	}
	
	stts = i2c_write(I2C_READ);
	if(stts != I2C_SLA_R_ACK)
	{
		i2c_stop();
		return 1;
	}
	*data = i2c_read_nack();

    /*
	if(i2c_status() != TW_MR_DATA_NACK)
	{
		i2c_stop();
		return 1;
	}
	*/
	i2c_stop();
	return 0;
}
