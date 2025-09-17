#ifndef _MCU_3DS_I2C_H
#define _MCU_3DS_I2C_H

#include <3ds/types.h>

extern Handle i2c_session;
extern bool i2c_init;

Result i2cMcuInit();
void i2cMcuExit();

enum
{
	ID_I2C_SetRegisterBits8     = 0x0001,
	ID_I2C_DisableRegisterBits8 = 0x0003,
	ID_I2C_WriteRegisterBuffer8 = 0x000B,
	ID_I2C_ReadRegisterBuffer8  = 0x000D,
	ID_I2C_WriteRegisterBuffer  = 0x0011,
	ID_I2C_ReadRegisterBuffer   = 0x0012,
};

enum
{
	I2C_DEVICE_MCU = 3
};

Result I2C_SetRegisterBits8(u8 devid, u8 regid, u8 mask, u8 data);
Result I2C_DisableRegisterBits8(u8 devid, u8 regid, u8 mask);
Result I2C_WriteRegisterBuffer8(u8 devid, u8 regid, const void *buf, u32 size);
Result I2C_ReadRegisterBuffer8(u8 devid, u8 regid, void *buf, u32 size);
Result I2C_WriteRegisterBuffer(u8 devid, u8 regid, const void *buf, u32 size);
Result I2C_ReadRegisterBuffer(u8 devid, u8 regid, void *buf, u32 size);

#endif