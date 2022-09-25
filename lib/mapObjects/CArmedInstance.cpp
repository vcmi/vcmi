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

VCMI_LIB_NAMESPACE_BEGIN

void CArmedInstance::randomizeArmy(int type)
{
	for (auto & elem : stacks)
	{
		int & randID = elem.second->idRand;
		if(randID >= 0)
		{
			int level = randID / 2;
			bool upgrade = randID % 2;
			elem.second->setType((*VLC->townh)[type]->town->creatures[level][upgrade]);

			randID = -1;
		}
		assert(elem.second->valid(false));
		assert(elem.second->armyObj == this);
	}
	return;
}

// Take Angelic Alliance troop-mixing freedom of non-evil units into account.
CSelector CArmedInstance::nonEvilAlignmentMixSelector = Selector::type()(Bonus::NONEVIL_ALIGNMENT_MIX);

CArmedInstance::CArmedInstance()
	:CArmedInstance(false)
{
}

CArmedInstance::CArmedInstance(bool isHypotetic)
	:CBonusSystemNode(isHypotetic), nonEvilAlignmentMix(this, nonEvilAlignmentMixSelector)
{
	battle = nullptr;
}

void CArmedInstance::updateMoraleBonusFromArmy()
{
	if(!validTypes(false)) //object not randomized, don't bother
		return;

	auto b = getExportedBonusList().getFirst(Selector::sourceType()(Bonus::ARMY).And(Selector::type()(Bonus::MORALE)));
 	if(!b)
	{
		b = std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::MORALE, Bonus::ARMY, 0, -1);
		addNewBonus(b);
	}

	//number of alignments and presence of undead
	std::set<TFaction> factions;
	bool hasUndead = false;

	const std::string undeadCacheKey = "type_UNDEAD";
	static const CSelector undeadSelector = Selector::type()(Bonus::UNDEAD);

	for(auto slot : Slots())
	{
		const CStackInstance * inst = slot.second;
		const CCreature * creature  = VLC->creh->objects[inst->getCreatureID()];

		factions.insert(creature->faction);
		// Check for undead flag instead of faction (undead mummies are neutral)
		if (!hasUndead)
		{
			//this is costly check, let's skip it at first undead
			hasUndead |= inst->hasBonus(undeadSelector, undeadCacheKey);
		}
	}

	size_t factionsInArmy = factions.size(); //town garrison seems to take both sets into account

	if (nonEvilAlignmentMix.getHasBonus())
	{
		size_t mixableFactions = 0;

		for(TFaction f : factions)
		{
			if ((*VLC->townh)[f]->alignment != EAlignment::EVIL)
				mixableFactions++;
		}
		if (mixableFactions > 0)
			factionsInArmy -= mixableFactions - 1;
	}

	std::string description;

	if(factionsInArmy == 1)
	{
		b->val = +1;
		description = VLC->generaltexth->arraytxt[115]; //All troops of one alignment +1
		description = description.substr(0, description.size()-3);//trim "+1"
	}
	else if (!factions.empty()) // no bonus from empty garrison
	{
	 	b->val = 2 - (si32)factionsInArmy;
		description = boost::str(boost::format(VLC->generaltexth->arraytxt[114]) % factionsInArmy % b->val); //Troops of %d alignments %d
		description = b->description.substr(0, description.size()-2);//trim value
	}
	
	boost::algorithm::trim(description);
	b->description = description;

	CBonusSystemNode::treeHasChanged();

	//-1 modifier for any Undead unit in army
	const ui8 UNDEAD_MODIFIER_ID = -2;
	auto undeadModifier = getExportedBonusList().getFirst(Selector::source(Bonus::ARMY, UNDEAD_MODIFIER_ID));
 	if(hasUndead)
	{
		if(!undeadModifier)
		{
			undeadModifier = std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::MORALE, Bonus::ARMY, -1, UNDEAD_MODIFIER_ID, VLC->generaltexth->arraytxt[116]);
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
		return gs->getPlayerState(tempOwner);
	else
		return &gs->globalEffects;
}

CBonusSystemNode * CArmedInstance::whatShouldBeAttached()
{
	return this;
}

VCMI_LIB_NAMESPACE_END
