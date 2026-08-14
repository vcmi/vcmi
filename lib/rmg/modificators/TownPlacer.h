/*
 * TownPlacer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "../Zone.h"

class ObjectManager;
class CGTownInstance;

class TownPlacer: public Modificator
{
public:
	MODIFICATOR(TownPlacer);
	
	void process() override;
	void init() override;
	
	int getTotalTowns() const;
	
private:
	bool hasTownExitInsideZone(const rmg::Object & rmgObject, const int3 & offset = int3()) const;

protected:
	void cleanupBoundaries(const rmg::Object & rmgObject);
	void addNewTowns(int count, bool hasFort, const PlayerColor & player, ObjectManager & manager);
	FactionID getRandomTownType(bool matchUndergroundType = false);
	FactionID getTownTypeFromHint(size_t hintIndex);
	bool hasTownTypeHint(size_t hintIndex) const;
	void placeTowns(ObjectManager & manager);
	bool placeMines(ObjectManager & manager);
	int3 placeMainTown(ObjectManager & manager, std::shared_ptr<CGTownInstance> town);

protected:
	int totalTowns = 0;
};
