#include <3ds/srv.h>
#include <3ds/i2c.h>

bool i2c_init = false;
Handle i2c_session = 0;

Result i2cMcuInit()
{
	if (i2c_init) return 0;
	
	Result res = SRV_GetServiceHandle(&i2c_session, "i2c::MCU", sizeof("i2c::mcu") - 1, 0);
	
	if (R_SUCCEEDED(res))
		i2c_init = true;
	
	return res;
}

void i2cMcuExit()
{
	if (!i2c_init) return;
	
	Result res = svcCloseHandle(i2c_session);
	if (R_SUCCEEDED(res))
	{
		i2c_init = false;
		i2c_session = 0;
	}
}

Result I2C_SetRegisterBits8(u8 devid, u8 regid, u8 mask, u8 data)
{
	u32 *cmdbuf = getThreadCommandBuffer();
	
	cmdbuf[0] = IPC_MakeHeader(ID_I2C_SetRegisterBits8, 4, 0);
	cmdbuf[1] = devid;
	cmdbuf[2] = regid;
	cmdbuf[3] = data;
	cmdbuf[4] = mask;
	
	BASIC_RET(i2c_session)
}

Result I2C_DisableRegisterBits8(u8 devid, u8 regid, u8 mask)
{
	u32 *cmdbuf = getThreadCommandBuffer();
	
	cmdbuf[0] = IPC_MakeHeader(ID_I2C_DisableRegisterBits8, 3, 0);
	cmdbuf[1] = devid;
	cmdbuf[2] = regid;
	cmdbuf[3] = mask;
	
	BASIC_RET(i2c_session)
}

Result I2C_WriteRegisterBuffer8(u8 devid, u8 regid, const void *buf, u32 size)
{
	u32 *cmdbuf = getThreadCommandBuffer();
	
	cmdbuf[0] = IPC_MakeHeader(ID_I2C_WriteRegisterBuffer8, 3, 2);
	cmdbuf[1] = devid;
	cmdbuf[2] = regid;
	cmdbuf[3] = size;
	cmdbuf[4] = IPC_Desc_StaticBuffer(size, 1);
	cmdbuf[5] = (u32)buf;
	
	BASIC_RET(i2c_session)
}

Result I2C_ReadRegisterBuffer8(u8 devid, u8 regid, void *buf, u32 size)
{
	u32 *cmdbuf = getThreadCommandBuffer();
	IPC_StaticBuffer *staticbufs = getThreadStaticBuffers();
	
	IPC_StaticBuffer bak0 = { staticbufs[0].desc, staticbufs[0].bufptr };
	
	cmdbuf[0] = IPC_MakeHeader(ID_I2C_ReadRegisterBuffer8, 3, 0);
	cmdbuf[1] = devid;
	cmdbuf[2] = regid;
	cmdbuf[3] = size;
	
	staticbufs[0].desc = IPC_Desc_StaticBuffer(size, 0);
	staticbufs[0].bufptr = buf;
	
	Result res = svcSendSyncRequest(i2c_session);
	
	staticbufs[0].desc = bak0.desc;
	staticbufs[0].bufptr = bak0.bufptr;
	
	if (R_FAILED(res))
		return res;
	
	return (Result)cmdbuf[1];
}

Result I2C_WriteRegisterBuffer(u8 devid, u8 regid, const void *buf, u32 size)
{
	u32 *cmdbuf = getThreadCommandBuffer();
	
	cmdbuf[0] = IPC_MakeHeader(ID_I2C_WriteRegisterBuffer, 3, 2);
	cmdbuf[1] = devid;
	cmdbuf[2] = regid;
	cmdbuf[3] = size;
	cmdbuf[4] = IPC_Desc_Buffer(size, IPC_BUFFER_R);
	cmdbuf[5] = (u32)buf;
	
	BASIC_RET(i2c_session)
}

Result I2C_ReadRegisterBuffer(u8 devid, u8 regid, void *buf, u32 size)
{
	u32 *cmdbuf = getThreadCommandBuffer();
	
	cmdbuf[0] = IPC_MakeHeader(ID_I2C_ReadRegisterBuffer, 3, 2);
	cmdbuf[1] = devid;
	cmdbuf[2] = regid;
	cmdbuf[3] = size;
	cmdbuf[4] = IPC_Desc_Buffer(size, IPC_BUFFER_W);
	cmdbuf[5] = (u32)buf;
	
	BASIC_RET(i2c_session)
}