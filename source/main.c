#include <3ds/synchronization.h>
#include <mcu/globals.h>
#include <3ds/result.h>
#include <3ds/types.h>
#include <3ds/gpio.h>
#include <mcu/util.h>
#include <3ds/svc.h>
#include <3ds/i2c.h>
#include <3ds/srv.h>
#include <mcu/ipc.h>
#include <mcu/mcu.h>
#include <3ds/os.h>
#include <memops.h>
#include <stdint.h>

#ifdef ENABLE_FIRM_UPLOAD
const char mcu_firm[] = {
	0x6A, 0x68, 0x6C,
	#embed "mcu_firm.bin"
};
#endif

#define countof(arr) (sizeof(arr) / sizeof(arr[0]))

// service constants

#define MCU_SERVICE_COUNT            9
#define MCU_MAX_SESSIONS_PER_SERVICE 1
#define MCU_MAX_TOTAL_SESSIONS       9

static const struct
{
	const char *name;
	void (* pre_serve)();
	void (* post_serve)();
	u8 len;
} MCU_ServiceConfigs[MCU_SERVICE_COUNT] =
{
	{ .name = "mcu::CAM", .pre_serve = NULL           , .post_serve = NULL            , .len = sizeof("mcu::CAM") - 1 },
	{ .name = "mcu::GPU", .pre_serve = MCUGPU_PreServe, .post_serve = MCUGPU_PostServe, .len = sizeof("mcu::GPU") - 1 },
	{ .name = "mcu::HID", .pre_serve = MCUHID_PreServe, .post_serve = MCUHID_PostServe, .len = sizeof("mcu::HID") - 1 },
	{ .name = "mcu::RTC", .pre_serve = MCURTC_PreServe, .post_serve = NULL            , .len = sizeof("mcu::RTC") - 1 },
	{ .name = "mcu::SND", .pre_serve = NULL           , .post_serve = NULL            , .len = sizeof("mcu::SND") - 1 },
	{ .name = "mcu::NWM", .pre_serve = NULL           , .post_serve = NULL            , .len = sizeof("mcu::NWM") - 1 },
	{ .name = "mcu::HWC", .pre_serve = NULL           , .post_serve = NULL            , .len = sizeof("mcu::HWC") - 1 },
	{ .name = "mcu::PLS", .pre_serve = NULL           , .post_serve = NULL            , .len = sizeof("mcu::PLS") - 1 },
	{ .name = "mcu::CDC", .pre_serve = NULL           , .post_serve = NULL            , .len = sizeof("mcu::CDC") - 1 }
};

// 9 ipc server threads and irq handler thread
#ifdef DEBUG
// increase stack size to account for unoptimized code
__attribute__((section(".data.thread_stacks"), aligned(8))) static u8 MCU_ThreadStacks[MCU_MAX_TOTAL_SESSIONS + 1][0x500] = { 0 };
#else
__attribute__((section(".data.thread_stacks"), aligned(8))) static u8 MCU_ThreadStacks[MCU_MAX_TOTAL_SESSIONS + 1][0x400] = { 0 };
#endif
__attribute__((section(".data.irqh_exit_flag"))) bool g_IrqHandlerThreadExitFlag;
__attribute__((section(".data.session_data"))) static MCU_SessionData MCU_SessionsData[MCU_MAX_TOTAL_SESSIONS] = { 0 };

void _thread_start(void *);

Result startThread(Handle *thread, void (* function)(void *), void *arg, void *stack_top, s32 priority, s32 processor_id)
{
	if ((u32)(stack_top) & (0x8 - 1))
		return OS_MISALIGNED_ADDRESS;
	//_thread_start will pop these out
	((u32 *)stack_top)[-1] = (u32)function;
	((u32 *)stack_top)[-2] = (u32)arg;

	return svcCreateThread(thread, _thread_start, function, stack_top, priority, processor_id);
}

static inline void freeThread(Handle *thread)
{
	if (thread && *thread)
	{
		T(svcWaitSynchronization(*thread, -1))
		T(svcCloseHandle(*thread));
		*thread = 0;
	}
}

extern void (* MCU_IPCHandlers[9])();

static inline MCU_SessionData *getNewSessionData(s32 service_index)
{
	MCU_SessionData *data = &MCU_SessionsData[service_index]; /* service_index = id */
	
	if (data->thread) {
		freeThread(&data->thread);
		_memset32_aligned(data, 0, sizeof(MCU_SessionData));
	}

	data->handle_ipc = MCU_IPCHandlers[service_index];
	data->pre_serve = MCU_ServiceConfigs[service_index].pre_serve;
	data->post_serve = MCU_ServiceConfigs[service_index].post_serve;
	
	return data;
}

void MCU_SessionThreadMain(void *arg)
{
	MCU_SessionData *data = (MCU_SessionData *)arg;
	Result res;
	s32 index;
	
	if (data->pre_serve)
		data->pre_serve();

	getThreadCommandBuffer()[0] = 0xFFFF0000;

	while (true)
	{
		res = svcReplyAndReceive(&index, &data->session, 1, data->session);

		if (R_FAILED(res))
		{
			if (res != OS_REMOTE_SESSION_CLOSED)
				Err_Panic(res);

			break;
		}
		else if (index != 0)
			Err_Panic(OS_EXCEEDED_HANDLES_INDEX);

		data->handle_ipc();
	}

	T(svcCloseHandle(data->session))
	
	if (data->post_serve)
		data->post_serve();
}

static inline void initializeBSS()
{
	extern void *__bss_start__;
	extern void *__bss_end__;

	_memset32_aligned(__bss_start__, 0, (size_t)__bss_end__ - (size_t)__bss_end__);
}

#define SRV_NOTIF_REPLY(idx) (idx == 0) // handles[0]
#define SERVICE_REPLY(idx) (idx > 0 && idx < MCU_SERVICE_COUNT + 1) // handles[1] until handles[9]

void MCU_IRQHandlerMain(void *arg) {
	(void)arg;
	
	while (1) {
		T(svcWaitSynchronization(g_GPIO_MCUInterruptEvent, -1));
		
		RecursiveLock_Lock(&g_ExclusiveIRQLock);
		
		if (g_IrqHandlerThreadExitFlag)
			break;
		
		u32 received_irqs = 0;
		T(mcuGetReceivedIrqs(&received_irqs, LOCK));
		
		mcuHandleInterruptEvents(received_irqs);
		
		RecursiveLock_Unlock(&g_ExclusiveIRQLock);
	}
}

const vu8 *const CFG_PREV_FIRM = (vu8 *const)0x1FF80016;

#define DEFAULT_ENABLED_IRQS MCUINT_POWER_BUTTON_PRESS | MCUINT_POWER_BUTTON_HELD | \
                             MCUINT_HOME_BUTTON_PRESS | MCUINT_HOME_BUTTON_RELEASE | \
                             MCUINT_WLAN_SWITCH_TRIGGER | MCUINT_SHELL_CLOSE | \
                             MCUINT_SHELL_OPEN | MCUINT_FATAL_HW_ERROR | \
                             MCUINT_AC_ADAPTER_REMOVED | MCUINT_AC_ADAPTER_PLUGGED_IN | \
                             MCUINT_CHARGING_STOP | MCUINT_CHARGING_START

void MCU_Main()
{
	initializeBSS();

	T(srvInit());
	T(syncInit());
	T(i2cMcuInit());
	T(gpioMcuInit());

	/*
		handles[0]  = SRV notification handle
		handles[1]  = mcu::CAM server handle
		handles[2]  = mcu::GPU server handle
		handles[3]  = mcu::HID server handle
		handles[4]  = mcu::RTC server handle
		handles[5]  = mcu::SND server handle
		handles[6]  = mcu::NWM server handle
		handles[7]  = mcu::HWC server handle
		handles[8]  = mcu::PLS server handle
		handles[9]  = mcu::CDC server handle
		handles[10] = IRQ handler thread
	*/
	Handle handles[MCU_SERVICE_COUNT + 2];
	
	// globals init
	RecursiveLock_Init(&g_I2CLock);
	RecursiveLock_Init(&g_GPIOLock);
	RecursiveLock_Init(&g_ExclusiveIRQLock);
	
	LightEvent_Init(&g_AccelerometerManualI2CEvent, false);
	
	// handles[0] - srv notification event
	T(SRV_EnableNotification(&handles[0]));

	// handles[1] through handles[9] - services
	for (u8 i = 0, j = 1; i < MCU_SERVICE_COUNT; i++, j++)
		T(SRV_RegisterService(&handles[j], MCU_ServiceConfigs[i].name, MCU_ServiceConfigs[i].len, MCU_MAX_SESSIONS_PER_SERVICE));
	
	T(svcCreateEvent(&g_GPIO_MCUInterruptEvent, RESET_ONESHOT));
	
	T(svcCreateEvent(&g_IRQEvents[EVENT_GPU], RESET_ONESHOT));
	T(svcCreateEvent(&g_IRQEvents[EVENT_HID], RESET_ONESHOT));
	T(svcCreateEvent(&g_IRQEvents[EVENT_POWER], RESET_ONESHOT));
	
	g_McuFirmWasUpdated = false;
	g_IrqHandlerThreadExitFlag = false;
	
	// some setup
	
	// enable GPIO<--->MCU interrupt
	T(gpioMcuSetInterruptMask_l(GPIO_MCU_INTERRUPT, GPIO_MCU_INTERRUPT));
	
	{
		MCU_RtcAlarm alarm = { 0, 0, 0, 0, 0 };
		T(mcuSetRtcAlarm(&alarm, LOCK));
	}
	T(gpioMcuBindInterrupt_l(GPIO_MCU_INTERRUPT, g_GPIO_MCUInterruptEvent, 8));
	
	u32 atboot_irqs = 0;
	bool poweroff_flag = false;
	bool signal_poweroff = false;
	
	T(mcuGetReceivedIrqs(&atboot_irqs, LOCK));
	
	I2C_LOCKED({
		T(mcuGetFirmFlag(&poweroff_flag, MCU_FIRMFLG_POWEROFF_INITIATED, NOLOCK));
		T(mcuSetFirmFlag(MCU_FIRMFLG_POWEROFF_INITIATED, false, NOLOCK));
	})
	
	/* if there is already a poweroff pending, or if not, if the power button is held */
	if (poweroff_flag || (atboot_irqs & MCUINT_POWER_BUTTON_HELD))
	{
		/* then if the power button was actually held, or we ended up here because of a reset (indicated by PREV_FIRM), initiate a poweroff */
		if ((atboot_irqs & MCUINT_POWER_BUTTON_HELD) || *CFG_PREV_FIRM == PREV_CTR_MODE_RESET)
			signal_poweroff = true;
	}
	
	// handles[10] - irq handler thread
	T(startThread(&handles[10], &MCU_IRQHandlerMain, NULL, &MCU_ThreadStacks[10], 11, -2));
	
	// set default interrupt mask
	T(mcuSetInterruptMask(DEFAULT_ENABLED_IRQS, LOCK));

#ifdef ENABLE_FIRM_UPLOAD
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wcpp"
#warning Building with firmware upgrade support. Please be careful!
#pragma GCC diagnostic pop
	u8 ver_high = 0, ver_low = 0;
	
	I2C_LOCKED({
		T(mcuReadFwVerHigh(&ver_high, NOLOCK));
		T(mcuReadFwVerLow(&ver_low, NOLOCK));
	})
	
	if (ver_high < MCU_FIRM_VER_HIGH || (ver_high == MCU_FIRM_VER_HIGH && ver_low < MCU_FIRM_VER_LOW))
	{
		u8 wlan_led_state = 0;
		u32 wlan_mode = 0;
		u32 wlan_state = 0;
		bool had_wireless_disabled = false;
		
		/* read some state to restore after the firmware upgrade */
		
		T(mcuGetLedState(MCUREG_WLAN_LED_STATE, &wlan_led_state, LOCK));
		T(gpioMcuReadData_l(&wlan_mode, GPIO_WLAN_MODE));
		GPIO_LOCKED({
			gpioMcuSetRegPart1(0, GPIO_WLAN_STATE); // not needed? will error
			T(gpioMcuReadData(&wlan_state, GPIO_WLAN_STATE));
		});
		T(mcuGetFirmFlag(&had_wireless_disabled, MCU_FIRMFLG_WIRELESS_DISABLED, LOCK));
		
		T(mcuUpdateFirmware(mcu_firm, sizeof mcu_firm, LOCK));
		g_McuFirmWasUpdated = true;
		
		T(mcuSetLedState(MCUREG_WLAN_LED_STATE, wlan_led_state, LOCK));
		T(gpioMcuWriteData_l(wlan_mode, GPIO_WLAN_MODE));
		GPIO_LOCKED({
			gpioMcuSetRegPart1(GPIO_WLAN_STATE, GPIO_WLAN_STATE); // not needed? will error
			T(gpioMcuWriteData(wlan_state, GPIO_WLAN_STATE));
		});
		
		T(mcuSetFirmFlag(MCU_FIRMFLG_WIRELESS_DISABLED, had_wireless_disabled, LOCK));
	}
#endif

	if (signal_poweroff)
		mcuHandleInterruptEvents(MCUINT_POWER_BUTTON_HELD);
	
	u8 battery_perc = 0;
	T(mcuReadBatteryPercentageInt(&battery_perc, LOCK));
	
	if (battery_perc == 0)
		mcuHandleInterruptEvents(MCUINT_CRITICAL_BATTERY);
	
	while (true)
	{
		s32 index;

		// (num_handles - 1) because we don't want to wait for irq thread to join here
		Result res = svcWaitSynchronizationN(&index, handles, countof(handles) - 1, false, -1);

		if (R_FAILED(res))
			Err_Throw(res);

		if (SRV_NOTIF_REPLY(index)) // SRV event fired for notification
		{
			u32 notification_id = 0;
			T(SRV_ReceiveNotification(&notification_id))
			if (notification_id == 0x100) // terminate
				break;
		}
		else if (SERVICE_REPLY(index)) // service handle received request to create session
		{
			Handle session, thread;
			
			/* we only allow one active thread per service */
			MCU_SessionData *data = getNewSessionData(index - 1);

			T(svcAcceptSession(&session, handles[index]));
			T(startThread(&thread, &MCU_SessionThreadMain, data, MCU_ThreadStacks[index], 20, -2));

			data->session = session;
			data->thread = thread;
		}
		else // invalid index
			Err_Throw(MCU_INTERNAL_RANGE);
	}

	// wait and close thread handles
	for (u8 i = 0; i < MCU_MAX_TOTAL_SESSIONS + 1; i++)
		freeThread(&MCU_SessionsData[i].thread);
	
	g_IrqHandlerThreadExitFlag = true;
	
	// wait and close irq handler thread
	freeThread(&handles[10]);

	T(svcCloseHandle(handles[0]));

	// unregister services
	for (u8 i = 0, j = 1; i < MCU_SERVICE_COUNT; i++, j++)
	{
		T(SRV_UnregisterService(MCU_ServiceConfigs[i].name, MCU_ServiceConfigs[i].len));
		T(svcCloseHandle(handles[j]));
	}
	
	T(svcCloseHandle(g_GPIO_MCUInterruptEvent));
	T(svcCloseHandle(g_IRQEvents[EVENT_GPU]));
	T(svcCloseHandle(g_IRQEvents[EVENT_HID]));
	T(svcCloseHandle(g_IRQEvents[EVENT_POWER]));

	gpioMcuExit();
	i2cMcuExit();
	srvExit();
	syncExit();
}
