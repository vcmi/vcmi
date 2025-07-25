/*
* PrisonHeroPlacer.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#include "StdInc.h"
#include "PrisonHeroPlacer.h"
#include "../CMapGenerator.h"
#include "../RmgMap.h"
#include "TreasurePlacer.h"
#include "../CZonePlacer.h"
#include "../../GameLibrary.h"
#include "../../mapObjectConstructors/AObjectTypeHandler.h"
#include "../../mapObjectConstructors/CObjectClassesHandler.h"
#include "../../mapObjects/MapObjects.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

void PrisonHeroPlacer::process()
{
	getAllowedHeroes();
}

void PrisonHeroPlacer::init()
{
	// Reserve at least 16 heroes for each player
	reservedHeroes = 16 * generator.getMapGenOptions().getHumanOrCpuPlayerCount();
}

void PrisonHeroPlacer::getAllowedHeroes()
{
	// TODO: Give each zone unique HeroPlacer with private hero list?

	// Call that only once
	if (allowedHeroes.empty())
	{
		allowedHeroes = generator.getAllPossibleHeroes();
	}
}

int PrisonHeroPlacer::getPrisonsRemaining() const
{
	return std::max<int>(allowedHeroes.size() - reservedHeroes, 0);
}

HeroTypeID PrisonHeroPlacer::drawRandomHero()
{
	RecursiveLock lock(externalAccessMutex);
	if (getPrisonsRemaining() > 0)
	{
		RandomGeneratorUtil::randomShuffle(allowedHeroes, zone.getRand());
		HeroTypeID ret = allowedHeroes.back();
		allowedHeroes.pop_back();
		return ret;
	}
	else
	{
		throw rmgException("No unused heroes left for prisons!");
	}
}

void PrisonHeroPlacer::restoreDrawnHero(const HeroTypeID & hid)
{
	RecursiveLock lock(externalAccessMutex);
	allowedHeroes.push_back(hid);
}

VCMI_LIB_NAMESPACE_END
