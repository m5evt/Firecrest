#include <inttypes.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include "i2c.h"


static uint8_t i2c_read_ack(void);
// Init TWI (I2C)
//
void i2c_init(void) {
	TWBR = 12;						
	TWSR = 0;
	TWDR = 0xFF;
	
	PRR0 = 0;	//Change to PRR0 for ATMEGA324PA
	//PRR = 0;
}

uint8_t i2c_start(void) {
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);

	while (!(TWCR & (1<<TWINT))) ;

	return (TWSR & 0xF8);
}

void i2c_stop(void) {
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);

	while ((TWCR & (1<<TWSTO))) ;
}

uint8_t i2c_write(uint8_t data) {
	TWDR = data;

	TWCR = (1<<TWINT) | (1<<TWEN);

	while (!(TWCR & (1<<TWINT))) ;

	return (TWSR & 0xF8);
}

static uint8_t i2c_read_ack(void) {
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);
	while (!(TWCR & (1<<TWINT)));
	return TWDR;
}

uint8_t i2c_read_nack(void) {
	TWCR = (1<<TWINT) | (1<<TWEN);
	while (!(TWCR & (1<<TWINT))) ;
	return (TWDR);
}

uint8_t i2c_status(void) {
	return TWSR & 0xF8;
}

uint8_t i2cSendRegister(uint8_t reg, uint8_t data) {
	uint8_t stts;
	
	stts = i2c_start();
	if (stts != I2C_START) return 1;

	stts = i2c_write(I2C_WRITE);
	if (stts != I2C_SLA_W_ACK) return 2;

	stts = i2c_write(reg);
	if (stts != I2C_DATA_ACK) return 3;

	stts = i2c_write(data);
	if (stts != I2C_DATA_ACK) return 4;

	i2c_stop();

	return 0;
}

uint8_t i2cReadRegister(uint8_t reg, uint8_t *data) {
	uint8_t stts;
	
	stts = i2c_start();
	if (stts != I2C_START) return 1;

	stts = i2c_write(I2C_WRITE);
	if (stts != I2C_SLA_W_ACK) return 2;
 
	stts = i2c_write(reg);
	if (stts != I2C_DATA_ACK) return 3;

	stts = i2c_start();
	if (stts != I2C_START_RPT) return 4;

	stts = i2c_write(I2C_READ);
	if (stts != I2C_SLA_R_ACK) return 5;

	*data = i2c_read_ack();

	i2c_stop();

	return 0;
}
