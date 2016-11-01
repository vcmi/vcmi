
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

#include <boost/test/unit_test.hpp>

#include "../lib/filesystem/CMemoryBuffer.h"


struct CMemoryBufferFixture
{
	CMemoryBuffer subject;
};

BOOST_FIXTURE_TEST_CASE(CMemoryBuffer_Empty, CMemoryBufferFixture)
{
	BOOST_REQUIRE_EQUAL(0, subject.tell());
	BOOST_REQUIRE_EQUAL(0, subject.getSize());
	
	si32 dummy = 1337;
	
	auto ret = subject.read((ui8 *)&dummy, sizeof(si32));
	
	BOOST_CHECK_EQUAL(0, ret);	
	BOOST_CHECK_EQUAL(1337, dummy);
	BOOST_CHECK_EQUAL(0, subject.tell());
}

BOOST_FIXTURE_TEST_CASE(CMemoryBuffer_Write, CMemoryBufferFixture)
{
	const si32 initial = 1337;
	
	subject.write((const ui8 *)&initial, sizeof(si32));
	
	BOOST_CHECK_EQUAL(4, subject.tell());
	subject.seek(0);
	BOOST_CHECK_EQUAL(0, subject.tell());
	
	si32 current = 0;
	auto ret = subject.read((ui8 *)&current, sizeof(si32));
	BOOST_CHECK_EQUAL(sizeof(si32), ret);	
	BOOST_CHECK_EQUAL(initial, current);
	BOOST_CHECK_EQUAL(4, subject.tell());	
}
