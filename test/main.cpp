/*
 * main.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CVcmiTestConfig.h"
#include "../lib/filesystem/CMemoryBuffer.h"

int main(int argc, char * argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	CVcmiTestConfig test;
	return RUN_ALL_TESTS();
}
