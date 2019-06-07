/*
 * ERM_MA.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../scripting/ScriptFixture.h"
#include "../JsonComparer.h"

#include "../../lib/NetPacks.h"

namespace test
{
namespace scripting
{

using namespace ::testing;

class ERM_MA : public Test, public ScriptFixture
{
protected:
	void SetUp() override
	{
		ScriptFixture::setUp();
	}
};

TEST_F(ERM_MA, Example)
{
	std::stringstream source;
	source << "VERM" << std::endl;
	source << "!#MA:C68/6/750 A68/17 D68/15 P68/50 S68/9 M68/12 E68/20 N68/0 G68/1 B68/0 R68/0 I68/3388 F68/2420 L68/6 O68/4 X68/394371;" << std::endl;

	loadScript(VLC->scriptHandler->erm, source.str());
	SCOPED_TRACE("\n" + subject->code);
	run();


}


}
}
