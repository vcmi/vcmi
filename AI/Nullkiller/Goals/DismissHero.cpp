/*
* DismissHero.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "DismissHero.h"
#include "../VCAI.h"
#include "../FuzzyHelper.h"
#include "../AIhelper.h"
#include "../../../lib/mapping/CMap.h" //for victory conditions
#include "../../../lib/CPathfinder.h"

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

using namespace Goals;

bool DismissHero::operator==(const DismissHero & other) const
{
	return hero.h == other.hero.h;
}

TSubgoal DismissHero::whatToDoToAchieve()
{
	if(!hero.validAndSet())
		throw cannotFulfillGoalException("Invalid hero!");

	return iAmElementar();
}

void DismissHero::accept(VCAI * ai)
{
	if(!hero.validAndSet())
		throw cannotFulfillGoalException("Invalid hero!");

	cb->dismissHero(hero.h);

	throw goalFulfilledException(sptr(*this));
}

std::string DismissHero::name() const
{
	return "DismissHero " + hero.name;
}
