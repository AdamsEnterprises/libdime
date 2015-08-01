/**
 * @file
 * @brief	An implementation of the Fletcher hash algorithim.
 */

#include <dime/core/magma.h>

/**
 * @brief	Computer a 32-bit Fletcher hash for a block of data.
 * @param	buffer	a pointer to the data buffer to be checked.
 * @param	length	the length, in bytes, of the data to be checked.
 * @return	a 32-bit number containing the Fletcher hash of the specified data.
 */
uint32_t hash_fletcher32(const void *buffer, size_t length) {
	size_t input, blocks = length / 2;
	uint32_t a = 0xffff, b = 0xffff;
	const uint16_t *buf = buffer;

	while (blocks) {
		input = blocks > 360 ? 360 : blocks;
		blocks -= input;
		do {
			a += *buf++;
			b += a;
		} while (--input);
		a = (a & 0xffff) + (a >> 16);
		b = (b & 0xffff) + (b >> 16);
	}
	a = (a & 0xffff) + (a >> 16);
	b = (b & 0xffff) + (b >> 16);
	return b << 16 | a;
}
