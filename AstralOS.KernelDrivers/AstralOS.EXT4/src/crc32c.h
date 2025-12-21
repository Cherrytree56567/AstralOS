#include <stdint.h>
#include <stddef.h>

void crc32c_init_sw(void);
uint32_t crc32c_sw(uint32_t crci, const void *buf, size_t len);
