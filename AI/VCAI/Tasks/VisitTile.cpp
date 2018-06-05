/*
* Goals.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "../VCAI.h"
#include "lib/mapping/CMap.h" //for victory conditions
#include "lib/CPathfinder.h"
#include "VisitTile.h"

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;

using namespace Tasks;

void VisitTile::execute()
{
	if (!hero->movement)
		return;

	if (tile == hero->visitablePos() && cb->getVisitableObjs(hero->visitablePos()).size() < 2)
	{
		logAi->warn("Why do I want to move hero %s to tile %s? Already standing on that tile! ", hero->name, tile.toString());
		return;
	}

	ai->moveHeroToTile(tile, hero.get());
}

bool VisitTile::canExecute()
{
	if (!hero->movement)
		return false;

	CGPath path;

	cb->getPathsInfo(hero.get())->getPath(path, tile);

	if (path.nodes.size() <= 1) {
		return false;
	}

	// path is reversed. The last node is where hero stands
	auto nextNode = path.nodes.at(path.nodes.size() - 2);

	return nextNode.turns == 0;
}

std::string Tasks::VisitTile::toString()
{
	auto baseInfo = "VisitTile " + hero->name + " => " + tile.toString();

	return objInfo.size() ? baseInfo + " [" + objInfo + "]" : baseInfo;
}