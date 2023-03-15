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
#include "Zone.h"

VCMI_LIB_NAMESPACE_BEGIN

class ObjectManager;
class CGTownInstance;

class TownPlacer: public Modificator
{
public:
	MODIFICATOR(TownPlacer);
	
	void process() override;
	void init() override;
	
	int getTotalTowns() const;
	
protected:
	void cleanupBoundaries(const rmg::Object & rmgObject);
	void addNewTowns(int count, bool hasFort, const PlayerColor & player, ObjectManager & manager);
	si32 getRandomTownType(bool matchUndergroundType = false);
	void placeTowns(ObjectManager & manager);
	bool placeMines(ObjectManager & manager);
	int3 placeMainTown(ObjectManager & manager, CGTownInstance & town);
	
protected:
	
	int totalTowns = 0;
};

VCMI_LIB_NAMESPACE_END
