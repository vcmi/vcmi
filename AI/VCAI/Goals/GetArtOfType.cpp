/*
* GetArtOfType.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "GetArtOfType.h"
#include "FindObj.h"
#include "../VCAI.h"
#include "../AIUtility.h"


extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

using namespace Goals;

bool GetArtOfType::operator==(const GetArtOfType & other) const
{
	return other.hero.h == hero.h && other.objid == objid;
}

TSubgoal GetArtOfType::whatToDoToAchieve()
{
	return sptr(FindObj(Obj::ARTIFACT, aid));
}