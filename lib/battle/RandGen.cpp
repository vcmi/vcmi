/*
 * RandGen.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "RandGen.h"

void RandGen::srand(int64_t s)
{
	seed = s;
}

void RandGen::srand(int3 pos)
{
	srand(110291 * pos.x + 167801 * pos.y + 81569);
}

int64_t RandGen::rand()
{
	seed = 214013 * seed + 2531011;
	return (seed >> 16) & 0x7FFF;
}

int64_t RandGen::rand(int min, int max)
{
	if(min == max)
		return min;
	if(min > max)
		return min;
	return min + rand() % (max - min + 1);
}
