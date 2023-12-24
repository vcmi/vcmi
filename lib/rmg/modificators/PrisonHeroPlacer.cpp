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
#include "../../VCMI_Lib.h"
#include "../../mapObjectConstructors/AObjectTypeHandler.h"
#include "../../mapObjectConstructors/CObjectClassesHandler.h"
#include "../../mapObjects/MapObjects.h" 

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

int PrisonHeroPlacer::getPrisonsRemaning() const
{
	return std::max<int>(allowedHeroes.size() - reservedHeroes, 0);
}

HeroTypeID PrisonHeroPlacer::drawRandomHero()
{
	RecursiveLock lock(externalAccessMutex);
	if (getPrisonsRemaning() > 0)
	{
		RandomGeneratorUtil::randomShuffle(allowedHeroes, zone.getRand());
        HeroTypeID ret = allowedHeroes.back();
        allowedHeroes.pop_back();

        generator.banHero(ret);
		return ret;
	}
	else
	{
		throw rmgException("No quest heroes left for prisons!");
	}
}

void PrisonHeroPlacer::unbanHero(const HeroTypeID & hid)
{
	RecursiveLock lock(externalAccessMutex);
	generator.unbanHero(hid);
}

VCMI_LIB_NAMESPACE_END
