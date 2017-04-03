#include "Landscape_1.h"
#include "TileUtils.h"

uint32 TileUtils::Hash(uint32 a)
{
	a = (a ^ 61) ^ (a >> 16);
	a = a + (a << 3);
	a = a ^ (a >> 4);
	a = a * 0x27d4eb2d;
	a = a ^ (a >> 15);
	return a;
}