#include <3ds/synchronization.h>
#include <3ds/gpio.h>
#include <3ds/err.h>


#include <mcu/globals.h>
#include <mcu/mcu.h>
#include <mcu/ipc.h>
#include <util.h>

#define CMD_ID_RANGE(id, lower, upper) \
	(id >= lower && id <= upper)

#define MCUGPU_ENABLED_IRQS MCUINT_VIDEO_LCD_PUSH_POWER_OFF | MCUINT_VIDEO_LCD_PUSH_POWER_ON | \
                            MCUINT_VIDEO_BOT_BACKLIGHT_OFF | MCUINT_VIDEO_BOT_BACKLIGHT_ON | \
                            MCUINT_VIDEO_TOP_BACKLIGHT_OFF | MCUINT_VIDEO_TOP_BACKLIGHT_ON
                            
#define MCUHID_ENABLED_IRQS MCUINT_ACCELEROMETER_I2C_MANUAL_IO

#define MCURTC_ENABLED_IRQS MCUINT_POWER_BUTTON_HELD | MCUINT_AC_ADAPTER_REMOVED | \
                            MCUINT_AC_ADAPTER_PLUGGED_IN | MCUINT_RTC_ALARM | \
                            MCUINT_CRITICAL_BATTERY | MCUINT_CHARGING_STOP | MCUINT_CHARGING_START

void MCUGPU_PreServe()
{
	I2C_LOCKED({
		u32 enabled_irqs = 0;
		T(mcuGetInterruptMask(&enabled_irqs, NOLOCK));
		T(mcuSetInterruptMask(enabled_irqs | MCUGPU_ENABLED_IRQS, NOLOCK));
	});
}

void MCUGPU_PostServe()
{
	I2C_LOCKED({
		u32 enabled_irqs = 0;
		T(mcuGetInterruptMask(&enabled_irqs, NOLOCK));
		T(mcuSetInterruptMask(enabled_irqs & ~(MCUGPU_ENABLED_IRQS), NOLOCK));
	});
}

void MCUHID_PreServe()
{
	/* do not fire on accelerometer sample until asked by hid (mcu::HID cmd 0xf) */
	I2C_LOCKED({
		u32 enabled_irqs = 0;
		T(mcuGetInterruptMask(&enabled_irqs, NOLOCK));
		T(mcuSetInterruptMask(enabled_irqs | MCUINT_ACCELEROMETER_I2C_MANUAL_IO, NOLOCK));
	});
}

void MCUHID_PostServe()
{
	/* but disable auto and manual sampling regardless on session exit */
	I2C_LOCKED({
		u32 enabled_irqs = 0;
		T(mcuGetInterruptMask(&enabled_irqs, NOLOCK));
		T(mcuSetInterruptMask(enabled_irqs & ~(MCUINT_ACCELEROMETER_I2C_MANUAL_IO | MCUINT_ACCELEROMETER_NEW_SAMPLE), NOLOCK));
	});
}

void MCURTC_PreServe()
{
	I2C_LOCKED({
		u32 enabled_irqs = 0;
		T(mcuGetInterruptMask(&enabled_irqs, NOLOCK));
		enabled_irqs |= MCURTC_ENABLED_IRQS;
		T(mcuSetInterruptMask(enabled_irqs, NOLOCK));
	})
}

void MCUCAM_HandleIPC()
{
	u32 *cmdbuf = getThreadCommandBuffer();
	u32 cmd_header = cmdbuf[0];
	u16 cmd_id = (cmd_header >> 16) & 0xFFFF;
	
	switch (cmd_id)
	{
	case 0x0001: // set camera LED state
		{
			CHECK_HEADER(0x0001, 1, 0);
			
			u8 state = (u8)cmdbuf[1] & 0xFF;
			Result res = mcuSetLedState(MCUREG_CAMERA_LED_STATE, state, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0001, 1, 0);
			cmdbuf[1] = res;
		}
		break;	
	case 0x0002: // get camera LED state
		{
			CHECK_HEADER(0x0002, 0, 0);
			
			u8 state = 0;
			Result res = mcuGetLedState(MCUREG_CAMERA_LED_STATE, &state, LOCK);

			cmdbuf[0] = IPC_MakeHeader(0x0002, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)state;
		}
		break;
	}
}

void MCUGPU_HandleIPC()
{
	u32 *cmdbuf = getThreadCommandBuffer();
	u32 cmd_header = cmdbuf[0];
	u16 cmd_id = (cmd_header >> 16) & 0xFFFF;
	
	switch (cmd_id)
	{
	case 0x0001: // get backlight power status
		{
			CHECK_HEADER(0x0001, 0, 0);
			u8 power_status = 0;
			
			Result res = mcuGetPowerStatus(&power_status, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0001, 3, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)CHECKBIT(power_status, MCU_PWRSTAT_TOP_BL_ON);
			cmdbuf[3] = (u32)CHECKBIT(power_status, MCU_PWRSTAT_BOTTOM_BL_ON);
		}
		break;
	case 0x0002: // set backlight power
		{
			CHECK_HEADER(0x0002, 2, 0);
			
			u8 top_bl_on = (u8)cmdbuf[1] & 0xFF;
			u8 bot_bl_on = (u8)cmdbuf[2] & 0xFF;
			
			Result res = mcuSetBacklightPowerState(top_bl_on, bot_bl_on, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0002, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0003: // get LCD power status
		{
			CHECK_HEADER(0x0003, 0, 0);
			
			u8 power_status = 0;
			
			Result res = mcuGetPowerStatus(&power_status, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0003, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)CHECKBIT(power_status, MCU_PWRSTAT_LCD_ON);
		}
		break;
	case 0x0004: // set LCD power
		{
			CHECK_HEADER(0x0004, 1, 0);
			
			u8 state = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetLcdPowerState(state, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0004, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0005: // set top LCD VCOM/flicker
		{
			CHECK_HEADER(0x0005, 1, 0);
			
			u8 value = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetLcdFlicker(true, value, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0005, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0006: // get top LCD VCOM/flicker
		{
			CHECK_HEADER(0x0006, 0, 0);
			
			u8 value = 0;
			
			Result res = mcuGetLcdFlicker(true, &value, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0006, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)value;
		}
		break;
	case 0x0007: // set bottom LCD VCOM/flicker
		{
			CHECK_HEADER(0x0007, 1, 0);
			
			u8 value = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetLcdFlicker(false, value, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0007, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0008: // get bottom LCD VCOM/flicker
		{
			CHECK_HEADER(0x0008, 0, 0);
			
			u8 value = 0;
			
			Result res = mcuGetLcdFlicker(false, &value, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0008, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)value;
		}
		break;
	case 0x0009: // read major firmware version
		{
			CHECK_HEADER(0x0009, 0, 0);
			
			u8 value = 0;
			
			Result res = mcuReadFwVerHigh(&value, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0009, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)value;
		}
		break;
	case 0x000A: // read minor firmware version
		{
			CHECK_HEADER(0x000A, 0, 0);
			
			u8 value = 0;
			
			Result res = mcuReadFwVerLow(&value, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x000A, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)value;
		}
		break;
	case 0x000B: // set 3d led state
		{
			CHECK_HEADER(0x000B, 1, 0);
			
			u8 state = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetLedState(MCUREG_3D_LED_STATE, state, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x000B, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x000C: // get 3d led state
		{
			CHECK_HEADER(0x000C, 0, 0);
			
			u8 state = 0;
			
			Result res = mcuGetLedState(MCUREG_3D_LED_STATE, &state, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x000C, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)state;
		}
		break;
	case 0x000D: // get gpu event handle
		{
			CHECK_HEADER(0x000D, 0, 0);
			
			cmdbuf[0] = IPC_MakeHeader(0x000D, 1, 2);
			cmdbuf[1] = 0;
			cmdbuf[2] = IPC_Desc_SharedHandles(1);
			cmdbuf[3] = g_IRQEvents[EVENT_GPU];
		}
		break;
	case 0x000E: // get received gpu event IRQs
		{
			CHECK_HEADER(0x000E, 0, 0);
			
			u32 events = g_ReceivedIRQs[EVENT_GPU];
			g_ReceivedIRQs[EVENT_GPU] = 0;
			
			cmdbuf[0] = IPC_MakeHeader(0x000E, 2, 0);
			cmdbuf[1] = 0;
			cmdbuf[2] = events;
		}
		break;
	}
}

void MCUHID_HandleIPC()
{
	u32 *cmdbuf = getThreadCommandBuffer();
	u32 cmd_header = cmdbuf[0];
	u16 cmd_id = (cmd_header >> 16) & 0xFFFF;
	
	switch (cmd_id)
	{
	case 0x0001: // set -ometer mode (enable/disable pedometer and/or accelerometer)
		{
			CHECK_HEADER(0x0001, 1, 0);
			
			u8 mode = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetOmeterMode(mode, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0001, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0002: // is accelerometer enabled
		{
			CHECK_HEADER(0x0002, 0, 0);
			
			u8 mode = 0;
			
			Result res = mcuGetOmeterMode(&mode, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0002, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)CHECKBIT(mode, MCU_OMETER_ACCELEROMETER_ON);
		}
		break;
	case 0x0003: // start manual accelerometer i2c register read (set register id)
		{
			CHECK_HEADER(0x0003, 1, 0);
			
			u8 hw_regid = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuStartAccelerometerManualRegRead(hw_regid, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0003, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0004: // get manual accelerometer i2c read result (get data)
		{
			CHECK_HEADER(0x0004, 0, 0);
			
			u8 data = 0;
			
			Result res = mcuGetAccelerometerManualRegReadResult(&data, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0004, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)data;
		}
		break;
	case 0x0005: // start manual accelerometer i2c write and get data (combined)
		{
			CHECK_HEADER(0x0005, 2, 0);
			
			u8 hw_regid = (u8)cmdbuf[1] & 0xFF;
			u8 data = (u8)cmdbuf[2] & 0xFF;
			
			Result res = mcuPerformAccelerometerManualWrite(hw_regid, data, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0005, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0006: // read accelerometer data
		{
			CHECK_HEADER(0x0006, 0, 0);
			
			Result res = mcuReadAccelerometerData((MCU_AccelerometerData *)(&cmdbuf[2]), LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0006, 3, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0007: // read 3d slider position
		{
			CHECK_HEADER(0x0007, 0, 0);
			
			u8 pos = 0;
			
			Result res = mcuRead3dSliderPosition(&pos, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0007, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)pos;
		}
		break;
	case 0x0008: // get accelerometer scale
		{
			CHECK_HEADER(0x0008, 0, 0);
			
			u8 scale = 0;
			
			Result res = mcuGetAccelerometerScale(&scale, LOCK);
			if (R_FAILED(res)) res = ACC_FAILURE;
			
			cmdbuf[0] = IPC_MakeHeader(0x0008, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)scale;
		}
		break;
	case 0x0009: // set accelerometer scale
		{
			CHECK_HEADER(0x0009, 1, 0);
			
			u8 scale = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetAccelerometerScale(scale, LOCK);
			if (R_FAILED(res)) res = ACC_FAILURE;
			
			cmdbuf[0] = IPC_MakeHeader(0x0009, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x000A: // set accelerometer internal filter enabled
		{
			CHECK_HEADER(0x000A, 1, 0);
			
			u8 enabled = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetAccelerometerInternalFilterEnabled(enabled, LOCK);
			if (R_FAILED(res)) res = ACC_FAILURE;
			
			cmdbuf[0] = IPC_MakeHeader(0x000A, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x000B: // is accelerometer internal filter enabled
		{
			CHECK_HEADER(0x000B, 0, 0);
			
			u8 enabled = 0;
			
			Result res = mcuGetAccelerometerInternalFilterEnabled(&enabled, LOCK);
			if (R_FAILED(res)) res = ACC_FAILURE;

			cmdbuf[0] = IPC_MakeHeader(0x000B, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)enabled;
		}
		break;
	case 0x000C: // get HID IRQ event handle
		{
			CHECK_HEADER(0x000C, 0, 0);
			
			cmdbuf[0] = IPC_MakeHeader(0x000C, 1, 2);
			cmdbuf[1] = 0;
			cmdbuf[2] = IPC_Desc_SharedHandles(1);
			cmdbuf[3] = g_IRQEvents[EVENT_HID];
		}
		break;
	case 0x000D: // get received HID event IRQs
		{
			CHECK_HEADER(0x000D, 0, 0);
			
			u32 events = g_ReceivedIRQs[EVENT_HID];
			g_ReceivedIRQs[EVENT_HID] = 0;
			
			cmdbuf[0] = IPC_MakeHeader(0x000D, 2, 0);
			cmdbuf[1] = 0;
			cmdbuf[2] = events;
		}
		break;
	case 0x000E: // read volume slider position
		{
			CHECK_HEADER(0x000E, 0, 0);
			
			u8 pos = 0;
			
			Result res = mcuReadVolumeSliderPositiion(&pos, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x000E, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)pos;
		}
		break;
	case 0x000F: // set accelerometer irq enabled (true/false)
		{
			CHECK_HEADER(0x000F, 1, 0);
			
			u8 enabled = (u8)cmdbuf[1] & 0xFF;
			
			I2C_LOCKED({
				u32 enabled_irqs = 0;
				T(mcuGetInterruptMask(&enabled_irqs, NOLOCK));
				if (enabled) enabled_irqs |= MCUINT_ACCELEROMETER_NEW_SAMPLE;
				else         enabled_irqs &= ~(MCUINT_ACCELEROMETER_NEW_SAMPLE);
				T(mcuSetInterruptMask(enabled_irqs, NOLOCK));
			});
			
			cmdbuf[0] = IPC_MakeHeader(0x000F, 1, 0);
			cmdbuf[1] = 0;
		}
		break;
	}
}

void MCURTC_HandleIPC()
{
	u32 *cmdbuf = getThreadCommandBuffer();
	u32 cmd_header = cmdbuf[0];
	u16 cmd_id = (cmd_header >> 16) & 0xFFFF;
	
	switch (cmd_id)
	{
	case 0x0001: // set RTC time (full)
		{
			CHECK_HEADER(0x0001, 2, 0)
			
			MCU_RtcData data = { 0 };
			
			_memcpy(&data, &cmdbuf[1], sizeof(MCU_RtcData));
			
			Result res = mcuSetRtcTime(&data, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0001, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0002: // get RTC time (full)
		{
			CHECK_HEADER(0x0002, 0, 0)
			
			MCU_RtcData data = { 0 };
			s64 systick = 0;
			
			Result res = mcuGetRtcTime(&data, LOCK);
			systick = svcGetSystemTick();
			
			cmdbuf[0] = IPC_MakeHeader(0x0002, 5, 0);
			cmdbuf[1] = res;
			_memcpy(&cmdbuf[2], &data, sizeof(MCU_RtcData));
			*(s64 *)(&cmdbuf[4]) = systick;
		}
		break;
	case 0x0003: // set RTC time (second part)
		{
			CHECK_HEADER(0x0003, 1, 0)
			
			u8 second = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetRtcTimeField(MCUREG_RTC_TIME_SECOND, second, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0003, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0004: // get RTC time (second part)
		{
			CHECK_HEADER(0x0004, 0, 0)
			
			u8 second = 0;
			s64 systick = 0;
			
			Result res = mcuGetRtcTimeField(MCUREG_RTC_TIME_SECOND, &second, LOCK);
			systick = svcGetSystemTick();
			
			cmdbuf[0] = IPC_MakeHeader(0x0004, 4, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)second;
			*((s64 *)&cmdbuf[3]) = systick;
		}
		break;
	case 0x0005: // set RTC time (minute part)
		{
			CHECK_HEADER(0x0005, 1, 0)
			
			u8 minute = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetRtcTimeField(MCUREG_RTC_TIME_MINUTE, minute, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0005, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0006: // get RTC time (minute part)
		{
			CHECK_HEADER(0x0006, 0, 0)
			
			u8 minute = 0;
			s64 systick = 0;
			
			Result res = mcuGetRtcTimeField(MCUREG_RTC_TIME_MINUTE, &minute, LOCK);
			systick = svcGetSystemTick();
			
			cmdbuf[0] = IPC_MakeHeader(0x0006, 4, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)minute;
			*((s64 *)&cmdbuf[3]) = systick;
		}
		break;
	case 0x0007: // set RTC time (hour part)
		{
			CHECK_HEADER(0x0007, 1, 0)
			
			u8 hour = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetRtcTimeField(MCUREG_RTC_TIME_HOUR, hour, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0007, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0008: // get RTC time (hour part)
		{
			CHECK_HEADER(0x0008, 0, 0)
			
			u8 hour = 0;
			s64 systick = 0;
			
			Result res = mcuGetRtcTimeField(MCUREG_RTC_TIME_HOUR, &hour, LOCK);
			systick = svcGetSystemTick();
			
			cmdbuf[0] = IPC_MakeHeader(0x0008, 4, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)hour;
			*((s64 *)&cmdbuf[3]) = systick;
		}
		break;
	case 0x0009: // set RTC time (weekday part)
		{
			CHECK_HEADER(0x0009, 1, 0)
			
			u8 weekday = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetRtcTimeField(MCUREG_RTC_TIME_WEEKDAY, weekday, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0009, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x000A: // get RTC time (weekday part)
		{
			CHECK_HEADER(0x000A, 0, 0)
			
			u8 weekday = 0;
			s64 systick = 0;
			
			Result res = mcuGetRtcTimeField(MCUREG_RTC_TIME_WEEKDAY, &weekday, LOCK);
			systick = svcGetSystemTick();
			
			cmdbuf[0] = IPC_MakeHeader(0x000A, 4, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)weekday;
			*((s64 *)&cmdbuf[3]) = systick;
		}
		break;
	case 0x000B: // set RTC time (day part)
		{
			CHECK_HEADER(0x000B, 1, 0)
			
			u8 day = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetRtcTimeField(MCUREG_RTC_TIME_DAY, day, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x000B, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x000C: // get RTC time (day part)
		{
			CHECK_HEADER(0x000C, 0, 0)
			
			u8 day = 0;
			s64 systick = 0;
			
			Result res = mcuGetRtcTimeField(MCUREG_RTC_TIME_DAY, &day, LOCK);
			systick = svcGetSystemTick();
			
			cmdbuf[0] = IPC_MakeHeader(0x000C, 4, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)day;
			*((s64 *)&cmdbuf[3]) = systick;
		}
		break;
	case 0x000D: // set RTC time (month part)
		{
			CHECK_HEADER(0x000D, 1, 0)
			
			u8 month = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetRtcTimeField(MCUREG_RTC_TIME_MONTH, month, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x000D, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x000E: // get RTC time (month part)
		{
			CHECK_HEADER(0x000E, 0, 0)
			
			u8 month = 0;
			s64 systick = 0;
			
			Result res = mcuGetRtcTimeField(MCUREG_RTC_TIME_MONTH, &month, LOCK);
			systick = svcGetSystemTick();
			
			cmdbuf[0] = IPC_MakeHeader(0x000E, 4, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)month;
			*((s64 *)&cmdbuf[3]) = systick;
		}
		break;
	case 0x000F: // set RTC time (year since 2000 part)
		{
			CHECK_HEADER(0x000F, 1, 0)
			
			u8 year = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetRtcTimeField(MCUREG_RTC_TIME_YEAR, year, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x000F, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0010: // get RTC time (year since 2000 part)
		{
			CHECK_HEADER(0x0010, 0, 0)
			
			u8 year = 0;
			s64 systick = 0;
			
			Result res = mcuGetRtcTimeField(MCUREG_RTC_TIME_YEAR, &year, LOCK);
			systick = svcGetSystemTick();
			
			cmdbuf[0] = IPC_MakeHeader(0x0010, 4, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)year;
			*((s64 *)&cmdbuf[3]) = systick;
		}
		break;
	case 0x0011: // set RTC time "correction"(?)
		{
			CHECK_HEADER(0x0011, 1, 0)
			
			u8 correction_data = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetRtcTimeCorrection(correction_data, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0011, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0012: // get RTC time "correction"(?)
		{
			CHECK_HEADER(0x0012, 2, 0)
			
			u8 correction_data = 0;
			
			Result res = mcuGetRtcTimeCorrection(&correction_data, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0012, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)correction_data;
		}
		break;
	case 0x0013: // set RTC alarm time (full)
		{
			CHECK_HEADER(0x0013, 2, 0)
			
			MCU_RtcAlarm data = { 0 };
			
			_memcpy(&data, &cmdbuf[1], sizeof(MCU_RtcAlarm));
			
			Result res = mcuSetRtcAlarm(&data, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0013, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0014: // get RTC alarm time (full)
		{
			CHECK_HEADER(0x0014, 0, 0)
			
			MCU_RtcAlarm data = { 0 };
			
			Result res = mcuGetRtcAlarm(&data, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0014, 3, 0);
			cmdbuf[1] = res;
			_memcpy(&cmdbuf[2], &data, sizeof(MCU_RtcAlarm));
		}
		break;
	case 0x0015: // set RTC alarm time (minute part)
		{
			CHECK_HEADER(0x0015, 1, 0)
			
			u8 minute = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetRtcAlarmField(MCUREG_RTC_ALARM_MINUTE, minute, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0015, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0016: // get RTC alarm time (minute part)
		{
			CHECK_HEADER(0x0016, 1, 0)
			
			u8 minute = 0;
			
			Result res = mcuGetRtcAlarmField(MCUREG_RTC_ALARM_MINUTE, &minute, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0016, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)minute;
		}
		break;
	case 0x0017: // set RTC alarm time (hour part)
		{
			CHECK_HEADER(0x0017, 1, 0)
			
			u8 hour = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetRtcAlarmField(MCUREG_RTC_ALARM_HOUR, hour, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0017, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0018: // get RTC alarm time (hour part)
		{
			CHECK_HEADER(0x0018, 1, 0)
			
			u8 hour = 0;
			
			Result res = mcuGetRtcAlarmField(MCUREG_RTC_ALARM_HOUR, &hour, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0018, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)hour;
		}
		break;
	case 0x0019: // set RTC alarm time (day part)
		{
			CHECK_HEADER(0x0019, 1, 0)
			
			u8 day = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetRtcAlarmField(MCUREG_RTC_ALARM_DAY, day, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0019, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x001A: // get RTC alarm time (day part)
		{
			CHECK_HEADER(0x001A, 1, 0)
			
			u8 day = 0;
			
			Result res = mcuGetRtcAlarmField(MCUREG_RTC_ALARM_DAY, &day, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x001A, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)day;
		}
		break;
	case 0x001B: // set RTC alarm time (month part)
		{
			CHECK_HEADER(0x001B, 1, 0)
			
			u8 month = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetRtcAlarmField(MCUREG_RTC_ALARM_MONTH, month, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x001B, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x001C: // get RTC alarm time (month part)
		{
			CHECK_HEADER(0x001C, 1, 0)
			
			u8 month = 0;
			
			Result res = mcuGetRtcAlarmField(MCUREG_RTC_ALARM_MONTH, &month, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x001C, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)month;
		}
		break;
	case 0x001D: // set RTC alarm time (year since 2000 part)
		{
			CHECK_HEADER(0x001D, 1, 0)
			
			u8 year = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetRtcAlarmField(MCUREG_RTC_ALARM_YEAR, year, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x001D, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x001E: // get RTC alarm time (year since 2000 part)
		{
			CHECK_HEADER(0x001E, 1, 0)
			
			u8 year = 0;
			
			Result res = mcuGetRtcAlarmField(MCUREG_RTC_ALARM_YEAR, &year, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x001E, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)year;
		}
		break;
	case 0x001F: // set pedometer enabled
		{
			CHECK_HEADER(0x001F, 1, 0)
			
			u8 enabled = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetPedometerEnabled(enabled, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x001F, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0020: // get pedometer enabled
		{
			CHECK_HEADER(0x0020, 0, 0)
			
			u8 enabled = 0;
			
			Result res = mcuGetPedometerEnabled(&enabled, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0020, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)enabled;
		}
		break;
	case 0x0021: // read pedometer step count
		{
			CHECK_HEADER(0x0021, 0, 0)
			
			u32 count = 0;
			
			Result res = mcuReadPedometerStepCount(&count, LOCK);

			cmdbuf[0] = IPC_MakeHeader(0x0021, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = count;
		}
		break;
	case 0x0022: // read pedometer step data
		{
			CHECK_HEADER(0x0022, 1, 2)
			
			CHECK_WRONGARG(
				!IPC_VerifyBuffer(cmdbuf[2], IPC_BUFFER_W) ||
				IPC_GetBufferSize(cmdbuf[2]) != cmdbuf[1]
			);
			
			u32 bufsize = IPC_GetBufferSize(cmdbuf[2]);
			MCU_PedometerStepData *buf = (MCU_PedometerStepData *)cmdbuf[3];
			
			Result res = 0;
			
			if (bufsize != sizeof(MCU_PedometerStepData)) {
				res = MCU_INVALID_SIZE;
			} else {
				res = mcuReadPedoemterStepData(buf, LOCK);
			}
			
			cmdbuf[0] = IPC_MakeHeader(0x0022, 1, 2);
			cmdbuf[1] = res;
			cmdbuf[2] = IPC_Desc_Buffer(bufsize, IPC_BUFFER_W);
			cmdbuf[3] = (u32)buf;
		}
		break;
	case 0x0023: // clear pedometer step data
		{
			CHECK_HEADER(0x0023, 0, 0)
			
			Result res = mcuClearPedometerStepData(LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0023, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0024: // get MCU power IRQ event handle
		{
			CHECK_HEADER(0x0024, 0, 0)
			
			cmdbuf[0] = IPC_MakeHeader(0x0024, 1, 2);
			cmdbuf[1] = 0;
			cmdbuf[2] = IPC_Desc_SharedHandles(1);
			cmdbuf[3] = g_IRQEvents[EVENT_POWER];
		}
		break;
	case 0x0025: // get received MCU power IRQ events
		{
			CHECK_HEADER(0x0025, 0, 0)
				
			u32 events = g_ReceivedIRQs[EVENT_POWER];
			g_ReceivedIRQs[EVENT_POWER] = 0;
			
			cmdbuf[0] = IPC_MakeHeader(0x0025, 2, 0);
			cmdbuf[1] = 0;
			cmdbuf[2] = events;
		}
		break;
	case 0x0026: // check RTC time lost
		{
			CHECK_HEADER(0x0026, 0, 0)
			
			u8 reset_events = 0;
			
			Result res = mcuGetResetEventFlags(&reset_events, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0026, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)CHECKBIT(reset_events, MCU_RESETFLG_RTC_TIME_LOST);
		}
		break;
	case 0x0027: // clear RTC time lost flag
		{
			CHECK_HEADER(0x0027, 0, 0)
			
			Result res = mcuClearResetEventFlag(MCU_RESETFLG_RTC_TIME_LOST, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0027, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0028: // check watchdog reset occurred
		{
			CHECK_HEADER(0x0028, 0, 0)
			
			u8 reset_events = 0;
			
			Result res = mcuGetResetEventFlags(&reset_events, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0028, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)CHECKBIT(reset_events, MCU_RESETFLG_WATCHDOG_RESET);
		}
		break;
	case 0x0029: // clear watchdog reset occurred flag
		{
			CHECK_HEADER(0x0029, 0, 0)
			
			Result res = mcuClearResetEventFlag(MCU_RESETFLG_WATCHDOG_RESET, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0029, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x002A: // get shell state
		{
			CHECK_HEADER(0x002A, 0, 0)
			
			u8 power_status = 0;
			
			Result res = mcuGetPowerStatus(&power_status, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x002A, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)CHECKBIT(power_status, MCU_PWRSTAT_SHELL_OPEN);
		}
		break;
	case 0x002B: // get adapter state
		{
			CHECK_HEADER(0x002B, 0, 0)
			
			u8 power_status = 0;
			
			Result res = mcuGetPowerStatus(&power_status, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x002B, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)CHECKBIT(power_status, MCU_PWRSTAT_ADAPTER_CONNECTED);
		}
		break;
	case 0x002C: // get charging state
		{
			CHECK_HEADER(0x002C, 0, 0)
			
			u8 power_status = 0;
			
			Result res = mcuGetPowerStatus(&power_status, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x002C, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)CHECKBIT(power_status, MCU_PWRSTAT_CHARGING);
		}
		break;
	case 0x002D: // read battery percentage (integer part)
		{
			CHECK_HEADER(0x002D, 0, 0)
			
			u8 perc = 0;
			
			Result res = mcuReadBatteryPercentageInt(&perc, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x002D, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)perc;
		}
		break;
	case 0x002E: // set power LED state
		{
			CHECK_HEADER(0x002E, 1, 0)
			
			u8 state = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetLedState(MCUREG_POWER_LED_STATE, state, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x002E, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x002F: // get power LED state
		{
			CHECK_HEADER(0x002F, 0, 0)
			
			u8 state = 0;
			
			Result res = mcuGetLedState(MCUREG_POWER_LED_STATE, &state, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x002F, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)state;
		}
		break;
	case 0x0030: // set LED brightness
		{
			CHECK_HEADER(0x0030, 1, 0)
			
			u8 brightness = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetLedState(MCUREG_LED_BRIGHTNESS_STATE, brightness, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0030, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0031: // get LED brightness
		{
			CHECK_HEADER(0x0031, 0, 0)
			
			u8 brightness = 0;
			
			Result res = mcuGetLedState(MCUREG_LED_BRIGHTNESS_STATE, &brightness, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0031, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)brightness;
		}
		break;
	case 0x0032: // poweroff
		{
			CHECK_HEADER(0x0032, 0, 0)
			
			Result res = mcuSetPowerState(MCU_PWR_SHUTDOWN, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0032, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0033: // reboot?
		{
			CHECK_HEADER(0x0033, 0, 0)
			
			Result res = mcuSetPowerState(MCU_PWR_REBOOT, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0033, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0034: // reset?
		{
			CHECK_HEADER(0x0034, 0, 0)
			
			Result res = mcuReset(LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0034, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0035: // enter sleep mode
		{
			CHECK_HEADER(0x0035, 0, 0)
			
			Result res = mcuSetPowerState(MCU_PWR_SLEEP, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0035, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0036: // set delay for force shutdown via power button
		{
			CHECK_HEADER(0x0036, 1, 0)
			
			u8 delay = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetForceShutdownDelay(delay, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0036, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0037: // get delay for force shutdown via power button
		{
			CHECK_HEADER(0x0037, 0, 0)
			
			u8 delay = 0;
			
			Result res = mcuGetForceShutdownDelay(&delay, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0037, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)delay;
		}
		break;
	case 0x0038: // read info registers
		{
			CHECK_HEADER(0x0038, 1, 2)
			
			CHECK_WRONGARG(
				!IPC_VerifyBuffer(cmdbuf[2], IPC_BUFFER_W) ||
				IPC_GetBufferSize(cmdbuf[2]) != cmdbuf[1]
			)
			
			u32 bufsize = IPC_GetBufferSize(cmdbuf[2]);
			void *buf = (void *)cmdbuf[3];
			
			Result res = mcuReadInfoRegisters(buf, bufsize, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0038, 1, 2);
			cmdbuf[1] = res;
			cmdbuf[2] = IPC_Desc_Buffer(bufsize, IPC_BUFFER_W);
			cmdbuf[3] = (u32)buf;
		}
		break;
	case 0x0039: // write to storage area
		{
			CHECK_HEADER(0x0039, 2, 2)
			
			CHECK_WRONGARG(
				!IPC_VerifyBuffer(cmdbuf[3], IPC_BUFFER_R) ||
				IPC_GetBufferSize(cmdbuf[3]) != cmdbuf[2]
			)
			
			u8 offset = (u8)cmdbuf[1] & 0xFF;
			u32 size = IPC_GetBufferSize(cmdbuf[3]);
			void *buf = (void *)cmdbuf[4];
			
			Result res = 0;
			
			if (offset + size <= sizeof(MCU_StorageArea) - 8) {
				res = mcuWriteStorageArea(offset, buf, size, LOCK);
			}
			else {
				res = MCU_INVALID_SIZE;
			}
			
			cmdbuf[0] = IPC_MakeHeader(0x0039, 1, 2);
			cmdbuf[1] = res;
			cmdbuf[2] = IPC_Desc_Buffer(size, IPC_BUFFER_R);
			cmdbuf[3] = (u32)buf;
		}
		break;
	case 0x003A: // read from storage area
		{
			CHECK_HEADER(0x003A, 2, 2)
			
			CHECK_WRONGARG(
				!IPC_VerifyBuffer(cmdbuf[3], IPC_BUFFER_W) ||
				IPC_GetBufferSize(cmdbuf[3]) != cmdbuf[2]
			)
			
			u8 offset = (u8)cmdbuf[1] & 0xFF;
			u32 size = IPC_GetBufferSize(cmdbuf[3]);
			void *buf = (void *)cmdbuf[4];
			
			Result res = 0;
			
			if (offset + size <= sizeof(MCU_StorageArea) - 8) {
				res = mcuReadStorageArea(offset, buf, size, LOCK);
			} else {
				res = MCU_INVALID_SIZE;
			}
			
			cmdbuf[0] = IPC_MakeHeader(0x003A, 1, 2);
			cmdbuf[1] = res;
			cmdbuf[2] = IPC_Desc_Buffer(size, IPC_BUFFER_W);
			cmdbuf[3] = (u32)buf;
		}
		break;
	case 0x003B: // write notification LED config
		{
			CHECK_HEADER(0x003B, 25, 0)
			
			MCU_NotificationLedData data = { 0 };
			
			_memcpy32_aligned(&data, &cmdbuf[1], sizeof(MCU_NotificationLedData));
			
			Result res = mcuSetNotificationLedData(&data, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x003B, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x003C: // set notification LED animation (does not modify pattern)
		{
			CHECK_HEADER(0x003C, 1, 0)
			
			MCU_NotifictationLedAnimation animation = { 0 };
			_memcpy32_aligned(&animation, &cmdbuf[1], sizeof(MCU_NotifictationLedAnimation));
			
			Result res = mcuSetNotificationLedAnimation(&animation, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x003C, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x003D: // get notification LED cycle state
		{
			CHECK_HEADER(0x003D, 0, 0)
			
			u8 state = 0;
			
			Result res = mcuGetNotificationLedCycleState(&state, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x003D, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)state;
		}
		break;
	case 0x003E: // set pedometer wrap time minute
		{
			CHECK_HEADER(0x003E, 1, 0)
			
			u8 value = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetPedometerWrapTimeMinute(value, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x003E, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x003F: // get pedometer wrap time minute
		{
			CHECK_HEADER(0x003F, 0, 0)
			
			u8 value = 0;
			
			Result res = mcuGetPedometerWrapTimeMinute(&value, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x003F, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)value;
		}
		break;
	case 0x0040: // set pedometer wrap time second
		{
			CHECK_HEADER(0x0040, 1, 0)
			
			u8 value = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetPedometerWrapTimeSecond(value, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0040, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0041: // get pedometer wrap time second
		{
			CHECK_HEADER(0x0041, 2, 0)
			
			u8 value = 0;
			
			Result res = mcuGetPedometerWrapTimeSecond(&value, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0041, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)value;
		}
		break;
	case 0x0042: // set power LED blink pattern (for critial battery)
		{
			CHECK_HEADER(0x0042, 1, 0)
			
			u32 pattern = cmdbuf[1];
			
			Result res = mcuSetPowerLedBlinkPattern(pattern, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0042, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0043: // set top LCD flicker/VCOM
		{
			CHECK_HEADER(0x0043, 1, 0)
			
			u8 value = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetLcdFlicker(true, value, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0043, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0044: // get top LCD flicker/VCOM
		{
			CHECK_HEADER(0x0044, 0, 0)
			
			u8 value = 0;
			
			Result res = mcuGetLcdFlicker(true, &value, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0044, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)value;
		}
		break;
	case 0x0045: // set bottom LCD flicker/VCOM
		{
			CHECK_HEADER(0x0045, 1, 0)
			
			u8 value = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetLcdFlicker(false, value, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0045, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0046: // get bottom LCD flicker/VCOM
		{
			CHECK_HEADER(0x0046, 0, 0)
			
			u8 value = 0;
			
			Result res = mcuGetLcdFlicker(false, &value, LOCK);

			cmdbuf[0] = IPC_MakeHeader(0x0046, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)value;
		}
		break;
	case 0x0047: // set volume slider calibration data
		{
			CHECK_HEADER(0x0047, 2, 0)
			
			u8 min = (u8)cmdbuf[1] & 0xFF;
			u8 max = (u8)cmdbuf[2] & 0xFF;
			
			Result res = mcuSetVolumeCalibration(min, max, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0047, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0048: // get volume slider calibration data
		{
			CHECK_HEADER(0x0048, 0, 0)
			
			u8 min = 0;
			u8 max = 0;
			
			Result res = mcuGetVolumeCalibration(&min, &max, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0048, 3, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)min;
			cmdbuf[3] = (u32)max;
		}
		break;
	case 0x0049: // set interrupt mask
		{
			CHECK_HEADER(0x0049, 1, 0)
			
			u32 enabled_irqs = cmdbuf[1];
			
			Result res = mcuSetInterruptMask(enabled_irqs, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0049, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x004A: // get interrupt mask
		{
			CHECK_HEADER(0x004A, 0, 0)
			
			u32 enabled_irqs = 0;
			
			Result res = mcuGetInterruptMask(&enabled_irqs, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x004A, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = enabled_irqs;
		}
		break;
	case 0x004B: // leave exclusive interrupt mode
		{
			CHECK_HEADER(0x004B, 0, 0)
			
			RecursiveLock_Unlock(&g_ExclusiveIRQLock);
			
			cmdbuf[0] = IPC_MakeHeader(0x004B, 1, 0);
			cmdbuf[1] = 0;
		}
		break;
	case 0x004C: // enter exclusive interrupt mode
		{
			CHECK_HEADER(0x004C, 0, 0)
			
			RecursiveLock_Lock(&g_ExclusiveIRQLock);
			
			cmdbuf[0] = IPC_MakeHeader(0x004C, 1, 0);
			cmdbuf[1] = 0;
		}
		break;
	case 0x004D: // get received interrupts
		{
			CHECK_HEADER(0x004D, 0, 0)
			
			u32 received_irqs = 0;
			
			Result res = mcuGetReceivedIrqs(&received_irqs, LOCK);

			cmdbuf[0] = IPC_MakeHeader(0x004D, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = received_irqs;
		}
		break;
	case 0x004E: // trigger interrupt ("fake" interrupt)
		{
			CHECK_HEADER(0x004E, 1, 0)
			
			u32 irqs_to_trigger = cmdbuf[1];
			
			mcuHandleInterruptEvents(irqs_to_trigger);
			
			cmdbuf[0] = IPC_MakeHeader(0x004E, 1, 0);
			cmdbuf[1] = 0;
		}
		break;
	case 0x004F: // set was MCU firmware updated
		{
			CHECK_HEADER(0x004F, 1, 0)
				
			u8 was_updated = (u8)cmdbuf[1] & 0xFF;
			
			g_McuFirmWasUpdated = !!was_updated;
			
			cmdbuf[0] = IPC_MakeHeader(0x004F, 1, 0);
			cmdbuf[1] = 0;
		}
		break;
	case 0x0050: // get was MCU firmware updated
		{
			CHECK_HEADER(0x0050, 0, 0)
			
			cmdbuf[0] = IPC_MakeHeader(0x0050, 2, 0);
			cmdbuf[1] = 0;
			cmdbuf[2] = (u8)g_McuFirmWasUpdated;
		}
		break;
	case 0x0051: // set software closed flag
		{
			CHECK_HEADER(0x0051, 1, 0)
			
			u8 set = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetFirmFlag(MCU_FIRMFLG_SOFTWARE_CLOSED, !!set, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0051, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0052: // get software closed flag
		{
			CHECK_HEADER(0x0052, 0, 0)
			
			bool set = false;
			
			Result res = mcuGetFirmFlag(&set, MCU_FIRMFLG_SOFTWARE_CLOSED, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0052, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)set;
		}
		break;
	case 0x0053: // set lgy lcd data
		{
			CHECK_HEADER(0x0053, 1, 0)
			
			u8 data = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuWriteStorageArea(offsetof(MCU_StorageArea, lgy_lcd_data), &data, sizeof(u8), LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0053, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0054: // get lgy lcd data
		{
			CHECK_HEADER(0x0054, 0, 0)
			
			u8 data = 0;
			
			Result res = mcuReadStorageArea(offsetof(MCU_StorageArea, lgy_lcd_data), &data, sizeof(u8), LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0054, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)data;
		}
		break;
	case 0x0055: // set lgy native resolution flag
		{
			CHECK_HEADER(0x0055, 1, 0)
			
			u8 set = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetFirmFlag(MCU_FIRMFLG_LGY_NATIVE_RESOLUTION, !!set, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0055, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0056: // get lgy native resolution flag
		{
			CHECK_HEADER(0x0056, 0, 0)
			
			bool set = false;
			
			Result res = mcuGetFirmFlag(&set, MCU_FIRMFLG_LGY_NATIVE_RESOLUTION, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0056, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u8)set;
		}
		break;
	case 0x0057: // set local friend code counter
		{
			CHECK_HEADER(0x0057, 1, 0)
			
			u16 counter = (u16)cmdbuf[1] & 0xFFFF;
			
			Result res = mcuWriteStorageArea(offsetof(MCU_StorageArea, local_friend_code_counter), &counter, sizeof(u16), LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0057, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0058: // get local friend code counter
		{
			CHECK_HEADER(0x0058, 0, 0)
			
			u16 counter = 0;
			
			Result res = mcuReadStorageArea(offsetof(MCU_StorageArea, local_friend_code_counter), &counter, sizeof(u16), LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0058, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)counter;
		}
		break;
	case 0x0059: // set legacy jump prohibited flag
		{
			CHECK_HEADER(0x0059, 1, 0)
			
			u8 set = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetFirmFlag(MCU_FIRMFLG_LEGACY_JUMP_PROHIBITED, !!set, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0059, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x005A: // get legacy jump prohibited flag
		{
			CHECK_HEADER(0x005A, 0, 0)
			
			bool set = false;
			
			Result res = mcuGetFirmFlag(&set, MCU_FIRMFLG_LEGACY_JUMP_PROHIBITED, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x005A, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u8)set;
		}
		break;
	case 0x005B: // set uuid clock sequence
		{
			CHECK_HEADER(0x005B, 1, 0)
			
			u16 clockseq = (u16)cmdbuf[1] & 0xFFFF;
			
			Result res = mcuWriteStorageArea(offsetof(MCU_StorageArea, uuid_clock_sequence), &clockseq, sizeof(u16), LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x005B, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x005C: // get uuid clock sequence
		{
			CHECK_HEADER(0x005C, 0, 0)
			
			u16 clockseq = 0;
			
			Result res = mcuReadStorageArea(offsetof(MCU_StorageArea, uuid_clock_sequence), &clockseq, sizeof(u16), LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x005C, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)clockseq;
		}
		break;
	}
}

void MCUSND_HandleIPC()
{
	u32 *cmdbuf = getThreadCommandBuffer();
	u32 cmd_header = cmdbuf[0];
	u16 cmd_id = (cmd_header >> 16) & 0xFFFF;
	
	switch (cmd_id)
	{
	case 0x0001: // read volume slider position
		{
			CHECK_HEADER(0x0001, 0, 0)
			
			u8 pos = 0;
			
			Result res = mcuReadVolumeSliderPositiion(&pos, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0001, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)pos;
		}
		break;
	case 0x0002: // set unknown volume(?) register 0x25
		{
			CHECK_HEADER(0x0002, 1, 0)
			
			u8 value = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetUnkVolume25(value, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0002, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0003: // get unknown volume(?) register 0x25
		{
			CHECK_HEADER(0x0003, 0, 0)
			
			u8 value = 0;
			
			Result res = mcuGetUnkVolume25(&value, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0003, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)value;
		}
		break;
	}
}

void MCUNWM_HandleIPC()
{
	u32 *cmdbuf = getThreadCommandBuffer();
	u32 cmd_header = cmdbuf[0];
	u16 cmd_id = (cmd_header >> 16) & 0xFFFF;
	
	switch (cmd_id)
	{
	case 0x0001: // set WiFi LED state
		{
			CHECK_HEADER(0x0001, 1, 0)
			
			u8 state = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetLedState(MCUREG_WLAN_LED_STATE, state, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0001, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0002: // get WiFi LED state
		{
			CHECK_HEADER(0x0002, 0, 0)
			
			u8 state = 0;
			
			Result res = mcuGetLedState(MCUREG_WLAN_LED_STATE, &state, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0002, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)state;
		}
		break;
	case 0x0003: // set MP/CTR WiFi mode
		{
			CHECK_HEADER(0x0003, 1, 0)
			
			u8 mode = (u8)cmdbuf[1] & 0xFF; /* 0 = CTR, 1 = MP (DS[i] WiFi) */
			
			Result res = gpioMcuWriteData_l((mode == GPIO_WLAN_MODE_MP) * GPIO_WLAN_MODE, GPIO_WLAN_MODE);
			
			cmdbuf[0] = IPC_MakeHeader(0x0003, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0004: // get MP/CTR WiFi mode
		{
			CHECK_HEADER(0x0004, 0, 0)
			
			u32 mode = 0;
			
			Result res = gpioMcuReadData_l(&mode, GPIO_WLAN_MODE);
			
			cmdbuf[0] = IPC_MakeHeader(0x0004, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = CHECKBIT(mode, GPIO_WLAN_MODE); /* 0 = CTR, 1 = MP */
		}
		break;
	case 0x0005: // WiFi reset/enable, 1 = enable, 0 = reset/off
		{
			CHECK_HEADER(0x0005, 1, 0)
			
			u8 value = (u8)cmdbuf[1] & 0xFF;
			
			Result res = 0;
			
			GPIO_LOCKED({
				//res = gpioMcuSetRegPart1(GPIO_WLAN_STATE, GPIO_WLAN_STATE);
				//if (R_SUCCEEDED(res)) {
					res = gpioMcuWriteData(value ? GPIO_WLAN_STATE : 0, GPIO_WLAN_STATE);
					//}
			});
			
			cmdbuf[0] = IPC_MakeHeader(0x0005, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0006: // get WiFi reset/enable, 1 = enabled, 0 = reset/not enabled
		{
			CHECK_HEADER(0x0006, 0, 0)
			
			u32 status = 0;
			
			Result res = 0;
			
			GPIO_LOCKED({
				//res = gpioMcuSetRegPart1(GPIO_WLAN_STATE, GPIO_WLAN_STATE);
				//if (R_SUCCEEDED(res)) {
					res = gpioMcuReadData(&status, GPIO_WLAN_STATE);
					//}
			});
			
			cmdbuf[0] = IPC_MakeHeader(0x0006, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = CHECKBIT(status, GPIO_WLAN_STATE);
		}
		break;
	case 0x0007: // set wireless disabled flag
		{
			CHECK_HEADER(0x0007, 1, 0)
			
			u8 set = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetFirmFlag(MCU_FIRMFLG_WIRELESS_DISABLED, !!set, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0007, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0008: // get wireless disabled flag
		{
			CHECK_HEADER(0x0008, 0, 0)
			
			bool set = false;
			
			Result res = mcuGetFirmFlag(&set, MCU_FIRMFLG_WIRELESS_DISABLED, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0008, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)set;
		}
		break;
	}
}

void MCUHWC_HandleIPC()
{
	u32 *cmdbuf = getThreadCommandBuffer();
	u32 cmd_header = cmdbuf[0];
	u16 cmd_id = (cmd_header >> 16) & 0xFFFF;
	
	switch (cmd_id)
	{
	case 0x0001: // read register(s)
		{
			CHECK_HEADER(0x0001, 2, 2)
			
			CHECK_WRONGARG(
				!IPC_VerifyBuffer(cmdbuf[3], IPC_BUFFER_W) ||
				IPC_GetBufferSize(cmdbuf[3]) != cmdbuf[2]
			)
			
			u8 regid = (u8)cmdbuf[1];
			u32 size = IPC_GetBufferSize(cmdbuf[3]);
			void *buf = (void *)cmdbuf[4];
			
			Result res = mcuReadRegisterBuffer_l(regid, buf, size);
			
			cmdbuf[0] = IPC_MakeHeader(0x0001, 1, 2);
			cmdbuf[1] = res;
			cmdbuf[2] = IPC_Desc_Buffer(size, IPC_BUFFER_W);
			cmdbuf[3] = (u32)buf;
		}
		break;
	case 0x0002: // write register(s)
		{
			CHECK_HEADER(0x0002, 2, 2)
			
			CHECK_WRONGARG(
				!IPC_VerifyBuffer(cmdbuf[3], IPC_BUFFER_R) ||
				IPC_GetBufferSize(cmdbuf[3]) != cmdbuf[2]
			)
			
			u8 regid = (u8)cmdbuf[1] & 0xFF;
			u32 size = IPC_GetBufferSize(cmdbuf[3]);
			void *buf = (void *)cmdbuf[4];
			
			Result res = mcuWriteRegisterBuffer_l(regid, buf, size);
			
			cmdbuf[0] = IPC_MakeHeader(0x0002, 1, 2);
			cmdbuf[1] = res;
			cmdbuf[2] = IPC_Desc_Buffer(size, IPC_BUFFER_R);
			cmdbuf[3] = (u32)buf;
		}
		break;
	case 0x0003: // read info registers
		{
			CHECK_HEADER(0x0003, 1, 2)
			
			CHECK_WRONGARG(
				!IPC_VerifyBuffer(cmdbuf[2], IPC_BUFFER_W) ||
				IPC_GetBufferSize(cmdbuf[2]) != cmdbuf[1]
			);
			
			u8 size = (u8)cmdbuf[1] & 0xFF;
			void *buf = (void *)cmdbuf[3];
			
			Result res = mcuReadInfoRegisters(buf, size, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0003, 1, 2);
			cmdbuf[1] = res;
			cmdbuf[2] = IPC_Desc_Buffer(size, IPC_BUFFER_W);
			cmdbuf[3] = (u32)buf;
		}
		break;
	case 0x0004: // read battery voltage (in 20mV units)
		{
			CHECK_HEADER(0x0004, 0, 0)
			
			u8 value = 0;
			
			Result res = mcuReadBatteryVoltage(&value, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0004, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)value;
		}
		break;
	case 0x0005: // read battery percentage (integer part)
		{
			CHECK_HEADER(0x0005, 0, 0)
			
			u8 value = 0;
			
			Result res = mcuReadBatteryPercentageInt(&value, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0005, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)value;
		}
		break;
	case 0x0006: // set power LED state
		{
			CHECK_HEADER(0x0006, 1, 0)
			
			u8 state = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetLedState(MCUREG_POWER_LED_STATE, state, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0006, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0007: // set WiFi LED state
		{
			CHECK_HEADER(0x0007, 1, 0)
			
			u8 state = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetLedState(MCUREG_WLAN_LED_STATE, state, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0007, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0008: // set camera LED state
		{
			CHECK_HEADER(0x0008, 1, 0)
			
			u8 state = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetLedState(MCUREG_CAMERA_LED_STATE, state, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0008, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x0009: // set 3D LED state
		{
			CHECK_HEADER(0x0009, 1, 0)
			
			u8 state = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetLedState(MCUREG_3D_LED_STATE, state, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0009, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x000A: // set notification LED config
		{
			CHECK_HEADER(0x000A, 25, 0)
			
			MCU_NotificationLedData data = { 0 };
			
			_memcpy32_aligned(&data, &cmdbuf[1], sizeof(MCU_NotificationLedData));
			
			Result res = mcuSetNotificationLedData(&data, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x000A, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x000B: // read volume slider position
		{
			CHECK_HEADER(0x000B, 0, 0)
			
			u8 pos = 0;
			
			Result res = mcuReadVolumeSliderPositiion(&pos, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x000B, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)pos;
		}
		break;
	case 0x000C: // set top LCD flicker/VCOM
		{
			CHECK_HEADER(0x000C, 1, 0)
			
			u8 value = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetLcdFlicker(true, value, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x000C, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x000D: // set bottom LCD flicker/VCOM
		{
			CHECK_HEADER(0x000D, 1, 0)
			
			u8 value = (u8)cmdbuf[1] & 0xFF;
			
			Result res = mcuSetLcdFlicker(false, value, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x000D, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	case 0x000E: // read battery PCB temperature
		{
			CHECK_HEADER(0x000E, 0, 0)
			
			u8 value = 0;
			
			Result res = mcuReadBatteryPcbTemperature(&value, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x000E, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)value;
		}
		break;
	case 0x000F: // read RTC time (full)
		{
			CHECK_HEADER(0x000F, 0, 0)
			
			MCU_RtcData data = { 0 };
			
			Result res = mcuGetRtcTime(&data, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x000F, 3, 0);
			cmdbuf[1] = res;
			_memcpy(&cmdbuf[2], &data, sizeof(MCU_RtcData));
		}
		break;
	case 0x0010: // get major MCU firmware version
		{
			CHECK_HEADER(0x0010, 0, 0)
			
			u8 value = 0;
			
			Result res = mcuReadFwVerHigh(&value, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0010, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)value;
		}
		break;
	case 0x0011: // get minor MCU firmware version
		{
			CHECK_HEADER(0x0011, 0, 0)
			
			u8 value = 0;
			
			Result res = mcuReadFwVerLow(&value, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0011, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)value;
		}
		break;
	}
}

void MCUPLS_HandleIPC()
{
	u32 *cmdbuf = getThreadCommandBuffer();
	u32 cmd_header = cmdbuf[0];
	u16 cmd_id = (cmd_header >> 16) & 0xFFFF;
	
	switch (cmd_id)
	{
	case 0x0001: // get RTC time (full)
		{
			CHECK_HEADER(0x0001, 0, 0)
			
			MCU_RtcData data = { 0 };
			
			Result res = mcuGetRtcTime(&data, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0001, 3, 0);
			cmdbuf[1] = res;
			_memcpy(&cmdbuf[2], &data, sizeof(MCU_RtcData));
		}
		break;
	case 0x0002: // get RTC time (second part)
		{
			CHECK_HEADER(0x0002, 0, 0)
			
			u8 second = 0;
			
			Result res = mcuGetRtcTimeField(MCUREG_RTC_TIME_SECOND, &second, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0002, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)second;
		}
		break;
	case 0x0003: // get RTC time (minute part)
		{
			CHECK_HEADER(0x0003, 0, 0)
			
			u8 minute = 0;
			
			Result res = mcuGetRtcTimeField(MCUREG_RTC_TIME_MINUTE, &minute, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0003, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)minute;
		}
		break;
	case 0x0004: // get RTC time (hour part)
		{
			CHECK_HEADER(0x0004, 0, 0)
			
			u8 hour = 0;
			
			Result res = mcuGetRtcTimeField(MCUREG_RTC_TIME_HOUR, &hour, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0004, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)hour;
		}
		break;
	case 0x0005: // get RTC time (weekday part)
		{
			CHECK_HEADER(0x0005, 0, 0)
			
			u8 weekday = 0;
			
			Result res = mcuGetRtcTimeField(MCUREG_RTC_TIME_WEEKDAY, &weekday, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0005, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)weekday;
		}
		break;
	case 0x0006: // get RTC time (day part)
		{
			CHECK_HEADER(0x0006, 0, 0)
			
			u8 day = 0;
			
			Result res = mcuGetRtcTimeField(MCUREG_RTC_TIME_DAY, &day, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0006, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)day;
		}
		break;
	case 0x0007: // get RTC time (month part)
		{
			CHECK_HEADER(0x0007, 0, 0)
			
			u8 month = 0;
			
			Result res = mcuGetRtcTimeField(MCUREG_RTC_TIME_MONTH, &month, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0007, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)month;
		}
		break;
	case 0x0008: // get RTC time (year since 2000 part)
		{
			CHECK_HEADER(0x0008, 0, 0)
			
			u8 year = 0;
			
			Result res = mcuGetRtcTimeField(MCUREG_RTC_TIME_YEAR, &year, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0008, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = (u32)year;
		}
		break;
	case 0x0009: // get tick counter
		{
			CHECK_HEADER(0x0009, 0, 0)
			
			u16 value = 0;
			
			Result res = mcuGetTickCounter(&value, LOCK);
			
			cmdbuf[0] = IPC_MakeHeader(0x0009, 2, 0);
			cmdbuf[1] = res;
			cmdbuf[2] = value;
		}
		break;
	}
}

void MCUCDC_HandleIPC()
{
	u32 *cmdbuf = getThreadCommandBuffer();
	u32 cmd_header = cmdbuf[0];
	u16 cmd_id = (cmd_header >> 16) & 0xFFFF;
	
	switch (cmd_id)
	{
	case 0x0001: // unknown/unused?
		{
			CHECK_HEADER(0x0001, 0, 0)
			
			Result res = mcuSetRegisterBits8_l(MCUREG_UNK_26, 0x10, 0x10); /* what is this? */
			
			cmdbuf[0] = IPC_MakeHeader(0x0001, 1, 0);
			cmdbuf[1] = res;
		}
		break;
	}
}

void (* MCU_IPCHandlers[9])() =
{
	&MCUCAM_HandleIPC,
	&MCUGPU_HandleIPC,
	&MCUHID_HandleIPC,
	&MCURTC_HandleIPC,
	&MCUSND_HandleIPC,
	&MCUNWM_HandleIPC,
	&MCUHWC_HandleIPC,
	&MCUPLS_HandleIPC,
	&MCUCDC_HandleIPC
};
