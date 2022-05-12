#ifndef I2C_H
#define I2C_H

#include <inttypes.h>


#define I2C_START 0x08
#define I2C_START_RPT 0x10
#define I2C_SLA_W_ACK 0x18
#define I2C_SLA_R_ACK 0x40
#define I2C_DATA_ACK 0x28
//For the old batch of chips
#define I2C_WRITE 0b11011110
#define I2C_READ  0b11011111

//For the new batch of chips
//#define I2C_WRITE 0b11000000
//#define I2C_READ  0b11000001

extern void i2c_init(void);
extern uint8_t i2c_start(void);
extern void i2c_stop(void);
extern uint8_t i2c_write(uint8_t);
extern uint8_t i2c_read_nack(void);
extern uint8_t i2c_status(void);
extern uint8_t i2cSendRegister(uint8_t reg, uint8_t data);
extern uint8_t i2cReadRegister(uint8_t reg, uint8_t *data);

#endif //I2C_H
