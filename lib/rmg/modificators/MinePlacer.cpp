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
#include "../../mapping/CMapEditManager.h"
#include "../../mapObjects/CObjectClassesHandler.h"
#include "../RmgPath.h"
#include "../RmgObject.h"
#include "ObjectManager.h"
#include "../Functions.h"
#include "RoadPlacer.h"
#include "WaterAdopter.h"
#include "../TileInfo.h"

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
	std::vector<CGMine*> createdMines;

	std::vector<std::pair<CGObjectInstance*, ui32>> requiredObjects;

	for(const auto & mineInfo : zone.getMinesInfo())
	{
		const auto res = GameResID(mineInfo.first);
		for(int i = 0; i < mineInfo.second; ++i)
		{
			auto mineHandler = VLC->objtypeh->getHandlerFor(Obj::MINE, res);
			const auto & rmginfo = mineHandler->getRMGInfo();
			auto * mine = dynamic_cast<CGMine *>(mineHandler->create());
			mine->producedResource = res;
			mine->tempOwner = PlayerColor::NEUTRAL;
			mine->producedQuantity = mine->defaultResProduction();
			createdMines.push_back(mine);


			if(!i && (res == EGameResID::WOOD || res == EGameResID::ORE))
				manager.addCloseObject(mine, rmginfo.value); //only first wood&ore mines are close
			else
				requiredObjects.push_back(std::pair<CGObjectInstance*, ui32>(mine, rmginfo.value));
		}
	}

	//Shuffle mines to avoid patterns, but don't shuffle key objects like towns
	RandomGeneratorUtil::randomShuffle(requiredObjects, zone.getRand());
	for (const auto& obj : requiredObjects)
	{
		manager.addRequiredObject(obj.first, obj.second);
	}

	//create extra resources
	if(int extraRes = generator.getConfig().mineExtraResources)
	{
		for(auto * mine : createdMines)
		{
			for(int rc = zone.getRand().nextInt(1, extraRes); rc > 0; --rc)
			{
				auto * resourse = dynamic_cast<CGResource *>(VLC->objtypeh->getHandlerFor(Obj::RESOURCE, mine->producedResource)->create());
				resourse->amount = CGResource::RANDOM_AMOUNT;
				manager.addNearbyObject(resourse, mine);
			}
		}
	}

	return true;
}

VCMI_LIB_NAMESPACE_END
