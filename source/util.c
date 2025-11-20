#include <stddef.h>
#include <stdint.h>
#include <util.h>
#include <memops.h>

void *memset(void *s, int c, size_t n) {
	_memset(s, c, n);
	return s;
}