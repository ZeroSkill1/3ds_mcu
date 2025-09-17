#ifndef _MCU_3DS_OS_H
#define _MCU_3DS_OS_H

enum PrevFirmType {
	PREV_COLD_BOOT      = 0,
	PREV_CTR_MODE_RESET = 1,
	PREV_TWL_MODE_RESET = 3,
	PREV_NTR_MODE_RESET = 5,
	PREV_AGB_MODE_RESET = 7,
};

#endif