#ifndef _MCU_IPC_H
#define _MCU_IPC_H

#include <3ds/types.h>

typedef struct MCU_SessionData
{
	Handle thread;
	Handle session; // needs to be freed in thread itself!
	void (* handle_ipc)();
	void (* pre_serve)();
	void (* post_serve)();
} MCU_SessionData;

void MCUGPU_PreServe();
void MCUGPU_PostServe();

void MCUHID_PreServe();
void MCUHID_PostServe();

void MCURTC_PreServe();

extern void (* MCU_IPCHandlers[9])();

#endif