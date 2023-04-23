/*
* FindObj.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "UnlockCluster.h"
#include "../AIGateway.h"
#include "../Engine/Nullkiller.h"
#include "../AIUtility.h"

namespace NKAI
{

extern boost::thread_specific_ptr<AIGateway> ai;

using namespace Goals;

bool UnlockCluster::operator==(const UnlockCluster & other) const
{
	return other.tile == tile;
}

std::string UnlockCluster::toString() const
{
	return "Unlock Cluster " + cluster->blocker->getObjectName() + tile.toString();
}

}
