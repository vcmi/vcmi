/*
* CaptureObject.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "../StdInc.h"
#include "CaptureObject.h"
#include "../../../lib/mapObjects/CGTownInstance.h"
#include "../AIGateway.h"
#include "../Engine/Nullkiller.h"
#include "../Behaviors/CaptureObjectsBehavior.h"


namespace NKAI
{

extern boost::thread_specific_ptr<CCallback> cb;

using namespace Goals;

bool CaptureObject::operator==(const CaptureObject & other) const
{
	return objid == other.objid;
}

uint64_t CaptureObject::getHash() const
{
	return objid;
}

std::string CaptureObject::toString() const
{
	return "Capture " + name + " at " + tile.toString();
}

TGoalVec CaptureObject::decompose() const
{
	return CaptureObjectsBehavior(cb->getObj(ObjectInstanceID(objid))).decompose();
}

}
