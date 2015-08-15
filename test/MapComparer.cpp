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

#define VCMI_REQUIRE_FIELD_EQUAL_P(field) BOOST_REQUIRE_EQUAL(actual->field, expected->field)
#define VCMI_REQUIRE_FIELD_EQUAL(field) BOOST_REQUIRE_EQUAL(actual.field, expected.field)

template <class T>
void checkEqual(const T & actual, const T & expected)
{
	BOOST_CHECK_EQUAL(actual, expected)	;
}

template <class Element>
void checkEqual(const std::vector<Element> & actual, const std::vector<Element> & expected)
{
	BOOST_CHECK_EQUAL(actual.size(), expected.size());
	
	for(auto actualIt = actual.begin(), expectedIt = expected.begin(); actualIt != actual.end() && expectedIt != expected.end(); actualIt++, expectedIt++)
	{
		checkEqual(*actualIt, *expectedIt);
	}		
}

template <class Element>
void checkEqual(const std::set<Element> & actual, const std::set<Element> & expected)
{
	BOOST_CHECK_EQUAL(actual.size(), expected.size());
	
	for(auto elem : expected)
	{
		if(!vstd::contains(actual, elem))
			BOOST_ERROR("Required element not found "+boost::to_string(elem));
	}		
}

void checkEqual(const PlayerInfo & actual, const PlayerInfo & expected)
{
	VCMI_CHECK_FIELD_EQUAL(canHumanPlay);
	VCMI_CHECK_FIELD_EQUAL(canComputerPlay);
	VCMI_CHECK_FIELD_EQUAL(aiTactic);
	
	checkEqual(actual.allowedFactions, expected.allowedFactions);
	
	VCMI_CHECK_FIELD_EQUAL(isFactionRandom);
	VCMI_CHECK_FIELD_EQUAL(mainCustomHeroPortrait);	
	VCMI_CHECK_FIELD_EQUAL(mainCustomHeroName);
	
	VCMI_CHECK_FIELD_EQUAL(mainCustomHeroId);
	
	//todo:heroesNames
	
	VCMI_CHECK_FIELD_EQUAL(hasMainTown);
	VCMI_CHECK_FIELD_EQUAL(generateHeroAtMainTown);
	VCMI_CHECK_FIELD_EQUAL(posOfMainTown);
	VCMI_CHECK_FIELD_EQUAL(team);
	VCMI_CHECK_FIELD_EQUAL(hasRandomHero);

}

void checkEqual(const EventExpression & actual,  const EventExpression & expected)
{
	//todo: checkEventExpression
}

void checkEqual(const TriggeredEvent & actual,  const TriggeredEvent & expected)
{
	VCMI_CHECK_FIELD_EQUAL(identifier);
	VCMI_CHECK_FIELD_EQUAL(description);
	VCMI_CHECK_FIELD_EQUAL(onFulfill);
	VCMI_CHECK_FIELD_EQUAL(effect.type);
	VCMI_CHECK_FIELD_EQUAL(effect.toOtherMessage);	

	checkEqual(actual.trigger, expected.trigger);	
}

void checkEqual(const TerrainTile & actual, const TerrainTile & expected)
{
	//fatal fail here on any error
	VCMI_REQUIRE_FIELD_EQUAL(terType);
	VCMI_REQUIRE_FIELD_EQUAL(terView);
	VCMI_REQUIRE_FIELD_EQUAL(riverType);
	VCMI_REQUIRE_FIELD_EQUAL(riverDir);
	VCMI_REQUIRE_FIELD_EQUAL(roadType);
	VCMI_REQUIRE_FIELD_EQUAL(roadDir);
	VCMI_REQUIRE_FIELD_EQUAL(extTileFlags);
}

void MapComparer::compareHeader()
{
	//map size parameters are vital for further checks 
	VCMI_REQUIRE_FIELD_EQUAL_P(height);
	VCMI_REQUIRE_FIELD_EQUAL_P(width);
	VCMI_REQUIRE_FIELD_EQUAL_P(twoLevel);

	VCMI_CHECK_FIELD_EQUAL_P(name);
	VCMI_CHECK_FIELD_EQUAL_P(description);
	VCMI_CHECK_FIELD_EQUAL_P(difficulty);
	VCMI_CHECK_FIELD_EQUAL_P(levelLimit);

	VCMI_CHECK_FIELD_EQUAL_P(victoryMessage);
	VCMI_CHECK_FIELD_EQUAL_P(defeatMessage);
	VCMI_CHECK_FIELD_EQUAL_P(victoryIconIndex);
	VCMI_CHECK_FIELD_EQUAL_P(defeatIconIndex);
	
	checkEqual(actual->players, expected->players);	
	
	//todo: allowedHeroes, placeholdedHeroes
	
	
	std::vector<TriggeredEvent> actualEvents = actual->triggeredEvents;
	std::vector<TriggeredEvent> expectedEvents = expected->triggeredEvents;
	
	auto sortByIdentifier = [](const TriggeredEvent & lhs, const TriggeredEvent & rhs) -> bool
	{
		return lhs.identifier  < rhs.identifier;
	};
	boost::sort (actualEvents, sortByIdentifier);
	boost::sort (expectedEvents, sortByIdentifier);
	
	checkEqual(actualEvents, expectedEvents);	
}

void MapComparer::compareOptions()
{
	//rumors
	//disposedHeroes
	//predefinedHeroes
	//allowedSpell
	//allowedArtifact
	//allowedAbilities
	
	BOOST_ERROR("Not implemented compareOptions()");
}

void MapComparer::compareObjects()
{
	BOOST_ERROR("Not implemented compareObjects()");
}

void MapComparer::compareTerrain()
{
	//assume map dimensions check passed
	//todo: separate check for underground
	
	for(int x = 0; x < expected->width; x++)
		for(int y = 0; y < expected->height; y++)
		{
			int3 coord(x,y,0);
			BOOST_TEST_CHECKPOINT(coord);
			
			checkEqual(actual->getTile(coord), expected->getTile(coord));
		}
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
