/*
 * mock_vstd_RNG.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vstd/RNG.h>

namespace vstd
{

class RNGMock : public RNG
{
public:
	MOCK_METHOD2(nextInt, int(int lower, int upper));
	MOCK_METHOD2(nextInt64, int64_t(int64_t lower, int64_t upper));
	MOCK_METHOD2(nextDouble, double(double lower, double upper));

	MOCK_METHOD1(nextInt, int(int upper));
	MOCK_METHOD1(nextInt64, int64_t(int64_t upper));
	MOCK_METHOD1(nextDouble, double(double upper));

	MOCK_METHOD0(nextInt, int());
	MOCK_METHOD2(nextBinomialInt, int(int coinsCount, double coinChance));
};

}
