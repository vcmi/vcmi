/*
* AdventureSpellCast.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "AdventureSpellCast.h"
#include "../AIGateway.h"
#include "../../../lib/spells/ISpellMechanics.h"
#include "../../../lib/spells/adventure/TownPortalEffect.h"

namespace NKAI
{

using namespace Goals;

bool AdventureSpellCast::operator==(const AdventureSpellCast & other) const
{
	return hero == other.hero;
}

void AdventureSpellCast::accept(AIGateway * ai)
{
	if(!hero)
		throw cannotFulfillGoalException("Invalid hero!");

	auto spell = getSpell();

	logAi->trace("Decomposing adventure spell cast of %s for hero %s", spell->getNameTranslated(), hero->getNameTranslated());

	if(!spell->isAdventure())
		throw cannotFulfillGoalException(spell->getNameTranslated() + " is not an adventure spell.");

	if(!hero->canCastThisSpell(spell))
		throw cannotFulfillGoalException("Hero can not cast " + spell->getNameTranslated());

	if(hero->mana < hero->getSpellCost(spell))
		throw cannotFulfillGoalException("Hero has not enough mana to cast " + spell->getNameTranslated());

	auto townPortalEffect = spell->getAdventureMechanics().getEffectAs<TownPortalEffect>(hero);

	if(town && townPortalEffect)
	{
		ai->selectedObject = town->id;

		if(town->getVisitingHero() && town->tempOwner == ai->playerID && !town->getUpperArmy()->stacksCount())
		{
			ai->myCb->swapGarrisonHero(town);
		}

		if(town->getVisitingHero())
			throw cannotFulfillGoalException("The town is already occupied by " + town->getVisitingHero()->getNameTranslated());
	}

	if (hero->isGarrisoned())
		ai->myCb->swapGarrisonHero(hero->getVisitedTown());

	auto wait = cb->waitTillRealize;

	cb->waitTillRealize = true;
	cb->castSpell(hero, spellID, tile);

	if(town && townPortalEffect)
	{
		// visit town
		ai->moveHeroToTile(town->visitablePos(), hero);
	}

	cb->waitTillRealize = wait;

	throw goalFulfilledException(sptr(*this));
}

std::string AdventureSpellCast::toString() const
{
	return "AdventureSpellCast " + spellID.toSpell()->getNameTranslated();
}

}
