/*
 * MapComparer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include <boost/test/unit_test.hpp>

#include "MapComparer.h"

#define VCMI_CHECK_FIELD_EQUAL(field) BOOST_CHECK_EQUAL(actual->field, expected->field)

std::ostream& operator<< (std::ostream& os, const PlayerInfo & p)
{
	os << "PlayerInfo"; 
	return os;
}

bool operator!=(const PlayerInfo & actual, const PlayerInfo & expected)
{
	
	
	return true;
}

void MapComparer::compareHeader()
{
	VCMI_CHECK_FIELD_EQUAL(name);
	VCMI_CHECK_FIELD_EQUAL(description);
	VCMI_CHECK_FIELD_EQUAL(difficulty);
	VCMI_CHECK_FIELD_EQUAL(levelLimit);

	VCMI_CHECK_FIELD_EQUAL(victoryMessage);
	VCMI_CHECK_FIELD_EQUAL(defeatMessage);
	VCMI_CHECK_FIELD_EQUAL(victoryIconIndex);
	VCMI_CHECK_FIELD_EQUAL(defeatIconIndex);
	
	BOOST_CHECK_EQUAL_COLLECTIONS(actual->players.begin(), actual->players.end(), expected->players.begin(), expected->players.end());


	//map size parameters are vital for further checks 
	BOOST_REQUIRE_EQUAL(actual->height, expected->height);
	BOOST_REQUIRE_EQUAL(actual->width, expected->width);
	BOOST_REQUIRE_EQUAL(actual->twoLevel, expected->twoLevel);

	BOOST_FAIL("Not implemented");
}

void MapComparer::compareOptions()
{
	BOOST_FAIL("Not implemented");
}

void MapComparer::compareObjects()
{
	BOOST_FAIL("Not implemented");
}

void MapComparer::compareTerrain()
{
	BOOST_FAIL("Not implemented");
}

void MapComparer::compare()
{
	BOOST_REQUIRE_NE((void *) actual, (void *) expected); //should not point to the same object
	BOOST_REQUIRE_MESSAGE(actual != nullptr, "Actual map is not defined");
	BOOST_REQUIRE_MESSAGE(actual != nullptr, "Expected map is not defined");

	compareHeader();
	compareObjects();
	compareOptions();
	compareTerrain();
}

void MapComparer::operator() (const std::unique_ptr<CMap>& actual, const std::unique_ptr<CMap>&  expected)
{
	this->actual = actual.get();
	this->expected = expected.get();
	compare();
}
