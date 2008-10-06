#include "private/ff_common.h"

#include "private/ff_hash.h"

#define rot(x,k) (((x)<<(k)) | ((x)>>(32-(k))))

#define mix(a,b,c) \
{ \
  a -= c;  a ^= rot(c, 4);  c += b; \
  b -= a;  b ^= rot(a, 6);  a += c; \
  c -= b;  c ^= rot(b, 8);  b += a; \
  a -= c;  a ^= rot(c,16);  c += b; \
  b -= a;  b ^= rot(a,19);  a += c; \
  c -= b;  c ^= rot(b, 4);  b += a; \
}

#define final(a,b,c) \
{ \
  c ^= b; c -= rot(b,14); \
  a ^= c; a -= rot(c,11); \
  b ^= a; b -= rot(a,25); \
  c ^= b; c -= rot(b,16); \
  a ^= c; a -= rot(c,4);  \
  b ^= a; b -= rot(a,14); \
  c ^= b; c -= rot(b,24); \
}

/**
 * Calculates Bob Jenkins' hash for the given buf with the given buf_size size and the given start_value.
 * The function is based on the research http://www.burtleburtle.net/bob/hash/doobs.html
 * It is probably the fastest hash function for word elements
 * and gives distribution close to uniform for typical input.
 * See http://burtleburtle.net/bob/c/lookup3.c for reference implementation.
 */
uint32_t ff_hash_uint32(uint32_t start_value, const uint32_t *buf, int buf_size)
{
	uint32_t a, b, c;

	ff_assert(buf_size >= 0);

	a = b = c = 0xdeadbeef + (((uint32_t) buf_size) << 2) + start_value;
	while (buf_size > 3)
	{
		a += buf[0];
		b += buf[1];
		c += buf[2];
		mix(a, b, c);
		buf_size -= 3;
		buf += 3;
	}

	switch (buf_size)
	{ 
		case 3 : c += buf[2];
		case 2 : b += buf[1];
		case 1 : a += buf[0];
			final(a, b, c);
		case 0:
			break;
	}

	return c;
}

uint32_t ff_hash_uint16(uint32_t start_value, const uint16_t *buf, int buf_size)
{
	int uint32_chunks_cnt;
	uint32_t hash;
	int tail_size;

	ff_assert(buf_size >= 0);
	uint32_chunks_cnt = buf_size >> 1;
	hash = ff_hash_uint32(start_value, (const uint32_t *) buf, uint32_chunks_cnt);
	tail_size = buf_size & 0x01;
	if (tail_size == 1)
	{
		uint32_t tail_uint32;
		const uint16_t *tail;

		tail = buf + (uint32_chunks_cnt << 1);
		tail_uint32 = (uint32_t) tail[0];
		hash = ff_hash_uint32(hash, &tail_uint32, 1);
	}
	return hash;
}

uint32_t ff_hash_uint8(uint32_t start_value, const uint8_t *buf, int buf_size)
{
	int uint32_chunks_cnt;
	uint32_t hash;
	int tail_size;

	ff_assert(buf_size >= 0);

	uint32_chunks_cnt = buf_size >> 2;
	hash = ff_hash_uint32(start_value, (const uint32_t *) buf, uint32_chunks_cnt);
	tail_size = buf_size & 0x03;
	if (tail_size > 0)
	{
		uint32_t tail_uint32;
		const uint8_t *tail;

		tail = buf + (uint32_chunks_cnt << 2);
		switch (tail_size)
		{
			case 3:
				tail_uint32 = (((uint32_t) tail[0]) << 16) | (((uint32_t) tail[1]) << 8) | ((uint32_t) tail[2]);
				break;
			case 2:
				tail_uint32 = (((uint32_t) tail[0]) << 8) | ((uint32_t) tail[1]);
			case 1:
				tail_uint32 = (uint32_t) tail[0];
		}
		hash = ff_hash_uint32(hash, &tail_uint32, 1);
	}
	return hash;
}
