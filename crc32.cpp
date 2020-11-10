#include "crc32.h"

DWORD crc32(byte* data, int size, DWORD crc)
{
	DWORD r = crc;
	byte* end = data + size;
	DWORD t;

	while (data < end)
	{
		r ^= *data++;

		for (int i = 0; i < 8; i++)
		{
			t = ~((r & 1) - 1);
			r = (r >> 1) ^ (0xEDB88320 & t);
		}
	}

	return ~r;
}