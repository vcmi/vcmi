/*
 * RandGen.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "../int3.h"
//RNG that works like H3 one
struct RandGen
{
	int64_t seed;
	void srand(int64_t s);
	void srand(int3 pos);
	int64_t rand();
	int64_t rand(int min, int max);
};
