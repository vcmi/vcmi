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

#define VCMI_CHECK_FIELD_EQUAL_P(field) BOOST_CHECK_EQUAL(actual->field, expected->field)

#define VCMI_CHECK_FIELD_EQUAL(field) BOOST_CHECK_EQUAL(actual.field, expected.field)

#define VCMI_REQUIRE_FIELD_EQUAL(field) BOOST_REQUIRE_EQUAL(actual->field, expected->field)

std::ostream& operator<< (std::ostream& os, const PlayerInfo & value)
{
	os << "PlayerInfo"; 
	return os;
}

//std::ostream& operator<< (std::ostream& os, const std::set<ui8> & value)
//{
//	os << "'Set'"; 
//	return os;
//}

bool operator!=(const PlayerInfo & actual, const PlayerInfo & expected)
{
	VCMI_CHECK_FIELD_EQUAL(canHumanPlay);
	VCMI_CHECK_FIELD_EQUAL(canComputerPlay);
	VCMI_CHECK_FIELD_EQUAL(aiTactic);
	//VCMI_CHECK_FIELD_EQUAL(allowedFactions);
	return false;
}

void MapComparer::compareHeader()
{
	VCMI_CHECK_FIELD_EQUAL_P(name);
	VCMI_CHECK_FIELD_EQUAL_P(description);
	VCMI_CHECK_FIELD_EQUAL_P(difficulty);
	VCMI_CHECK_FIELD_EQUAL_P(levelLimit);

	VCMI_CHECK_FIELD_EQUAL_P(victoryMessage);
	VCMI_CHECK_FIELD_EQUAL_P(defeatMessage);
	VCMI_CHECK_FIELD_EQUAL_P(victoryIconIndex);
	VCMI_CHECK_FIELD_EQUAL_P(defeatIconIndex);
	
	BOOST_CHECK_EQUAL_COLLECTIONS(actual->players.begin(), actual->players.end(), expected->players.begin(), expected->players.end());


	//map size parameters are vital for further checks 
	VCMI_REQUIRE_FIELD_EQUAL(height);
	VCMI_REQUIRE_FIELD_EQUAL(width);
	VCMI_REQUIRE_FIELD_EQUAL(twoLevel);

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
	BOOST_REQUIRE_MESSAGE(expected != nullptr, "Expected map is not defined");

	compareHeader();
	compareOptions();
	compareObjects();
	compareTerrain();
}

void MapComparer::operator() (const std::unique_ptr<CMap>& actual, const std::unique_ptr<CMap>&  expected)
{
	this->actual = actual.get();
	this->expected = expected.get();
	compare();
}
