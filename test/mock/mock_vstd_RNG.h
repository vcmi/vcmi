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
	MOCK_METHOD0(nextInt, int());
	MOCK_METHOD1(nextInt, int(int));
	MOCK_METHOD2(nextInt, int(int, int));
	MOCK_METHOD0(nextDouble, double());
	MOCK_METHOD1(nextDouble, double(double));
	MOCK_METHOD2(nextDouble, double(double, double));
	MOCK_METHOD2(getIntRange, TRandI(int, int));
	MOCK_METHOD2(getInt64Range, TRandI64(int64_t, int64_t));
	MOCK_METHOD2(getDoubleRange, TRand(double, double));
};

}
