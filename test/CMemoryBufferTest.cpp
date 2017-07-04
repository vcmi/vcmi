/*
 * CMemoryBufferTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "../lib/filesystem/CMemoryBuffer.h"

struct CMemoryBufferTest : testing::Test
{
	CMemoryBuffer subject;
};


TEST_F(CMemoryBufferTest, empty)
{
	EXPECT_EQ(subject.tell(), 0);
	EXPECT_EQ(subject.getSize(), 0);
	si32 dummy = 1337;
	auto ret = subject.read((ui8 *)&dummy, sizeof(si32));
	EXPECT_EQ(ret, 0);
	EXPECT_EQ(dummy, 1337);
	EXPECT_EQ(subject.tell(), 0);
}

TEST_F(CMemoryBufferTest, write)
{
	const si32 initial = 1337;

	subject.write((const ui8 *)&initial, sizeof(si32));

	EXPECT_EQ(subject.tell(), 4);
	subject.seek(0);
	EXPECT_EQ(subject.tell(), 0);

	si32 current = 0;
	auto ret = subject.read((ui8 *)&current, sizeof(si32));
	EXPECT_EQ(ret, sizeof(si32));
	EXPECT_EQ(current, initial);
	EXPECT_EQ(subject.tell(), 4);
}
