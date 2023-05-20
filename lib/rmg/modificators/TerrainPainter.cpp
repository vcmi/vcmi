/*
 * TerrainPainter.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "TerrainPainter.h"
#include "TownPlacer.h"
#include "WaterAdopter.h"
#include "WaterProxy.h"
#include "ConnectionsPlacer.h"
#include "ObjectManager.h"
#include "../Functions.h"
#include "../CMapGenerator.h"
#include "../RmgMap.h"
#include "../../VCMI_Lib.h"
#include "../../TerrainHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

void TerrainPainter::process()
{
	initTerrainType();

	auto v = zone.getArea().getTilesVector();
	mapProxy->drawTerrain(generator.rand, v, zone.getTerrainType());
}

void TerrainPainter::init()
{
	DEPENDENCY(TownPlacer);
	DEPENDENCY_ALL(WaterAdopter);
	POSTFUNCTION_ALL(WaterProxy);
	POSTFUNCTION_ALL(ConnectionsPlacer);
	POSTFUNCTION(ObjectManager);
}

void TerrainPainter::initTerrainType()
{
	if(zone.getType()==ETemplateZoneType::WATER)
	{
		//collect all water terrain types
		std::vector<TerrainId> waterTerrains;
		for(const auto & terrain : VLC->terrainTypeHandler->objects)
			if(terrain->isWater())
				waterTerrains.push_back(terrain->getId());

		zone.setTerrainType(*RandomGeneratorUtil::nextItem(waterTerrains, generator.rand));
	}
	else
	{
		if(zone.isMatchTerrainToTown() && zone.getTownType() != ETownType::NEUTRAL)
		{
			auto terrainType = (*VLC->townh)[zone.getTownType()]->nativeTerrain;

			if (terrainType <= ETerrainId::NONE)
			{
				logGlobal->warn("Town %s has invalid terrain type: %d", zone.getTownType(), terrainType);
				zone.setTerrainType(ETerrainId::DIRT);
			}
			else
			{
				zone.setTerrainType(terrainType);
			}
		}
		else
		{
			auto terrainTypes = zone.getTerrainTypes();
			if (terrainTypes.empty())
			{
				logGlobal->warn("No terrain types found, falling back to DIRT");
				zone.setTerrainType(ETerrainId::DIRT);
			}
			else
			{
				zone.setTerrainType(*RandomGeneratorUtil::nextItem(terrainTypes, generator.rand));
			}
		}

		//Now, replace disallowed terrains on surface and in the underground
		const auto & terrainType = VLC->terrainTypeHandler->getById(zone.getTerrainType());

		if(zone.isUnderground())
		{
			if(!terrainType->isUnderground())
			{
				zone.setTerrainType(ETerrainId::SUBTERRANEAN);
			}
		}
		else
		{
			if (!terrainType->isSurface())
			{
				zone.setTerrainType(ETerrainId::DIRT);
			}
		}
	}
}

VCMI_LIB_NAMESPACE_END
