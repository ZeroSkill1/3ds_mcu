#ifndef _MCU_MCU_H
#define _MCU_MCU_H

#include <3ds/types.h>

#define I2C_LOCKED_R(...) { \
	do { \
		Result res; \
		RecursiveLock_Lock(&g_I2CLock); \
		res = __VA_ARGS__; \
		RecursiveLock_Unlock(&g_I2CLock); \
		return res; \
	} while(0); \
}

#define I2C_LOCKED(...) { \
	RecursiveLock_Lock(&g_I2CLock); \
	__VA_ARGS__; \
	RecursiveLock_Unlock(&g_I2CLock); \
}

#define GPIO_LOCKED_R(...) { \
	do { \
		Result res; \
		RecursiveLock_Lock(&g_GPIOLock); \
		res = __VA_ARGS__; \
		RecursiveLock_Unlock(&g_GPIOLock); \
		return res; \
	} while(0); \
}

#define GPIO_LOCKED(...) { \
	RecursiveLock_Lock(&g_GPIOLock); \
	__VA_ARGS__; \
	RecursiveLock_Unlock(&g_GPIOLock); \
}

/* MCU registers */
enum MCU_Interrupt
{
	MCUINT_POWER_BUTTON_PRESS          = BIT(0),
	MCUINT_POWER_BUTTON_HELD           = BIT(1),
	MCUINT_HOME_BUTTON_PRESS           = BIT(2),
	MCUINT_HOME_BUTTON_RELEASE         = BIT(3),
	MCUINT_WLAN_SWITCH_TRIGGER         = BIT(4),
	MCUINT_SHELL_CLOSE                 = BIT(5),
	MCUINT_SHELL_OPEN                  = BIT(6),
	MCUINT_FATAL_HW_ERROR              = BIT(7),
	MCUINT_AC_ADAPTER_REMOVED          = BIT(8),
	MCUINT_AC_ADAPTER_PLUGGED_IN       = BIT(9),
	MCUINT_RTC_ALARM                   = BIT(10),
	MCUINT_ACCELEROMETER_I2C_MANUAL_IO = BIT(11),
	MCUINT_ACCELEROMETER_NEW_SAMPLE    = BIT(12),
	MCUINT_CRITICAL_BATTERY            = BIT(13),
	MCUINT_CHARGING_STOP               = BIT(14),
	MCUINT_CHARGING_START              = BIT(15),
	/* what on earth are these? */
	//MCUINT_UNK16                     = BIT(16),
	//MCUINT_UNK17                     = BIT(17),
	//MCUINT_UNK18                     = BIT(18),
	//MCUINT_UNK19                     = BIT(19),
	//MCUINT_UNK20                     = BIT(20),
	//MCUINT_UNK21                     = BIT(21),
	MCUINT_VOL_SLIDER                  = BIT(22),
	MCUINT_BPTWL_VER_REG_READ          = BIT(23),
	MCUINT_VIDEO_LCD_PUSH_POWER_OFF    = BIT(24),
	MCUINT_VIDEO_LCD_PUSH_POWER_ON     = BIT(25),
	MCUINT_VIDEO_BOT_BACKLIGHT_OFF     = BIT(26),
	MCUINT_VIDEO_BOT_BACKLIGHT_ON      = BIT(27),
	MCUINT_VIDEO_TOP_BACKLIGHT_OFF     = BIT(28),
	MCUINT_VIDEO_TOP_BACKLIGHT_ON      = BIT(29),
	/* these are unused */
	MCUINT_MCU_SYSMODULE_0             = BIT(30),
	MCUINT_MCU_SYSMODULE_1             = BIT(31),
};


enum MCU_RegisterID {
	/* firmware version info */
	MCUREG_VERSION_HIGH                 = 0x0,
	MCUREG_VERSION_LOW                  = 0x1,
	
	/* reset event flags */
	MCUREG_RESET_EVENTS                 = 0x2,
	
	/* LCD VCOMs */
	MCUREG_VCOM_TOP                     = 0x3,
	MCUREG_VCOM_BOTTOM                  = 0x4,
	
	/* firmware upload */
	MCUREG_FIRMWARE_UPLOAD_0            = 0x5,
	MCUREG_FIRMWARE_UPLOAD_1            = 0x6,
	MCUREG_FIRMWARE_UPLOAD_2            = 0x7,
	
	/* sliders */
	MCUREG_3D_SLIDER_POSITION           = 0x8,
	MCUREG_VOLUME_SLIDER_POSITION       = 0x9,
	
	/* battery */
	MCUREG_BATTERY_PCB_TEMPERATURE      = 0xA,
	MCUREG_BATTERY_PERCENTAGE_INT       = 0xB,
	MCUREG_BATTERY_PERCENTAGE_FRAC      = 0xC,
	MCUREG_BATTERY_VOLTAGE              = 0xD,
	
	/* power status */
	MCUREG_POWER_STATUS                 = 0xF,
	
	/* legacy firmware */
	MCUREG_LEGACY_VERSION_HIGH          = 0xF,
	MCUREG_LEGACY_VERSION_LOW           = 0x10,
	MCUREG_LEGACY_FIRM_UPLOAD           = 0x3B,
	
	/* IRQs */
	MCUREG_RECEIVED_IRQS_0              = 0x10,
	MCUREG_RECEIVED_IRQS_1              = 0x11,
	MCUREG_RECEIVED_IRQS_2              = 0x12,
	MCUREG_RECEIVED_IRQS_3              = 0x13,

	MCUREG_IRQ_MASK_0                   = 0x18,
	MCUREG_IRQ_MASK_1                   = 0x19,
	MCUREG_IRQ_MASK_2                   = 0x1A,
	MCUREG_IRQ_MASK_3                   = 0x1B,
	
	/* power control */
	MCUREG_PWR_CTL                      = 0x20,
	MCUREG_LCD_PWR_CTL                  = 0x22,
	MCUREG_MCU_RESET_CTL                = 0x23,
	MCUREG_FORCE_SHUTDOWN_DELAY         = 0x24,
	
	/* unknown volume? */
	MCUREG_VOLUME_UNK_25                = 0x25,
	
	/* unknown ??? */
	MCUREG_UNK_26                       = 0x26,
	
	/* LEDs */
	MCUREG_LED_BRIGHTNESS_STATE         = 0x28,
	MCUREG_POWER_LED_STATE              = 0x29,
	MCUREG_WLAN_LED_STATE               = 0x2A,
	MCUREG_CAMERA_LED_STATE             = 0x2B,
	MCUREG_3D_LED_STATE                 = 0x2C,
	MCUREG_NOTIFICATION_LED_STATE       = 0x2D,
	MCUREG_NOTIFICATION_LED_CYCLE_STATE = 0x2E,
	
	/* RTC time */
	MCUREG_RTC_TIME_SECOND              = 0x30,
	MCUREG_RTC_TIME_MINUTE              = 0x31,
	MCUREG_RTC_TIME_HOUR                = 0x32,
	MCUREG_RTC_TIME_WEEKDAY             = 0x33,
	MCUREG_RTC_TIME_DAY                 = 0x34,
	MCUREG_RTC_TIME_MONTH               = 0x35,
	MCUREG_RTC_TIME_YEAR                = 0x36,
	MCUREG_RTC_TIME_CORRECTION          = 0x37,
	
	/* RTC alarm */
	MCUREG_RTC_ALARM_MINUTE             = 0x38,
	MCUREG_RTC_ALARM_HOUR               = 0x39,
	MCUREG_RTC_ALARM_DAY                = 0x3A,
	MCUREG_RTC_ALARM_MONTH              = 0x3B,
	MCUREG_RTC_ALARM_YEAR               = 0x3C,
	
	/* Tick Counter */
	MCUREG_TICK_COUNTER_LSB             = 0x3D,
	MCUREG_TICK_COUNTER_MSB             = 0x3E,
	
	/* -ometers */
	MCUREG_OMETER_MODE                  = 0x40,

	/* accelerometer */
	MCUREG_ACCELEROMETER_MANUAL_REGID_R = 0x41,
	MCUREG_ACCELEROMETER_MANUAL_REGID_W = 0x43,
	MCUREG_ACCELEROMETER_MANUAL_IO      = 0x44,
	MCUREG_ACCELEROMETER_OUTPUT_X_LSB   = 0x45,
	MCUREG_ACCELEROMETER_OUTPUT_X_MSB   = 0x46,
	MCUREG_ACCELEROMETER_OUTPUT_Y_LSB   = 0x47,
	MCUREG_ACCELEROMETER_OUTPUT_Y_MSB   = 0x48,
	MCUREG_ACCELEROMETER_OUTPUT_Z_LSB   = 0x49,
	MCUREG_ACCELEROMETER_OUTPUT_Z_MSB   = 0x4A,

	/* software pedometer */
	MCUREG_PEDOMETER_STEPS_LOWBYTE      = 0x4B,
	MCUREG_PEDOMETER_STEPS_MIDDLEBYTE   = 0x4C,
	MCUREG_PEDOMETER_STEPS_HIGHBYTE     = 0x4D,
	MCUREG_PEDOMETER_CNT                = 0x4E,
	MCUREG_PEDOMETER_STEP_DATA          = 0x4F,
	MCUREG_PEDOMETER_WRAP_MINUTE        = 0x50,
	MCUREG_PEDOMETER_WRAP_SECOND        = 0x51,
	
	/* volume calibration */
	MCUREG_VOLUME_CALIBRATION_MIN       = 0x58,
	MCUREG_VOLUME_CALIBRATION_MAX       = 0x59,
	
	/* storage area */
	MCUREG_STORAGE_AREA_OFFSET          = 0x60,
	MCUREG_STORAGE_AREA                 = 0x61,
	
	/* info registes */
	MCUREG_INFO                         = 0x7F,
};

enum MCU_InterruptNotification {
	MCUNOTIF_SHELL_STATE_CHANGE    = 0x200,
	MCUNOTIF_POWER_BUTTON_PRESSED  = 0x202,
	MCUNOTIF_POWER_BUTTON_HELD     = 0x203,
	MCUNOTIF_HOME_BUTTON_PRESSED   = 0x204,
	MCUNOTIF_HOME_BUTTON_RELEASED  = 0x205,
	MCUNOTIF_WLAN_SWITCH_TRIGGERED = 0x206,
	MCUNOTIF_FATAL_HW_ERROR        = 0x20C,
	MCUNOTIF_AC_ADAPTER_REMOVED    = 0x20D,
	MCUNOTIF_AC_ADAPTER_CONNECTED  = 0x20E,
	MCUNOTIF_STARTED_CHARGING      = 0x20F,
	MCUNOTIF_STOPPED_CHARGING      = 0x210,
};

enum {
	MCU_FIRMFLG_WIRELESS_DISABLED      = BIT(0),
	MCU_FIRMFLG_SOFTWARE_CLOSED        = BIT(1),
	MCU_FIRMFLG_POWEROFF_INITIATED     = BIT(2),
	MCU_FIRMFLG_LGY_NATIVE_RESOLUTION  = BIT(3),
	MCU_FIRMFLG_LEGACY_JUMP_PROHIBITED = BIT(4)
};

enum MCU_PowerStatusFlags {
	MCU_PWRSTAT_SHELL_OPEN        = BIT(1),
	MCU_PWRSTAT_ADAPTER_CONNECTED = BIT(3),
	MCU_PWRSTAT_CHARGING          = BIT(4),
	MCU_PWRSTAT_BOTTOM_BL_ON      = BIT(5),
	MCU_PWRSTAT_TOP_BL_ON         = BIT(6),
	MCU_PWRSTAT_LCD_ON            = BIT(7)
};

enum MCU_PowerTrigger {
	MCU_PWR_SHUTDOWN     = BIT(0),
	MCU_PWR_RESET        = BIT(1),
	MCU_PWR_REBOOT       = BIT(2),
	MCU_PWR_LGY_SHUTDOWN = BIT(3),
	MCU_PWR_SLEEP        = BIT(4),
	OLDMCU_BL_OFF        = BIT(4),
	OLDMCU_BL_ON         = BIT(5),
	OLDMCU_LCD_OFF       = BIT(6),
	OLDMCU_LCD_ON        = BIT(7),
};

enum MCU_LCDPowerTrigger {
	MCU_LCDPWR_POWER_OFF     = BIT(0),
	MCU_LCDPWR_POWER_ON      = BIT(1),
	MCU_LCDPWR_BOTTOM_BL_OFF = BIT(2),
	MCU_LCDPWR_BOTTOM_BL_ON  = BIT(3),
	MCU_LCDPWR_TOP_BL_OFF    = BIT(4),
	MCU_LCDPWR_TOP_BL_ON     = BIT(5),
};

enum MCU_OmeterMode {
	MCU_OMETER_ACCELEROMETER_ON = BIT(0),
	MCU_OMETER_PEDOMETER_ON     = BIT(1)
};

enum MCU_PedometerControl {
	/* write */
	MCU_PEDOMETER_CLEAR = BIT(0),
	
	/* read */
	MCU_PEDOMETER_STEPS_FULL = BIT(4)
};

enum MCU_ResetEventFlags {
	MCU_RESETFLG_RTC_TIME_LOST  = BIT(0),
	MCU_RESETFLG_WATCHDOG_RESET = BIT(1),
};

typedef enum MCU_HardwareInfo {
	MCU_HWINFO_PRODUCT      = 0,
	MCU_HWINFO_TS_BOARD     = 1,
	MCU_HWINFO_KMC_DEBUGGER = 2,
	MCU_HWINFO_KMC_CAPTURE  = 4,
	MCU_HWINFO_IS_DEBUGGER  = 5,
	MCU_HWINFO_IS_CAPTURE   = 6,
} MCU_HardwareInfo;

enum {
	EVENT_GPU   = 0,
	EVENT_HID   = 1,
	EVENT_POWER = 2,
};

typedef struct __attribute__((packed)) MCU_RtcAlarm {
	u8 minute;
	u8 hour;
	u8 day;
	u8 month;
	u8 year;
} MCU_RtcAlarm;

typedef struct MCU_RtcData {
	u8 second;
	u8 minute;
	u8 hour;
	u8 weekday;
	u8 monthday;
	u8 month;
	u8 year;
} MCU_RtcData;

typedef struct MCU_RtcCorrectionData {
	s8 offset : 7;
	u8 interval_mode : 1;
} MCU_RtcCorrectionData;

typedef struct MCU_AccelerometerData
{
	s16 x;
	s16 y;
	s16 z;
} MCU_AccelerometerData;

typedef struct MCU_PedometerStepData {
	struct MCU_PedometerTime {
		u8 hour;
		u8 monthday;
		u8 month;
		u8 year; /* since 2000 */
		u8 minute;
		u8 second;
	} pedometer_time;
	u16 stepcounts[7 * 24]; /* step data for every hour in the week */
} MCU_PedometerStepData;

typedef struct MCU_NotifictationLedAnimation {
	u8 delay;
	u8 smoothing;
	u8 loop_delay;
	u8 blink_speed;
} MCU_NotifictationLedAnimation;

typedef struct MCU_NotificationLedData {
	MCU_NotifictationLedAnimation animation;
	u8 red_pattern[32];
	u8 green_pattern[32];
	u8 blue_pattern[32];
} MCU_NotificationLedData;

typedef struct __attribute__((packed)) MCU_StorageArea {
	u8 firm_flags;
	u8 lgy_lcd_data;
	u16 local_friend_code_counter;
	u16 uuid_clock_sequence;
	u8 unused0[2];
	u8 lgy_playtime_data[0xA8];
	u8 tmp_playtime_data[12];
	u8 unused2[12];
} MCU_StorageArea;

typedef struct __attribute__((packed)) MCU_InfoRegs {
	u8 hwinfo;
	u8 pmic_vendor_code;
	u8 battery_vendor_code;
	u8 fuelgauge_ver_major;
	u8 fuelgauge_ver_minor;
	u8 fuelgauge_config_rcomp;
	u8 raw_battery_pcb_temp_adc;
	u8 battery_pcb_temp_flags;
	u8 unk0; /* apparently 0x1 on n3ds, 0x0 or 0x1 on o3ds */
	u8 system_model;
	u8 power_led_mode; /* 0 = blue/off, 1 = red */
	u8 power_led_intensity; /* 0x00 - 0xFF */
	u8 _3d_led_intensity;
	u8 notif_led_red_intensity;
	u8 notif_led_green_intensity;
	u8 notif_led_blue_intensity;
	u8 camera_led_state;
	u8 wifi_led_intensity;
	u8 raw_button_states;
} MCU_InfoRegs;

typedef struct __attribute__((packed)) MCU_PowerLedConfig
{
	u8 mode;
	u32 blink_pattern;
} MCU_PowerLedConfig;

enum {
	NOLOCK = false,
	LOCK = true,
};

/* i2c mcu commands */
Result mcuSetRegisterBits8(u8 regid, u8 mask, u8 data);
Result mcuDisableRegisterBits8(u8 regid, u8 mask);
Result mcuWriteRegisterBuffer8(u8 regid, const void *buf, u32 size);
Result mcuReadRegisterBuffer8(u8 regid, void *buf, u32 size);
Result mcuWriteRegisterBuffer(u8 regid, const void *buf, u32 size);
Result mcuReadRegisterBuffer(u8 regid, void *buf, u32 size);
/* locked i2c mcu commands */
Result mcuSetRegisterBits8_l(u8 regid, u8 mask, u8 data);
Result mcuDisableRegisterBits8_l(u8 regid, u8 mask);
Result mcuWriteRegisterBuffer8_l(u8 regid, const void *buf, u32 size);
Result mcuReadRegisterBuffer8_l(u8 regid, void *buf, u32 size);
Result mcuWriteRegisterBuffer_l(u8 regid, const void *buf, u32 size);
Result mcuReadRegisterBuffer_l(u8 regid, void *buf, u32 size);
/* gpio mcu commands */
Result gpioMcuSetRegPart1(u32 value, u32 mask);
Result gpioMcuSetInterruptMask(u32 value, u32 mask);
Result gpioMcuReadData(u32 *out_value, u32 mask);
Result gpioMcuWriteData(u32 value, u32 mask);
Result gpioMcuBindInterrupt(u32 irq_mask, Handle syncobj, s32 priority);
Result gpioMcuUnbindInterrupt(u32 irq_mask, Handle syncobj);
/* locked i2c mcu commands */
Result gpioMcuSetRegPart1_l(u32 value, u32 mask);
Result gpioMcuSetInterruptMask_l(u32 value, u32 mask);
Result gpioMcuReadData_l(u32 *out_value, u32 mask);
Result gpioMcuWriteData_l(u32 value, u32 mask);
Result gpioMcuBindInterrupt_l(u32 irq_mask, Handle syncobj, s32 priority);
Result gpioMcuUnbindInterrupt_l(u32 irq_mask, Handle syncobj);

/* interrupt notifications */
void mcuHandleInterruptEvents(u32 received_irqs);

/* wrappers for mcu regs */
Result mcuUpdateFirmware(const void *payload, u32 payload_size, bool lock);

Result mcuReadFwVerHigh(u8 *out_value, bool lock);
Result mcuReadFwVerLow(u8 *out_value, bool lock);

Result mcuGetResetEventFlags(u8 *out_value, bool lock);
Result mcuClearResetEventFlag(u8 to_clear, bool lock);

Result mcuSetLcdFlicker(bool top, u8 value, bool lock);
Result mcuGetLcdFlicker(bool top, u8 *out_value, bool lock);

Result mcuRead3dSliderPosition(u8 *out_pos, bool lock);
Result mcuReadVolumeSliderPositiion(u8 *out_pos, bool lock);

Result mcuSetBacklightPowerState(u8 top_bl_on, u8 bottom_bl_on, bool lock);
Result mcuSetPowerState(u8 triggers, bool lock);
Result mcuSetLcdPowerState(u8 state, bool lock);
Result mcuReset(bool lock);
Result mcuSetForceShutdownDelay(u8 value, bool lock);
Result mcuGetForceShutdownDelay(u8 *out_value, bool lock);

Result mcuSetUnkVolume25(u8 value, bool lock);
Result mcuGetUnkVolume25(u8 *out_value, bool lock);

Result mcuReadBatteryPcbTemperature(u8 *out_value, bool lock);
Result mcuReadBatteryPercentageInt(u8 *out_value, bool lock);
Result mcuReadBatteryVoltage(u8 *out_value, bool lock);

Result mcuGetPowerStatus(u8 *out_status, bool lock);

Result mcuGetReceivedIrqs(u32 *out_events, bool lock);
Result mcuSetInterruptMask(u32 enabled_interrupts, bool lock);
Result mcuGetInterruptMask(u32 *out_enabled_interrupts, bool lock);

Result mcuSetLedState(u8 led_regid, u8 state, bool lock);
Result mcuGetLedState(u8 led_regid, u8 *out_state, bool lock);
Result mcuSetPowerLedBlinkPattern(u32 blink_pattern, bool lock);
Result mcuSetNotificationLedData(MCU_NotificationLedData *data, bool lock);
Result mcuSetNotificationLedAnimation(MCU_NotifictationLedAnimation *animation, bool lock);
Result mcuGetNotificationLedCycleState(u8 *out_state, bool lock);

Result mcuSetRtcTime(MCU_RtcData *data, bool lock);
Result mcuGetRtcTime(MCU_RtcData *out_data, bool lock);
Result mcuSetRtcTimeField(u8 field_regid, u8 value, bool lock);
Result mcuGetRtcTimeField(u8 field_regid, u8 *out_value, bool lock);
Result mcuSetRtcTimeCorrection(u8 value, bool lock);
Result mcuGetRtcTimeCorrection(u8 *out_value, bool lock);

Result mcuSetRtcAlarm(MCU_RtcAlarm *alarm, bool lock);
Result mcuGetRtcAlarm(MCU_RtcAlarm *alarm, bool lock);
Result mcuSetRtcAlarmField(u8 field_regid, u8 value, bool lock);
Result mcuGetRtcAlarmField(u8 field_regid, u8 *out_value, bool lock);
Result mcuGetTickCounter(u16 *out_value, bool lock);

Result mcuSetOmeterMode(u8 mode, bool lock);
Result mcuGetOmeterMode(u8 *out_mode, bool lock);
Result mcuSetPedometerEnabled(u8 enabled, bool lock);
Result mcuGetPedometerEnabled(u8 *out_enabled, bool lock);
Result mcuReadPedometerStepCount(u32 *out_count, bool lock);
Result mcuReadPedoemterStepData(MCU_PedometerStepData *out_data, bool lock);
Result mcuClearPedometerStepData(bool lock);
Result mcuStartAccelerometerManualRegRead(u8 hw_regid, bool lock);
Result mcuGetAccelerometerManualRegReadResult(u8 *out_data, bool lock);
Result mcuPerformAccelerometerManualWrite(u8 hw_regid, u8 data, bool lock);
Result mcuPerformAccelerometerManualRead(u8 hw_regid, u8 *out_data, bool lock);
Result mcuAccelerometerManualUpdateReg(u8 hw_regid, u32 to_set, u32 to_clear, bool lock);
Result mcuSetAccelerometerInternalFilterEnabled(u8 enabled, bool lock);
Result mcuGetAccelerometerInternalFilterEnabled(u8 *out_enabled, bool lock);
Result mcuSetAccelerometerScale(u8 scale, bool lock);
Result mcuGetAccelerometerScale(u8 *out_scale, bool lock);
Result mcuReadAccelerometerData(MCU_AccelerometerData *out_data, bool lock);
Result mcuSetPedometerWrapTimeMinute(u8 value, bool lock);
Result mcuGetPedometerWrapTimeMinute(u8 *out_value, bool lock);
Result mcuSetPedometerWrapTimeSecond(u8 value, bool lock);
Result mcuGetPedometerWrapTimeSecond(u8 *out_value, bool lock);

Result mcuSetVolumeCalibration(u8 min, u8 max, bool lock);
Result mcuGetVolumeCalibration(u8 *out_min, u8 *out_max, bool lock);

Result mcuReadStorageArea(u8 offset, void *outbuf, u32 size, bool lock);
Result mcuWriteStorageArea(u8 offset, void *inbuf, u32 size, bool lock);
Result mcuSetFirmFlag(u8 flag, bool set, bool lock);
Result mcuGetFirmFlag(bool *out_is_set, u8 flag, bool lock);

Result mcuReadInfoRegisters(void *outbuf, u32 size, bool lock);

#endif
