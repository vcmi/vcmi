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
#include "../AIGateway.h"

namespace NKAI
{

using namespace Goals;

bool DismissHero::operator==(const DismissHero & other) const
{
	return hero.h == other.hero.h;
}

void DismissHero::accept(AIGateway * ai)
{
	if(!hero.validAndSet())
		throw cannotFulfillGoalException("Invalid hero!");

	cb->dismissHero(hero.h);

	throw goalFulfilledException(sptr(*this));
}

std::string DismissHero::toString() const
{
	return "DismissHero " + hero.name;
}

}
