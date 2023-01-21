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
#include "../VCAI.h"
#include "../FuzzyHelper.h"
#include "../AIhelper.h"
#include "../../../lib/mapping/CMap.h" //for victory conditions
#include "../../../lib/CPathfinder.h"

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

using namespace Goals;

bool AdventureSpellCast::operator==(const AdventureSpellCast & other) const
{
	return hero.h == other.hero.h;
}

TSubgoal AdventureSpellCast::whatToDoToAchieve()
{
	if(!hero.validAndSet())
		throw cannotFulfillGoalException("Invalid hero!");

	auto spell = getSpell();

	logAi->trace("Decomposing adventure spell cast of %s for hero %s", spell->getNameTranslated(), hero->getNameTranslated());

	if(!spell->isAdventure())
		throw cannotFulfillGoalException(spell->getNameTranslated() + " is not an adventure spell.");

	if(!hero->canCastThisSpell(spell))
		throw cannotFulfillGoalException("Hero can not cast " + spell->getNameTranslated());

	if(hero->mana < hero->getSpellCost(spell))
		throw cannotFulfillGoalException("Hero has not enough mana to cast " + spell->getNameTranslated());

	if(spellID == SpellID::TOWN_PORTAL && town && town->visitingHero)
		throw cannotFulfillGoalException("The town is already occupied by " + town->visitingHero->getNameTranslated());

	return iAmElementar();
}

void AdventureSpellCast::accept(VCAI * ai)
{
	if(town && spellID == SpellID::TOWN_PORTAL)
	{
		ai->selectedObject = town->id;
	}

	auto wait = cb->waitTillRealize;

	cb->waitTillRealize = true;
	cb->castSpell(hero.h, spellID, tile);

	if(town && spellID == SpellID::TOWN_PORTAL)
	{
		// visit town
		ai->moveHeroToTile(town->visitablePos(), hero);
	}

	cb->waitTillRealize = wait;

	throw goalFulfilledException(sptr(*this));
}

std::string AdventureSpellCast::name() const
{
	return "AdventureSpellCast " + getSpell()->getNameTranslated();
}

std::string AdventureSpellCast::completeMessage() const
{
	return "Spell cast successfully " + getSpell()->getNameTranslated();
}
