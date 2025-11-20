#ifndef MCU_UTIL_H
#define MCU_UTIL_H

#define INT2BCD(x) ((x) + 6u * (((x) * 103u)>>10))
#define BCD2INT(x) ((x) - 6u * ((x)>>4))
#define CHECKBIT(x,y) (((x) & (y)) == (y))

#define LODWORD(x) ((u32)(x & 0xFFFFFFFF))
#define HIDWORD(x) ((u32)((x >> 32) & 0xFFFFFFFF))

#endif
