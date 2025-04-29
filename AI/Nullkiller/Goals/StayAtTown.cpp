/*
* ArmyUpgrade.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "StayAtTown.h"
#include "../AIGateway.h"
#include "../Engine/Nullkiller.h"
#include "../AIUtility.h"

namespace NKAI
{

using namespace Goals;

StayAtTown::StayAtTown(const CGTownInstance * town, AIPath & path)
	: ElementarGoal(Goals::STAY_AT_TOWN)
{
	sethero(path.targetHero);
	settown(town);
	movementWasted = static_cast<float>(hero->movementPointsRemaining()) / hero->movementPointsLimit(!hero->inBoat()) - path.movementCost();
	vstd::amax(movementWasted, 0);
}

bool StayAtTown::operator==(const StayAtTown & other) const
{
	return hero == other.hero && town == other.town;
}

std::string StayAtTown::toString() const
{
	return "Stay at town " + town->getNameTranslated()
		+ " hero " + hero->getNameTranslated()
		+ ", mana: " + std::to_string(hero->mana)
		+ " / " + std::to_string(hero->manaLimit());
}

void StayAtTown::accept(AIGateway * ai)
{
	ai->nullkiller->lockHero(hero, HeroLockedReason::DEFENCE);
}

}
