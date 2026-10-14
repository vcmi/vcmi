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
#include "../../../lib/spells/Problem.h"
#include "../../../lib/spells/adventure/TownPortalEffect.h"
#include "../../../lib/spells/CSpell.h"

namespace NK2AI
{

using namespace Goals;

bool AdventureSpellCast::operator==(const AdventureSpellCast & other) const
{
	return hero == other.hero
		&& spellID == other.spellID
		&& tile == other.tile
		&& town == other.town;
}

void AdventureSpellCast::accept(AIGateway * aiGw)
{
	if(!hero)
		throw cannotFulfillGoalException("Invalid hero!");

	auto spell = getSpell();

	logAi->trace("Decomposing adventure spell cast of %s for hero %s", spell->getNameTextID(), hero->getNameTextID());

	if(!spell->isAdventure())
		throw cannotFulfillGoalException(spell->getNameTranslated() + " is not an adventure spell.");

	if(!hero->canCastThisSpell(spell))
		throw cannotFulfillGoalException("Hero can not cast " + spell->getNameTranslated());

	if(hero->mana < hero->getSpellCost(spell))
		throw cannotFulfillGoalException("Hero has not enough mana to cast " + spell->getNameTranslated());

	auto townPortalEffect = spell->getAdventureMechanics().getEffectAs<TownPortalEffect>(hero);

	if(town && townPortalEffect)
	{
		aiGw->selectedObject = town->id;

		if(town->getVisitingHero() && town->tempOwner == aiGw->playerID && !town->getUpperArmy()->stacksCount())
		{
			aiGw->cc->swapGarrisonHero(town);
		}

		if(town->getVisitingHero())
			throw cannotFulfillGoalException("The town is already occupied by " + town->getVisitingHero()->getNameTextID());
	}

	if (hero->isGarrisoned())
		aiGw->cc->swapGarrisonHero(hero->getVisitedTown());

	if(aiGw->cc->isInTheMap(tile))
	{
		spells::detail::ProblemImpl problem;
		if(!spell->getAdventureMechanics().canBeCastAt(problem, aiGw->cc.get(), hero, tile))
			throw cannotFulfillGoalException("Can not cast " + spell->getNameTranslated() + " at " + tile.toString());
	}

	const auto wait = aiGw->cc->waitTillRealize;
	aiGw->cc->waitTillRealize = true;
	aiGw->cc->castSpell(hero, spellID, tile);
	aiGw->waitTillFree(); // Adventure spells may trigger visits and level-up queries.

	if(town && townPortalEffect)
	{
		// visit town
		aiGw->moveHeroToTile(town->visitablePos(), HeroPtr(hero, aiGw->cc.get()));
	}

	aiGw->cc->waitTillRealize = wait;
	throw goalFulfilledException(sptr(*this));
}

std::string AdventureSpellCast::toString() const
{
	return "AdventureSpellCast " + spellID.toSpell()->getNameTranslated();
}

}
