#include <3ds/srv.h>
#include <3ds/gpio.h>

bool gpio_init = false;
Handle gpio_session = 0;

Result gpioMcuInit()
{
	if (gpio_init) return 0;
	
	Result res = SRV_GetServiceHandle(&gpio_session, "gpio:MCU", sizeof("gpio:mcu") - 1, 0);
	
	if (R_SUCCEEDED(res))
		gpio_init = true;
	
	return res;
}

void gpioMcuExit()
{
	if (!gpio_init) return;
	
	Result res = svcCloseHandle(gpio_session);
	if (R_SUCCEEDED(res))
	{
		gpio_init = false;
		gpio_session = 0;
	}
}

Result GPIO_SetRegPart1(u32 value, u32 mask)
{
	u32 *cmdbuf = getThreadCommandBuffer();
	
	cmdbuf[0] = IPC_MakeHeader(ID_GPIO_SetRegPart1, 2, 0);
	cmdbuf[1] = value;
	cmdbuf[2] = mask;
	
	BASIC_RET(gpio_session)
}

Result GPIO_SetInterruptMask(u32 value, u32 mask)
{
	u32 *cmdbuf = getThreadCommandBuffer();
	
	cmdbuf[0] = IPC_MakeHeader(ID_GPIO_SetInterruptMask, 2, 0);
	cmdbuf[1] = value;
	cmdbuf[2] = mask;
	
	BASIC_RET(gpio_session)
}

Result GPIO_ReadData(u32 *out_value, u32 mask)
{
	u32 *cmdbuf = getThreadCommandBuffer();
	
	cmdbuf[0] = IPC_MakeHeader(ID_GPIO_ReadData, 1, 0);
	cmdbuf[1] = mask;
	
	CHECK_RET(gpio_session)
	
	*out_value = cmdbuf[2];
	return res;
}

Result GPIO_WriteData(u32 value, u32 mask)
{
	u32 *cmdbuf = getThreadCommandBuffer();
	
	cmdbuf[0] = IPC_MakeHeader(ID_GPIO_WriteData, 2, 0);
	cmdbuf[1] = value;
	cmdbuf[2] = mask;
	
	BASIC_RET(gpio_session)
}

Result GPIO_BindInterrupt(u32 irq_mask, Handle syncobj, s32 priority)
{
	u32 *cmdbuf = getThreadCommandBuffer();
	
	cmdbuf[0] = IPC_MakeHeader(ID_GPIO_BindInterrupt, 2, 2);
	cmdbuf[1] = irq_mask;
	cmdbuf[2] = (u32)priority;
	cmdbuf[3] = IPC_Desc_SharedHandles(1);
	cmdbuf[4] = syncobj;
	
	BASIC_RET(gpio_session)
}

Result GPIO_UnbindInterrupt(u32 irq_mask, Handle syncobj)
{
	u32 *cmdbuf = getThreadCommandBuffer();
	
	cmdbuf[0] = IPC_MakeHeader(ID_GPIO_UnbindInterrupt, 1, 2);
	cmdbuf[1] = irq_mask;
	cmdbuf[2] = IPC_Desc_SharedHandles(1);
	cmdbuf[3] = syncobj;
	
	BASIC_RET(gpio_session)
}