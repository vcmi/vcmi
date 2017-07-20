/*
 * TimedTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "EffectFixture.h"

#include <vstd/RNG.h>

namespace test
{
using namespace ::spells;
using namespace ::spells::effects;
using namespace ::testing;

class TimedTest : public Test, public EffectFixture
{
public:
	TimedTest()
		: EffectFixture("core:timed")
	{
	}

protected:
	void SetUp() override
	{
		EffectFixture::setUp();
	}
};


}
