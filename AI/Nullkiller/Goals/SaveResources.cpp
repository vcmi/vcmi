/*
* SaveResources.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "SaveResources.h"
#include "../AIGateway.h"
#include "../Behaviors/CaptureObjectsBehavior.h"

namespace NKAI
{

using namespace Goals;

bool SaveResources::operator==(const SaveResources & other) const
{
	return true;
}

void SaveResources::accept(AIGateway * ai)
{
	ai->nullkiller->lockResources(resources);

	logAi->debug("Locked %s resources", resources.toString());

	throw goalFulfilledException(sptr(*this));
}

std::string SaveResources::toString() const
{
	return "SaveResources " + resources.toString();
}

}
