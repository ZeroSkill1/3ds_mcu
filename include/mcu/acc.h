#ifndef _MCU_ACC_H
#define _MCU_ACC_H

#include <3ds/types.h>

enum ACC_Scale {
	ACC_SCALE_2G       = 0x0, /* -2G to 2G */
	ACC_SCALE_4G       = 0x1, /* -4G to 4G */
	ACC_SCALE_RESERVED = 0x2,
	ACC_SCALE_8G       = 0x3, /* -8G to 8G */
};

enum ACC_RegIds {
	ACCREG_FILTER_CTL = 0x21,
	ACCREG_MISC_CTL   = 0x23,
};

enum {
	ACC_FILTERCTL_ENABLE_INTERNAL_FILTER = BIT(4),
};

enum {
	ACC_MISCCTL_SCALE_SELECT = BIT(4) | BIT(5),
};

#endif