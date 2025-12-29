#include <mcu/globals.h>
#include <mcu/mcu.h>
#include <mcu/acc.h>
#include <3ds/srv.h>
#include <3ds/i2c.h>
#include <3ds/err.h>
#include <3ds/gpio.h>
#include <util.h>

RecursiveLock g_I2CLock;
RecursiveLock g_GPIOLock;
RecursiveLock g_ExclusiveIRQLock;

LightEvent g_AccelerometerManualI2CEvent;

Handle g_GPIO_MCUInterruptEvent;

Handle g_IRQEvents[3];
u32 g_ReceivedIRQs[3];

bool g_McuFirmWasUpdated;

// i2c mcu
inline Result mcuSetRegisterBits8(u8 regid, u8 mask, u8 data)
{
	return I2C_SetRegisterBits8(I2C_DEVICE_MCU, regid, mask, data);
}

Result mcuSetRegisterBits8_l(u8 regid, u8 mask, u8 data)
{
	I2C_LOCKED_R(
		mcuSetRegisterBits8(regid, mask, data)
	)
}

inline Result mcuDisableRegisterBits8(u8 regid, u8 mask)
{
	return I2C_DisableRegisterBits8(I2C_DEVICE_MCU, regid, mask);
}

Result mcuDisableRegisterBits8_l(u8 regid, u8 mask)
{
	I2C_LOCKED_R(
		mcuDisableRegisterBits8(regid, mask)
	);
}

inline Result mcuWriteRegisterBuffer8(u8 regid, const void *buf, u32 size)
{
	return I2C_WriteRegisterBuffer8(I2C_DEVICE_MCU, regid, buf, size);
}

Result mcuWriteRegisterBuffer8_l(u8 regid, const void *buf, u32 size)
{
	I2C_LOCKED_R(
		mcuWriteRegisterBuffer8(regid, buf, size)
	)
}

inline Result mcuReadRegisterBuffer8(u8 regid, void *buf, u32 size)
{
	return I2C_ReadRegisterBuffer8(I2C_DEVICE_MCU, regid, buf, size);
}

Result mcuReadRegisterBuffer8_l(u8 regid, void *buf, u32 size)
{
	I2C_LOCKED_R(
		mcuReadRegisterBuffer8(regid, buf, size);
	);
}

inline Result mcuWriteRegisterBuffer(u8 regid, const void *buf, u32 size)
{
	return I2C_WriteRegisterBuffer(I2C_DEVICE_MCU, regid, buf, size);
}

Result mcuWriteRegisterBuffer_l(u8 regid, const void *buf, u32 size)
{
	I2C_LOCKED_R(
		mcuWriteRegisterBuffer(regid, buf, size)
	);
}

inline Result mcuReadRegisterBuffer(u8 regid, void *buf, u32 size)
{
	return I2C_ReadRegisterBuffer(I2C_DEVICE_MCU, regid, buf, size);
}

Result mcuReadRegisterBuffer_l(u8 regid, void *buf, u32 size)
{
	I2C_LOCKED_R(
		mcuReadRegisterBuffer(regid, buf, size)
	);
}

// gpio

inline Result gpioMcuSetRegPart1(u32 value, u32 mask)
{
	return GPIO_SetRegPart1(value, mask);
}

Result gpioMcuSetRegPart1_l(u32 value, u32 mask)
{
	GPIO_LOCKED_R(
		gpioMcuSetRegPart1(value, mask)
	);
}

inline Result gpioMcuSetInterruptMask(u32 value, u32 mask)
{
	return GPIO_SetInterruptMask(value, mask);
}

Result gpioMcuSetInterruptMask_l(u32 value, u32 mask)
{
	GPIO_LOCKED_R(
		gpioMcuSetInterruptMask(value, mask)
	);
}

inline Result gpioMcuReadData(u32 *out_value, u32 mask)
{
	return GPIO_ReadData(out_value, mask);
}

Result gpioMcuReadData_l(u32 *out_value, u32 mask)
{
	GPIO_LOCKED_R(
		gpioMcuReadData(out_value, mask)
	);
}

inline Result gpioMcuWriteData(u32 value, u32 mask)
{
	return GPIO_WriteData(value, mask);
}

Result gpioMcuWriteData_l(u32 value, u32 mask)
{
	GPIO_LOCKED_R(
		gpioMcuWriteData(value, mask)
	);
}

inline Result gpioMcuBindInterrupt(u32 irq_mask, Handle syncobj, s32 priority)
{
	return GPIO_BindInterrupt(irq_mask, syncobj, priority);
}

Result gpioMcuBindInterrupt_l(u32 irq_mask, Handle syncobj, s32 priority)
{
	GPIO_LOCKED_R(
		gpioMcuBindInterrupt(irq_mask, syncobj, priority)
	);
}

inline Result gpioMcuUnbindInterrupt(u32 irq_mask, Handle syncobj)
{
	return GPIO_UnbindInterrupt(irq_mask, syncobj);
}

Result gpioMcuUnbindInterrupt_l(u32 irq_mask, Handle syncobj)
{
	GPIO_LOCKED_R(
		gpioMcuUnbindInterrupt(irq_mask, syncobj)
	);
}

/* Non Fatal assert */
#define NF(expr) do { \
	Result __tmp = expr; \
	if (R_FAILED(__tmp) && (R_LEVEL(__tmp) != RL_INFO || R_LEVEL(__tmp) != RL_STATUS)) \
		Err_Throw(__tmp); \
	} while (0);

void mcuHandleInterruptEvents(u32 received_irqs)
{
	static const u32 filtered_events[3] = {
		[EVENT_GPU] = MCUINT_VIDEO_LCD_PUSH_POWER_OFF | MCUINT_VIDEO_LCD_PUSH_POWER_ON |
                      MCUINT_VIDEO_BOT_BACKLIGHT_OFF | MCUINT_VIDEO_BOT_BACKLIGHT_ON |
                      MCUINT_VIDEO_TOP_BACKLIGHT_OFF | MCUINT_VIDEO_TOP_BACKLIGHT_ON,
                      
		[EVENT_HID] = MCUINT_ACCELEROMETER_I2C_MANUAL_IO | MCUINT_ACCELEROMETER_NEW_SAMPLE,
		
		[EVENT_POWER] = MCUINT_POWER_BUTTON_HELD | MCUINT_AC_ADAPTER_REMOVED |
                        MCUINT_AC_ADAPTER_PLUGGED_IN | MCUINT_RTC_ALARM |
                        MCUINT_CRITICAL_BATTERY | MCUINT_CHARGING_STOP | MCUINT_CHARGING_START,
	};
	
	/* seems this "feature" is meant for the separate IPC command, not used elsewhere */
	if (received_irqs == 0xFFFFFFFF)
		T(mcuGetReceivedIrqs(&received_irqs, LOCK));
	
	for (int i = 0; i < 3; i++) {
		if (received_irqs & filtered_events[i]) {
			g_ReceivedIRQs[i] |= (received_irqs & filtered_events[i]);
			T(svcSignalEvent(g_IRQEvents[i]));
		}
		
		/* special handling for HID, as that's signaled through the LightEvent later */
		if (i == EVENT_HID) {
			if (received_irqs & MCUINT_ACCELEROMETER_NEW_SAMPLE) {
				g_ReceivedIRQs[EVENT_HID] |= MCUINT_ACCELEROMETER_NEW_SAMPLE;
				T(svcSignalEvent(g_IRQEvents[EVENT_HID]));
			}
		}
	}
	
	if (received_irqs & MCUINT_ACCELEROMETER_I2C_MANUAL_IO)
		LightEvent_Signal(&g_AccelerometerManualI2CEvent);
	
	if (received_irqs & (MCUINT_SHELL_CLOSE | MCUINT_SHELL_OPEN))
		NF(SRV_PublishToSubscriber(MCUNOTIF_SHELL_STATE_CHANGE, SRVNOTIF_ONLY_IF_NOT_PENDING));
	
	if (received_irqs & MCUINT_POWER_BUTTON_PRESS)
		NF(SRV_PublishToSubscriber(MCUNOTIF_POWER_BUTTON_PRESSED, 0));
	
	if (received_irqs & MCUINT_POWER_BUTTON_HELD) {
		T(mcuSetFirmFlag(MCU_FIRMFLG_POWEROFF_INITIATED, true, LOCK));
		NF(SRV_PublishToSubscriber(MCUNOTIF_POWER_BUTTON_HELD, 0));
	}
	
	if (received_irqs & MCUINT_HOME_BUTTON_PRESS)
		NF(SRV_PublishToSubscriber(MCUNOTIF_HOME_BUTTON_PRESSED, SRVNOTIF_ONLY_IF_NOT_PENDING));
	
	if (received_irqs & MCUINT_HOME_BUTTON_RELEASE)
		NF(SRV_PublishToSubscriber(MCUNOTIF_HOME_BUTTON_RELEASED, SRVNOTIF_ONLY_IF_NOT_PENDING));
	
	if (received_irqs & MCUINT_WLAN_SWITCH_TRIGGER)
		NF(SRV_PublishToSubscriber(MCUNOTIF_WLAN_SWITCH_TRIGGERED, SRVNOTIF_ONLY_IF_NOT_PENDING));
	
	if (received_irqs & MCUINT_FATAL_HW_ERROR)
		NF(SRV_PublishToSubscriber(MCUNOTIF_FATAL_HW_ERROR, 0));
	
	if (received_irqs & MCUINT_AC_ADAPTER_REMOVED)
		NF(SRV_PublishToSubscriber(MCUNOTIF_AC_ADAPTER_REMOVED, 0));
	
	if (received_irqs & MCUINT_AC_ADAPTER_PLUGGED_IN)
		NF(SRV_PublishToSubscriber(MCUNOTIF_AC_ADAPTER_CONNECTED, 0));
	
	if (received_irqs & MCUINT_CHARGING_START)
		NF(SRV_PublishToSubscriber(MCUNOTIF_STARTED_CHARGING, 0));
	
	if (received_irqs & MCUINT_CHARGING_STOP)
		NF(SRV_PublishToSubscriber(MCUNOTIF_STOPPED_CHARGING, 0));
}

// reg wrappers

#define L(func, ...) lock ? func ## _l(__VA_ARGS__) : func(__VA_ARGS__)

static Result _mcuUpdateFirmware(const void *payload, u32 payload_size)
{
	u8 legacy_ver_high = 0, legacy_ver_low = 0;
	u8 target_reg = 0;
	Result res = mcuReadRegisterBuffer8(MCUREG_LEGACY_VERSION_HIGH, &legacy_ver_high, sizeof(u8));
	if (R_FAILED(res)) return res;
	
	res = mcuReadRegisterBuffer8(MCUREG_LEGACY_VERSION_LOW, &legacy_ver_low, sizeof(u8));
	if (R_FAILED(res)) return res;
	
	if (legacy_ver_high == 0 && legacy_ver_low == 1)
		target_reg = MCUREG_LEGACY_FIRM_UPLOAD;
	else target_reg = MCUREG_FIRMWARE_UPLOAD_0;
	
	res = mcuWriteRegisterBuffer(target_reg, payload, payload_size);
	
	if (R_SUCCEEDED(res))
		svcSleepThread(1000000000LL); // wait 1 second for the mcu to get back on its feet
	
	return res;
}

Result mcuUpdateFirmware(const void *payload, u32 payload_size, bool locked)
{
	if (locked) {
		I2C_LOCKED_R(
			_mcuUpdateFirmware(payload, payload_size)
		)
	}
	
	return _mcuUpdateFirmware(payload, payload_size);
}

inline Result mcuReadFwVerHigh(u8 *out_value, bool lock)
{
	return L(mcuReadRegisterBuffer8, MCUREG_VERSION_HIGH, out_value, sizeof(u8));
}

inline Result mcuReadFwVerLow(u8 *out_value, bool lock)
{
	return L(mcuReadRegisterBuffer8, MCUREG_VERSION_LOW, out_value, sizeof(u8));
}

inline Result mcuGetResetEventFlags(u8 *out_value, bool lock)
{
	return L(mcuReadRegisterBuffer8, MCUREG_RESET_EVENTS, out_value, sizeof(u8));
}

inline Result mcuClearResetEventFlag(u8 to_clear, bool lock)
{
	return L(mcuDisableRegisterBits8, MCUREG_RESET_EVENTS, to_clear);
}

inline Result mcuSetLcdFlicker(bool top, u8 value, bool lock)
{
	return L(mcuWriteRegisterBuffer8, top ? MCUREG_VCOM_TOP : MCUREG_VCOM_BOTTOM, &value, sizeof(u8));
}

inline Result mcuGetLcdFlicker(bool top, u8 *out_value, bool lock)
{
	return L(mcuReadRegisterBuffer8, top ? MCUREG_VCOM_TOP : MCUREG_VCOM_BOTTOM, out_value, sizeof(u8));
}

inline Result mcuRead3dSliderPosition(u8 *out_pos, bool lock)
{
	return L(mcuReadRegisterBuffer8, MCUREG_3D_SLIDER_POSITION, out_pos, sizeof(u8));
}

inline Result mcuReadVolumeSliderPositiion(u8 *out_pos, bool lock)
{
	return L(mcuReadRegisterBuffer8, MCUREG_VOLUME_SLIDER_POSITION, out_pos, sizeof(u8));
}

inline Result mcuSetBacklightPowerState(u8 top_bl_on, u8 bottom_bl_on, bool lock)
{
	u8 triggers = 0;
	
	if (top_bl_on) triggers |= MCU_LCDPWR_TOP_BL_ON;
	else           triggers |= MCU_LCDPWR_TOP_BL_OFF;
	
	if (bottom_bl_on) triggers |= MCU_LCDPWR_BOTTOM_BL_ON;
	else              triggers |= MCU_LCDPWR_BOTTOM_BL_OFF;
	
	return L(mcuWriteRegisterBuffer8, MCUREG_LCD_PWR_CTL, &triggers, sizeof(u8));
}

inline Result mcuSetPowerState(u8 triggers, bool lock)
{
	return L(mcuWriteRegisterBuffer8, MCUREG_PWR_CTL, &triggers, sizeof(u8));
}

inline Result mcuSetLcdPowerState(u8 state, bool lock)
{
	u8 trigger = 0;
	if (state) trigger |= MCU_LCDPWR_POWER_ON;
	else       trigger |= MCU_LCDPWR_POWER_OFF;
	
	return L(mcuWriteRegisterBuffer8, MCUREG_LCD_PWR_CTL, &trigger, sizeof(u8));
}

inline Result mcuReset(bool lock)
{
	u8 value = 'r'; // 0x78
	return L(mcuWriteRegisterBuffer8, MCUREG_MCU_RESET_CTL, &value, sizeof(u8));
}

inline Result mcuSetForceShutdownDelay(u8 value, bool lock)
{
	return L(mcuWriteRegisterBuffer8, MCUREG_FORCE_SHUTDOWN_DELAY, &value, sizeof(u8));
}

inline Result mcuSetUnkVolume25(u8 value, bool lock)
{
	/* upper bound = 63? */
	return L(mcuSetRegisterBits8, MCUREG_VOLUME_UNK_25, 0x3F, value);
}

inline Result mcuGetUnkVolume25(u8 *out_value, bool lock)
{
	Result res = L(mcuReadRegisterBuffer8, MCUREG_VOLUME_UNK_25, out_value, sizeof(u8));
	if (R_FAILED(res)) return res;
	*out_value &= 0x3F; /* upper bound = 63? */
	return res;
}

inline Result mcuGetForceShutdownDelay(u8 *out_value, bool lock)
{
	return L(mcuReadRegisterBuffer8, MCUREG_FORCE_SHUTDOWN_DELAY, out_value, sizeof(u8));
}

inline Result mcuReadBatteryPcbTemperature(u8 *out_value, bool lock)
{
	return L(mcuReadRegisterBuffer8, MCUREG_BATTERY_PCB_TEMPERATURE, out_value, sizeof(u8));
}

inline Result mcuReadBatteryPercentageInt(u8 *out_value, bool lock)
{
	return L(mcuReadRegisterBuffer8, MCUREG_BATTERY_PERCENTAGE_INT, out_value, sizeof(u8));
}

inline Result mcuReadBatteryVoltage(u8 *out_value, bool lock)
{
	return L(mcuReadRegisterBuffer8, MCUREG_BATTERY_VOLTAGE, out_value, sizeof(u8));
}

inline Result mcuGetPowerStatus(u8 *out_status, bool lock)
{
	return L(mcuReadRegisterBuffer8, MCUREG_POWER_STATUS, out_status, sizeof(u8));
}

inline Result mcuGetReceivedIrqs(u32 *out_events, bool lock)
{
	return L(mcuReadRegisterBuffer8, MCUREG_RECEIVED_IRQS, out_events, sizeof(u32));
}

inline Result mcuSetInterruptMask(u32 enabled_interrupts, bool lock)
{
	enabled_interrupts = ~(enabled_interrupts & ~(MCUINT_MCU_SYSMODULE_0 | MCUINT_MCU_SYSMODULE_1));
	return L(mcuWriteRegisterBuffer8, MCUREG_IRQ_MASK, &enabled_interrupts, sizeof(u32));
}

inline Result mcuGetInterruptMask(u32 *out_enabled_interrupts, bool lock)
{
	Result res = L(mcuReadRegisterBuffer8, MCUREG_IRQ_MASK, out_enabled_interrupts, sizeof(u32));
	if (R_FAILED(res)) return res;
	*out_enabled_interrupts = ~(*out_enabled_interrupts) & ~(MCUINT_MCU_SYSMODULE_0 | MCUINT_MCU_SYSMODULE_1);
	return res;
}

inline Result mcuSetWlanLedState(u8 state, bool lock)
{
	return L(mcuWriteRegisterBuffer8, MCUREG_WLAN_LED_STATE, &state, sizeof(u8));
}

inline Result mcuGetWlanLedState(u8 *out_state, bool lock)
{
	return L(mcuReadRegisterBuffer8, MCUREG_WLAN_LED_STATE, out_state, sizeof(u8));
}

inline Result mcuSetLedState(u8 led_regid, u8 state, bool lock)
{
	return L(mcuWriteRegisterBuffer8, led_regid, &state, sizeof(u8));
}

inline Result mcuGetLedState(u8 led_regid, u8 *out_state, bool lock)
{
	return L(mcuReadRegisterBuffer8, led_regid, out_state, sizeof(u8));
}

static Result _mcuSetPowerLedBlinkPattern(u32 blink_pattern)
{
	MCU_PowerLedConfig config = { 0 };
	Result res = mcuReadRegisterBuffer8(MCUREG_POWER_LED_STATE, &config.mode, sizeof(u8));
	if (R_FAILED(res)) return res;
	
	config.blink_pattern = blink_pattern;
	return mcuWriteRegisterBuffer8(MCUREG_POWER_LED_STATE, &config, sizeof(MCU_PowerLedConfig));
}

inline Result mcuSetPowerLedBlinkPattern(u32 blink_pattern, bool lock)
{
	if (lock) {
		I2C_LOCKED_R(
			_mcuSetPowerLedBlinkPattern(blink_pattern);
		);
	}
	
	return _mcuSetPowerLedBlinkPattern(blink_pattern);
}

inline Result mcuSetNotificationLedData(MCU_NotificationLedData *data, bool lock)
{
	return L(mcuWriteRegisterBuffer, MCUREG_NOTIFICATION_LED_STATE, data, sizeof(MCU_NotificationLedData));
}

inline Result mcuSetNotificationLedAnimation(MCU_NotifictationLedAnimation *animation, bool lock)
{
	return L(mcuWriteRegisterBuffer8, MCUREG_NOTIFICATION_LED_STATE, animation, sizeof(MCU_NotifictationLedAnimation));
}

inline Result mcuGetNotificationLedCycleState(u8 *out_state, bool lock)
{
	return L(mcuReadRegisterBuffer8, MCUREG_NOTIFICATION_LED_CYCLE_STATE, out_state, sizeof(u8));
}

inline Result mcuSetRtcTime(MCU_RtcData *data, bool lock)
{
	data->second = INT2BCD(data->second);
	data->minute = INT2BCD(data->minute);
	data->hour = INT2BCD(data->hour);
	data->weekday = INT2BCD(data->weekday);
	data->monthday = INT2BCD(data->monthday);
	data->month = INT2BCD(data->month);
	data->year = INT2BCD(data->year);
	
	return L(mcuWriteRegisterBuffer8, MCUREG_RTC_TIME_SECOND, data, sizeof(MCU_RtcData));
}

inline Result mcuGetRtcTime(MCU_RtcData *out_data, bool lock)
{
	Result res = L(mcuReadRegisterBuffer8, MCUREG_RTC_TIME_SECOND, out_data, sizeof(MCU_RtcData));
	if (R_FAILED(res)) return res;
	
	out_data->second = BCD2INT(out_data->second);
	out_data->minute = BCD2INT(out_data->minute);
	out_data->hour = BCD2INT(out_data->hour);
	out_data->weekday = BCD2INT(out_data->weekday);
	out_data->monthday = BCD2INT(out_data->monthday);
	out_data->month = BCD2INT(out_data->month);
	out_data->year = BCD2INT(out_data->year);
	return res;
}

Result mcuSetRtcTimeField(u8 field_regid, u8 value, bool lock)
{
	value = INT2BCD(value);
	return L(mcuWriteRegisterBuffer8, field_regid, &value, sizeof(u8));
}

Result mcuGetRtcTimeField(u8 field_regid, u8 *out_value, bool lock)
{
	Result res = L(mcuReadRegisterBuffer8, field_regid, out_value, sizeof(u8));
	if (R_FAILED(res)) return res;
	*out_value = BCD2INT(*out_value);
	return res;
}

inline Result mcuSetRtcTimeCorrection(u8 value, bool lock)
{
	return L(mcuWriteRegisterBuffer8, MCUREG_RTC_TIME_CORRECTION, &value, sizeof(u8));
}

inline Result mcuGetRtcTimeCorrection(u8 *out_value, bool lock)
{
	return L(mcuReadRegisterBuffer8, MCUREG_RTC_TIME_CORRECTION, out_value, sizeof(u8));
}

inline Result mcuSetRtcAlarm(MCU_RtcAlarm *alarm, bool lock)
{
	MCU_RtcAlarm bcdalarm = { 
		INT2BCD(alarm->minute),
		INT2BCD(alarm->hour),
		INT2BCD(alarm->day),
		INT2BCD(alarm->month),
		INT2BCD(alarm->year)
	};
	
	return L(mcuWriteRegisterBuffer8, MCUREG_RTC_ALARM_MINUTE, &bcdalarm, sizeof(MCU_RtcAlarm));
}

inline Result mcuGetRtcAlarm(MCU_RtcAlarm *alarm, bool lock)
{
	MCU_RtcAlarm bcdalarm;
	
	Result res = L(mcuReadRegisterBuffer8, MCUREG_RTC_ALARM_MINUTE, &bcdalarm, sizeof(MCU_RtcAlarm));
	if (R_FAILED(res)) return res;
	
	alarm->minute = BCD2INT(bcdalarm.minute);
	alarm->hour   = BCD2INT(bcdalarm.hour);
	alarm->day    = BCD2INT(bcdalarm.day);
	alarm->month  = BCD2INT(bcdalarm.month);
	alarm->year   = BCD2INT(bcdalarm.year);
	
	return res;
}

inline Result mcuSetRtcAlarmField(u8 field_regid, u8 value, bool lock)
{
	value = INT2BCD(value);
	return L(mcuWriteRegisterBuffer8, field_regid, &value, sizeof(u8));
}

inline Result mcuGetRtcAlarmField(u8 field_regid, u8 *out_value, bool lock)
{
	Result res = L(mcuReadRegisterBuffer8, field_regid, out_value, sizeof(u8));
	if (R_FAILED(res)) return res;
	*out_value = BCD2INT(*out_value);
	return res;
}

inline Result mcuGetTickCounter(u16 *out_value, bool lock)
{
	/* u16 spread across two registers */
	return L(mcuReadRegisterBuffer8, MCUREG_TICK_COUNTER_LSB, out_value, sizeof(u16));
}

inline Result mcuSetOmeterMode(u8 mode, bool lock)
{
	return L(mcuWriteRegisterBuffer8, MCUREG_OMETER_MODE, &mode, sizeof(u8));
}

inline Result mcuSetPedometerEnabled(u8 enabled, bool lock)
{
	return L(mcuSetRegisterBits8, MCUREG_OMETER_MODE, MCU_OMETER_PEDOMETER_ON, enabled ? MCU_OMETER_PEDOMETER_ON : 0);
}

inline Result mcuGetPedometerEnabled(u8 *out_enabled, bool lock)
{
	u8 ometer_mode = 0;
	Result res = L(mcuReadRegisterBuffer8, MCUREG_OMETER_MODE, &ometer_mode, sizeof(u8));
	if (R_FAILED(res)) return res;
	*out_enabled = (ometer_mode & MCU_OMETER_PEDOMETER_ON) == MCU_OMETER_PEDOMETER_ON;
	return res;
}

inline Result mcuReadPedometerStepCount(u32 *out_count, bool lock)
{
	/* the step count is a u24 spread across 3 registers */
	*out_count = 0;
	return L(mcuReadRegisterBuffer8, MCUREG_PEDOMETER_STEPS_LOWBYTE, out_count, 3);
}

inline Result mcuReadPedoemterStepData(MCU_PedometerStepData *out_data, bool lock)
{
	Result res = L(mcuReadRegisterBuffer, MCUREG_PEDOMETER_STEP_DATA, out_data, sizeof(MCU_PedometerStepData));
	if (R_FAILED(res)) return res;
	
	struct MCU_PedometerTime *time = &out_data->pedometer_time;
	
	time->hour = BCD2INT(time->hour);
	time->monthday = BCD2INT(time->monthday);
	time->month = BCD2INT(time->month);
	time->year = BCD2INT(time->year);
	time->minute = BCD2INT(time->minute);
	time->second = BCD2INT(time->second);
	return res;
}

inline Result mcuClearPedometerStepData(bool lock)
{
	u8 cnt = MCU_PEDOMETER_CLEAR;
	
	return L(mcuWriteRegisterBuffer8, MCUREG_PEDOMETER_CNT, &cnt, sizeof(u8));
}

inline Result mcuGetOmeterMode(u8 *out_mode, bool lock)
{
	return L(mcuReadRegisterBuffer8, MCUREG_OMETER_MODE, out_mode, sizeof(u8));
}

inline Result mcuStartAccelerometerManualRegRead(u8 hw_regid, bool lock)
{
	return L(mcuWriteRegisterBuffer8, MCUREG_ACCELEROMETER_MANUAL_REGID_R, &hw_regid, sizeof(u8));
}

inline Result mcuGetAccelerometerManualRegReadResult(u8 *out_data, bool lock)
{
	return L(mcuReadRegisterBuffer8, MCUREG_ACCELEROMETER_MANUAL_IO, out_data, sizeof(u8));
}

inline Result mcuPerformAccelerometerManualWrite(u8 hw_regid, u8 data, bool lock)
{
	u8 regs[2] = { hw_regid, data };
	
	return L(mcuWriteRegisterBuffer8, MCUREG_ACCELEROMETER_MANUAL_REGID_W, &regs, sizeof(regs));
}

static Result _mcuAccelerometerPerformManualRead(u8 hw_regid, u8 *out_data)
{
	if (hw_regid > 0x37)
		return ACC_BAD_REG_ID;
	
	LightEvent_Clear(&g_AccelerometerManualI2CEvent);
	
	Result res = mcuStartAccelerometerManualRegRead(hw_regid, NOLOCK);
	if (R_FAILED(res)) return res;
	
	LightEvent_Wait(&g_AccelerometerManualI2CEvent);
	return mcuGetAccelerometerManualRegReadResult(out_data, NOLOCK);
}

inline Result mcuPerformAccelerometerManualRead(u8 hw_regid, u8 *out_data, bool lock)
{
	if (lock) {
		I2C_LOCKED_R(
			_mcuAccelerometerPerformManualRead(hw_regid, out_data);
		);
	}
	
	return _mcuAccelerometerPerformManualRead(hw_regid, out_data);
}

static Result _mcuAccelerometerManualUpdateReg(u8 hw_regid, u32 to_set, u32 to_clear)
{
	if (hw_regid > 0x37)
		return ACC_BAD_REG_ID;
	
	u8 regdata = 0;
	
	Result res = mcuPerformAccelerometerManualRead(hw_regid, &regdata, NOLOCK);
	if (R_FAILED(res)) return res;
	
	svcSleepThread(100000);
	
	regdata &= ~(to_clear);
	regdata |= to_set;
	
	LightEvent_Clear(&g_AccelerometerManualI2CEvent);
	res = mcuPerformAccelerometerManualWrite(hw_regid, regdata, NOLOCK);
	if (R_FAILED(res)) return res;
	
	LightEvent_Wait(&g_AccelerometerManualI2CEvent);
	return res;
}

inline Result mcuAccelerometerManualUpdateReg(u8 hw_regid, u32 to_set, u32 to_clear, bool lock)
{
	if (lock) {
		I2C_LOCKED_R(
			_mcuAccelerometerManualUpdateReg(hw_regid, to_set, to_clear);
		);
	}
	
	return _mcuAccelerometerManualUpdateReg(hw_regid, to_set, to_clear);
}

inline Result mcuSetAccelerometerInternalFilterEnabled(u8 enabled, bool lock)
{
	return mcuAccelerometerManualUpdateReg(ACCREG_FILTER_CTL, enabled ? ACC_FILTERCTL_ENABLE_INTERNAL_FILTER : 0, ACC_FILTERCTL_ENABLE_INTERNAL_FILTER, lock);
}

inline Result mcuGetAccelerometerInternalFilterEnabled(u8 *out_enabled, bool lock)
{
	u8 value = 0;
	Result res = mcuPerformAccelerometerManualRead(ACCREG_FILTER_CTL, &value, lock);
	if (R_FAILED(res)) return res;
	
	*out_enabled = (value & ACC_FILTERCTL_ENABLE_INTERNAL_FILTER) == ACC_FILTERCTL_ENABLE_INTERNAL_FILTER;
	return res;
}

inline Result mcuSetAccelerometerScale(u8 scale, bool lock)
{
	if (scale != ACC_SCALE_2G && scale != ACC_SCALE_4G && scale != ACC_SCALE_8G)
		return ACC_BAD_INPUT_SCALE;
	
	return mcuAccelerometerManualUpdateReg(ACCREG_MISC_CTL, scale << 4, ACC_MISCCTL_SCALE_SELECT, lock);
}

inline Result mcuGetAccelerometerScale(u8 *out_scale, bool lock)
{
	u8 value = 0;
	Result res = mcuPerformAccelerometerManualRead(ACCREG_MISC_CTL, &value, lock);
	if (R_FAILED(res)) return res;
	
	value = (value & ACC_MISCCTL_SCALE_SELECT) >> 4;
	
	if (value == ACC_SCALE_RESERVED)
		return ACC_BAD_SCALE;
	
	*out_scale = value;
	return res;
}

inline Result mcuReadAccelerometerData(MCU_AccelerometerData *out_data, bool lock)
{
	Result res = L(mcuReadRegisterBuffer8, MCUREG_ACCELEROMETER_OUTPUT_X_LSB, out_data, sizeof(MCU_AccelerometerData));
	if (R_FAILED(res)) return res;
	out_data->x >>= 4;
	out_data->y >>= 4;
	out_data->z >>= 4;
	return res;
}

inline Result mcuSetPedometerWrapTimeMinute(u8 value, bool lock)
{
	value = INT2BCD(value);
	return L(mcuWriteRegisterBuffer8, MCUREG_PEDOMETER_WRAP_MINUTE, &value, sizeof(u8));
}

inline Result mcuGetPedometerWrapTimeMinute(u8 *out_value, bool lock)
{
	Result res = L(mcuReadRegisterBuffer8, MCUREG_PEDOMETER_WRAP_MINUTE, out_value, sizeof(u8));
	if (R_FAILED(res)) return res;
	*out_value = BCD2INT(*out_value);
	return res;
}

inline Result mcuSetPedometerWrapTimeSecond(u8 value, bool lock)
{
	value = INT2BCD(value);
	return L(mcuWriteRegisterBuffer8, MCUREG_PEDOMETER_WRAP_SECOND, &value, sizeof(u8));
}

inline Result mcuGetPedometerWrapTimeSecond(u8 *out_value, bool lock)
{
	Result res = L(mcuReadRegisterBuffer8, MCUREG_PEDOMETER_WRAP_SECOND, out_value, sizeof(u8));
	if (R_FAILED(res)) return res;
	*out_value = BCD2INT(*out_value);
	return res;
}

inline Result mcuSetVolumeCalibration(u8 min, u8 max, bool lock)
{
	u8 data[2] = { min, max };
	
	return L(mcuWriteRegisterBuffer8, MCUREG_VOLUME_CALIBRATION_MIN, &data, sizeof(data));
}

inline Result mcuGetVolumeCalibration(u8 *out_min, u8 *out_max, bool lock)
{
	u8 data[2] = { 0, 0 };
	
	Result res = L(mcuReadRegisterBuffer8, MCUREG_VOLUME_CALIBRATION_MIN, &data, sizeof(data));
	if (R_FAILED(res)) return res;
	
	*out_min = data[0];
	*out_max = data[1];
	return res;
}

static Result _mcuReadStorageArea(u8 offset, void *outbuf, u32 size)
{
	Result res = mcuWriteRegisterBuffer8(MCUREG_STORAGE_AREA_OFFSET, &offset, sizeof(u8));
	
	if (R_FAILED(res))
		return res;
	
	return mcuReadRegisterBuffer(MCUREG_STORAGE_AREA, outbuf, size);
}

inline Result mcuReadStorageArea(u8 offset, void *outbuf, u32 size, bool lock)
{
	if (lock) {
		I2C_LOCKED_R(
			_mcuReadStorageArea(offset, outbuf, size)
		);
	}
	
	return _mcuReadStorageArea(offset, outbuf, size);
}

static Result _mcuWriteStorageArea(u8 offset, void *outbuf, u32 size)
{
	Result res = mcuWriteRegisterBuffer8(MCUREG_STORAGE_AREA_OFFSET, &offset, sizeof(u8));
	
	if (R_FAILED(res)) return res;
	
	return mcuWriteRegisterBuffer(MCUREG_STORAGE_AREA, outbuf, size);
}

inline Result mcuWriteStorageArea(u8 offset, void *outbuf, u32 size, bool lock)
{
	if (lock) {
		I2C_LOCKED_R(
			_mcuWriteStorageArea(offset, outbuf, size)
		);
	}
	
	return _mcuWriteStorageArea(offset, outbuf, size);
}

static Result _mcuSetFirmFlag(u8 flag, bool set)
{
	u8 firmflags = 0;
	
	Result res = mcuReadStorageArea(offsetof(MCU_StorageArea, firm_flags), &firmflags, sizeof(u8), NOLOCK);
	if (R_FAILED(res)) return res;
	
	if (set) firmflags |= flag;
	else     firmflags &= (~flag);
	
	return mcuWriteStorageArea(offsetof(MCU_StorageArea, firm_flags), &firmflags, sizeof(u8), NOLOCK);
}

inline Result mcuSetFirmFlag(u8 flag, bool set, bool lock)
{
	if (lock) {
		I2C_LOCKED_R(
			_mcuSetFirmFlag(flag, set)
		);
	}
	
	return _mcuSetFirmFlag(flag, set);
}

inline Result mcuGetFirmFlag(bool *out_is_set, u8 flag, bool lock)
{
	u8 firmflags = 0;
	
	Result res = mcuReadStorageArea(offsetof(MCU_StorageArea, firm_flags), &firmflags, sizeof(u8), lock);
	if (R_FAILED(res)) return res;
	
	*out_is_set = CHECKBIT(firmflags, flag);
	return res;
}

inline Result mcuReadInfoRegisters(void *outbuf, u32 size, bool lock)
{
	return L(mcuReadRegisterBuffer8, MCUREG_INFO, outbuf, size);
}
