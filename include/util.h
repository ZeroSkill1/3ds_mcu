#ifndef MCU_UTIL_H
#define MCU_UTIL_H

#define INT2BCD(x) ((((x) / 10) << 4) | ((x) % 10))
#define BCD2INT(x) ((((x) >> 4) * 10) | ((x) & 0xF))
#define CHECKBIT(x,y) (((x) & (y)) == (y))

#define LODWORD(x) ((u32)(x & 0xFFFFFFFF))
#define HIDWORD(x) ((u32)((x >> 32) & 0xFFFFFFFF))

#endif