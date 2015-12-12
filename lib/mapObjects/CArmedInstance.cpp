/*
 * CArmedInstance.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CArmedInstance.h"

#include "../CTownHandler.h"
#include "../CCreatureHandler.h"
#include "../CGeneralTextHandler.h"
#include "../CGameState.h"
#include "../CPlayerState.h"

void CArmedInstance::randomizeArmy(int type)
{
	for (auto & elem : stacks)
	{
		int & randID = elem.second->idRand;
		if(randID >= 0)
		{
			int level = randID / 2;
			bool upgrade = randID % 2;
			elem.second->setType(VLC->townh->factions[type]->town->creatures[level][upgrade]);

			randID = -1;
		}
		assert(elem.second->valid(false));
		assert(elem.second->armyObj == this);
	}
	return;
}

CArmedInstance::CArmedInstance()
{
	battle = nullptr;
}

void CArmedInstance::updateMoraleBonusFromArmy()
{
	if(!validTypes(false)) //object not randomized, don't bother
		return;

	Bonus *b = getExportedBonusList().getFirst(Selector::sourceType(Bonus::ARMY).And(Selector::type(Bonus::MORALE)));
	if(!b)
	{
		b = new Bonus(Bonus::PERMANENT, Bonus::MORALE, Bonus::ARMY, 0, -1);
		addNewBonus(b);
	}

	//number of alignments and presence of undead
	std::set<TFaction> factions;
	bool hasUndead = false;

	for(auto slot : Slots())
	{
		const CStackInstance * inst = slot.second;
		const CCreature * creature  = VLC->creh->creatures[inst->getCreatureID()];

		factions.insert(creature->faction);
		// Check for undead flag instead of faction (undead mummies are neutral)
		hasUndead |= inst->hasBonusOfType(Bonus::UNDEAD);
	}

	size_t factionsInArmy = factions.size(); //town garrison seems to take both sets into account

	// Take Angelic Alliance troop-mixing freedom of non-evil units into account.
	if (hasBonusOfType(Bonus::NONEVIL_ALIGNMENT_MIX))
	{
		size_t mixableFactions = 0;

		for(TFaction f : factions)
		{
			if (VLC->townh->factions[f]->alignment != EAlignment::EVIL)
				mixableFactions++;
		}
		if (mixableFactions > 0)
			factionsInArmy -= mixableFactions - 1;
	}

	if(factionsInArmy == 1)
	{
		b->val = +1;
		b->description = VLC->generaltexth->arraytxt[115]; //All troops of one alignment +1
		b->description = b->description.substr(0, b->description.size()-3);//trim "+1"
	}
	else if (!factions.empty()) // no bonus from empty garrison
	{
	 	b->val = 2 - factionsInArmy;
		b->description = boost::str(boost::format(VLC->generaltexth->arraytxt[114]) % factionsInArmy % b->val); //Troops of %d alignments %d
		b->description = b->description.substr(0, b->description.size()-2);//trim value
	}
	boost::algorithm::trim(b->description);
	CBonusSystemNode::treeHasChanged();

	//-1 modifier for any Undead unit in army
	const ui8 UNDEAD_MODIFIER_ID = -2;
	Bonus *undeadModifier = getExportedBonusList().getFirst(Selector::source(Bonus::ARMY, UNDEAD_MODIFIER_ID));
 	if(hasUndead)
	{
		if(!undeadModifier)
		{
			undeadModifier = new Bonus(Bonus::PERMANENT, Bonus::MORALE, Bonus::ARMY, -1, UNDEAD_MODIFIER_ID, VLC->generaltexth->arraytxt[116]);
			undeadModifier->description = undeadModifier->description.substr(0, undeadModifier->description.size()-2);//trim value
			addNewBonus(undeadModifier);
		}			
	}
	else if(undeadModifier)
		removeBonus(undeadModifier);

}

void CArmedInstance::armyChanged()
{
	updateMoraleBonusFromArmy();
}

CBonusSystemNode * CArmedInstance::whereShouldBeAttached(CGameState *gs)
{
	if(tempOwner < PlayerColor::PLAYER_LIMIT)
		return gs->getPlayer(tempOwner);
	else
		return &gs->globalEffects;
}

CBonusSystemNode * CArmedInstance::whatShouldBeAttached()
{
	return this;
}
