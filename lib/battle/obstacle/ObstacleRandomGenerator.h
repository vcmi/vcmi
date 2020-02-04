/*
 * ObstacleRandomGenerator.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "battle/RangeGenerator.h"
#include "battle/RandGen.h"
#include "battle/obstacle/ObstacleJson.h"

class DLL_LINKAGE ObstacleRandomGenerator
{	
public:
	RandGen r;
	std::vector<int> getIndexesFromTerrainBattles(bool canBeRemovedBySpell);

	ObstacleRandomGenerator(int3 worldMapTile);
	void randomTilesAmountToBlock(int min, int max);
	void setTilesAmountToBlock(int amount);
	int getTilesAmountToBlock();
	int tilesToBlock;


	std::vector<std::shared_ptr<ObstacleJson>> appropriateObstacleConfigs;

private:
};
