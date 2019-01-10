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
#include "../../lib/mapping/CMap.h" //for victory conditions
#include "../../lib/CPathfinder.h"

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

	auto spell = spellID.toSpell();

	logAi->trace("Decomposing adventure spell cast of %s for hero %s", spell->name, hero->name);

	if(!spell->isAdventureSpell())
		throw cannotFulfillGoalException(spell->name + " is not an adventure spell.");

	if(!hero->canCastThisSpell(spell))
		throw cannotFulfillGoalException("Hero can not cast " + spell->name);

	if(hero->mana < hero->getSpellCost(spell))
		throw cannotFulfillGoalException("Hero has not enough mana to cast " + spell->name);

	return iAmElementary();
}

void AdventureSpellCast::accept(VCAI * ai)
{
	cb->castSpell(hero.h, spellID, tile);
}

std::string AdventureSpellCast::name() const
{
	return "AdventureSpellCast " + spellID.toSpell()->name;
}

std::string AdventureSpellCast::completeMessage() const
{
	return "Spell casted successfully  " + spellID.toSpell()->name;
}
