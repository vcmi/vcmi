/*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#include "StdInc.h"
#include "MinePlacer.h"
#include "TownPlacer.h"
#include "ConnectionsPlacer.h"
#include "../CMapGenerator.h"
#include "../RmgMap.h"
#include "../../mapObjectConstructors/AObjectTypeHandler.h"
#include "../../mapObjectConstructors/CObjectClassesHandler.h"
#include "../../mapObjects/CGResource.h"
#include "../../mapObjects/MiscObjects.h"
#include "../../mapping/CMapEditManager.h"
#include "../RmgPath.h"
#include "../RmgObject.h"
#include "ObjectManager.h"
#include "../Functions.h"
#include "RoadPlacer.h"
#include "WaterAdopter.h"
#include "../TileInfo.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

void MinePlacer::process()
{
	auto * manager = zone.getModificator<ObjectManager>();
	if(!manager)
	{
		logGlobal->error("ObjectManager doesn't exist for zone %d, skip modificator %s", zone.getId(), getName());
		return;
	}

	placeMines(*manager);
}

void MinePlacer::init()
{
	DEPENDENCY(TownPlacer);
	DEPENDENCY(ConnectionsPlacer);
	POSTFUNCTION(ObjectManager);
	POSTFUNCTION(RoadPlacer);
}

bool MinePlacer::placeMines(ObjectManager & manager)
{
	std::vector<std::shared_ptr<CGMine>> createdMines;

	std::vector<std::pair<std::shared_ptr<CGObjectInstance>, ui32>> requiredObjects;

	for(const auto & mineInfo : zone.getMinesInfo())
	{
		const auto res = GameResID(mineInfo.first);
		for(int i = 0; i < mineInfo.second; ++i)
		{
			auto mineHandler = LIBRARY->objtypeh->getHandlerFor(Obj::MINE, res);
			const auto & rmginfo = mineHandler->getRMGInfo();
			auto mine = std::dynamic_pointer_cast<CGMine>(mineHandler->create(map.mapInstance->cb, nullptr));
			mine->producedResource = res;
			mine->tempOwner = PlayerColor::NEUTRAL;
			mine->producedQuantity = mine->defaultResProduction();
			createdMines.push_back(mine);


			if (!i && (res == EGameResID::WOOD || res == EGameResID::ORE))
			{
				//only first wood & ore mines are close
				manager.addCloseObject(RequiredObjectInfo(mine, rmginfo.value));
			}
			else
				requiredObjects.emplace_back(mine, rmginfo.value);
		}
	}

	//Shuffle mines to avoid patterns, but don't shuffle key objects like towns
	RandomGeneratorUtil::randomShuffle(requiredObjects, zone.getRand());
	for (const auto& obj : requiredObjects)
	{
		manager.addRequiredObject(RequiredObjectInfo(obj.first, obj.second));
	}

	//create extra resources
	if(int extraRes = generator.getConfig().mineExtraResources)
	{
		for(auto mine : createdMines)
		{
			for(int rc = zone.getRand().nextInt(1, extraRes); rc > 0; --rc)
			{
				auto resource = std::dynamic_pointer_cast<CGResource>(LIBRARY->objtypeh->getHandlerFor(Obj::RESOURCE, mine->producedResource)->create(map.mapInstance->cb, nullptr));

				RequiredObjectInfo roi;
				roi.obj = resource;
				roi.nearbyTarget = mine.get();
				manager.addNearbyObject(roi);
			}
		}
	}

	return true;
}

VCMI_LIB_NAMESPACE_END
