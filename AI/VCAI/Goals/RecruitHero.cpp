/*
* RecruitHero.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "Goals.h"
#include "../VCAI.h"
#include "../AIUtility.h"
#include "../AIhelper.h"
#include "../FuzzyHelper.h"
#include "../ResourceManager.h"
#include "../BuildingManager.h"
#include "../../../lib/StringConstants.h"


extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

using namespace Goals;

TSubgoal RecruitHero::whatToDoToAchieve()
{
	const CGTownInstance * t = ai->findTownWithTavern();
	if(!t)
		return sptr(BuildThis(BuildingID::TAVERN).setpriority(2));

	TResources res;
	res[EGameResID::GOLD] = GameConstants::HERO_GOLD_COST;
	return ai->ah->whatToDo(res, iAmElementar()); //either buy immediately, or collect res
}
