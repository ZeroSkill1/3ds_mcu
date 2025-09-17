#ifndef _MCU_GLOBALS_H
#define _MCU_GLOBALS_H

#include <3ds/synchronization.h>

extern RecursiveLock g_I2CLock;
extern RecursiveLock g_GPIOLock;
extern RecursiveLock g_ExclusiveIRQLock;

extern LightEvent g_AccelerometerManualI2CEvent;

extern Handle g_GPIO_MCUInterruptEvent;

extern Handle g_IRQEvents[3];
extern u32 g_ReceivedIRQs[3];

extern bool g_McuFirmWasUpdated;

extern bool g_IrqHandlerThreadExitFlag;

#endif