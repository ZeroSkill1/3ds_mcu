#ifndef _MCU_3DS_GPIO_H
#define _MCU_3DS_GPIO_H

#include <3ds/types.h>

extern Handle gpio_session;
extern bool gpio_init;

Result gpioMcuInit();
void gpioMcuExit();

enum {
	ID_GPIO_SetRegPart1      = 0x0002,
	ID_GPIO_SetInterruptMask = 0x0006,
	ID_GPIO_ReadData         = 0x0007,
	ID_GPIO_WriteData        = 0x0008,
	ID_GPIO_BindInterrupt    = 0x0009,
	ID_GPIO_UnbindInterrupt  = 0x000A,
};

enum {
	GPIO_WLAN_MODE_CTR = 0,
	GPIO_WLAN_MODE_MP  = 1, /* DS WiFi, apparently */
};

enum {
	GPIO_WLAN_MODE     = BIT(5),
	GPIO_MCU_INTERRUPT = BIT(15),
	GPIO_WLAN_STATE    = BIT(18),
};

Result GPIO_SetRegPart1(u32 value, u32 mask);
Result GPIO_SetInterruptMask(u32 value, u32 mask);
Result GPIO_ReadData(u32 *out_value, u32 mask);
Result GPIO_WriteData(u32 value, u32 mask);
Result GPIO_BindInterrupt(u32 irq_mask, Handle syncobj, s32 priority);
Result GPIO_UnbindInterrupt(u32 irq_mask, Handle syncobj);

#endif