#ifndef _MCU_UTIL_H
#define _MCU_UTIL_H

#define INT2BCD(x) ((((x) / 10) << 4) | ((x) % 10))
#define BCD2INT(x) ((((x) >> 4) * 10) | ((x) & 0xF))
#define CHECKBIT(x,y) (((x) & (y)) == (y))

#ifdef DEBUG_PRINTS
	void int_convert_to_hex(char* out, unsigned int n, int newline);
	#define DEBUG_PRINT(str) svcOutputDebugString(str, sizeof(str) - 1)
	#define DEBUG_PRINTF(str, num) \
		{ \
			char __ibuf[12] = { 0 }; \
			\
			svcOutputDebugString(str, sizeof(str) - 1); \
			int_convert_to_hex(__ibuf, num, true); \
			svcOutputDebugString(__ibuf, 11); \
		}
	#define DEBUG_PRINTF2(str, num, str2, num2) \
		{ \
			char __ibuf[12] = { 0 }; \
			\
			svcOutputDebugString(str, sizeof(str) - 1); \
			int_convert_to_hex(__ibuf, num, false); \
			svcOutputDebugString(__ibuf, 10); \
			\
			svcOutputDebugString(str2, sizeof(str2) - 1); \
			int_convert_to_hex(__ibuf, num2, true); \
			svcOutputDebugString(__ibuf, 11); \
		}
	#define DEBUG_PRINTF3(str, num, str2, num2, str3, num3) \
		{ \
			char __ibuf[12] = { 0 }; \
			\
			svcOutputDebugString(str, sizeof(str) - 1); \
			int_convert_to_hex(__ibuf, num, false); \
			svcOutputDebugString(__ibuf, 10); \
			\
			svcOutputDebugString(str2, sizeof(str2) - 1); \
			int_convert_to_hex(__ibuf, num2, false); \
			svcOutputDebugString(__ibuf, 10); \
			\
			svcOutputDebugString(str3, sizeof(str3) - 1); \
			int_convert_to_hex(__ibuf, num3, true); \
			svcOutputDebugString(__ibuf, 11); \
		}

#else
	#define DEBUG_PRINT(str) {}
	#define DEBUG_PRINTF(str, num) {}
	#define DEBUG_PRINTF2(str, num, str2, num2) {}
	#define DEBUG_PRINTF3(str, num, str2, num2, str3, num3) {}
#endif

#endif