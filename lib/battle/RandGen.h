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
	int seed;
	void srand(int s);
	void srand(int3 pos);
	int rand();
	int rand(int min, int max);
};
