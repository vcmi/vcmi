/*
 * ObstaclePlacer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "Modificator.h"
#include "../mapping/ObstacleProxy.h"

VCMI_LIB_NAMESPACE_BEGIN

class CMap;
class CMapEditManager;
class RiverPlacer;
class ObjectManager;
class ObstaclePlacer: public Modificator, public ObstacleProxy
{
public:
	MODIFICATOR(ObstaclePlacer);
	
	void process() override;
	void init() override;

	bool isInTheMap(const int3& tile) override;
	
	std::pair<bool, bool> verifyCoverage(const int3 & t) const override;
	
	void placeObject(rmg::Object & object, std::set<CGObjectInstance*> & instances) override;
	
	void postProcess(const rmg::Object & object) override;
	
	bool isProhibited(const rmg::Area & objArea) const override;
	
private:
	rmg::Area prohibitedArea;
	RiverPlacer * riverManager;
	ObjectManager * manager;
};

VCMI_LIB_NAMESPACE_END
