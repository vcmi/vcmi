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

class ObjectManager;

class DLL_LINKAGE TownPlacer: public Modificator
{
public:
	MODIFICATOR(TownPlacer);
	
	void process() override;
	void init() override;
	
	int getTotalTowns() const;
	
protected:
	void cleanupBoundaries(const rmg::Object & rmgObject);
	void addNewTowns(int count, bool hasFort, PlayerColor player, ObjectManager & manager);
	si32 getRandomTownType(bool matchUndergroundType = false);
	void placeTowns(ObjectManager & manager);
	bool placeMines(ObjectManager & manager);
	
protected:
	
	int totalTowns;
};
