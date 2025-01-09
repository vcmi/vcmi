/*
* FakeRandomGenerator.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include <vstd/RNG.h>

namespace NKAI
{

class FakeRandomGenerator final : public vstd::RNG
{
	int nextInt(int upper) override { return upper/2;}
	int64_t nextInt64(int64_t upper) override { return upper/2;}
	int nextInt(int lower, int upper) override { return (lower + upper) / 2;}
	int64_t nextInt64(int64_t lower, int64_t upper) override { return (lower + upper) / 2;}
	int nextInt() override { return 1234567890;}
	int nextBinomialInt(int coinsCount, double coinChance) override { return coinsCount * coinChance;}
	double nextDouble(double upper) override { return upper/2;}
	double nextDouble(double lower, double upper) override { return (lower + upper) / 2;}
};

}
