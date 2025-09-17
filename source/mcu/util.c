#include <mcu/util.h>
#include <memops.h>
#include <3ds/types.h>

void int_convert_to_hex(char* out, unsigned int n, int newline)
{
	*out++ = '0';
	*out++ = 'x';

	static const char hextable[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	for (int i = sizeof(uint32_t) - 1; i >= 0; --i)
	{
		unsigned char _i = n >> (i * 8);
		out[0] = hextable[_i >> 4];
		out[1] = hextable[_i & 0xF];
		out += 2;
	}

	if (newline)
		*out++ = '\n';
	
	*out = 0;
}

void *memset(void *s, int c, size_t n) {
	_memset(s, c, n);
	return s;
}