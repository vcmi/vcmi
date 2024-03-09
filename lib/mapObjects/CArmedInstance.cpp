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
#include "../gameState/CGameState.h"
#include "../CPlayerState.h"
#include "../MetaString.h"

VCMI_LIB_NAMESPACE_BEGIN

void CArmedInstance::randomizeArmy(FactionID type)
{
	for (auto & elem : stacks)
	{
		if(elem.second->randomStack)
		{
			int level = elem.second->randomStack->level;
			int upgrade = elem.second->randomStack->upgrade;
			elem.second->setType((*VLC->townh)[type]->town->creatures[level][upgrade]);

			elem.second->randomStack = std::nullopt;
		}
		assert(elem.second->valid(false));
		assert(elem.second->armyObj == this);
	}
}

// Take Angelic Alliance troop-mixing freedom of non-evil units into account.
CSelector CArmedInstance::nonEvilAlignmentMixSelector = Selector::type()(BonusType::NONEVIL_ALIGNMENT_MIX);

CArmedInstance::CArmedInstance(IGameCallback *cb)
	:CArmedInstance(cb, false)
{
}

CArmedInstance::CArmedInstance(IGameCallback *cb, bool isHypothetic):
	CGObjectInstance(cb),
	CBonusSystemNode(isHypothetic),
	nonEvilAlignmentMix(this, nonEvilAlignmentMixSelector),
	battle(nullptr)
{
}

void CArmedInstance::updateMoraleBonusFromArmy()
{
	if(!validTypes(false)) //object not randomized, don't bother
		return;

	auto b = getExportedBonusList().getFirst(Selector::sourceType()(BonusSource::ARMY).And(Selector::type()(BonusType::MORALE)));
 	if(!b)
	{
		b = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::MORALE, BonusSource::ARMY, 0, BonusSourceID());
		addNewBonus(b);
	}

	//number of alignments and presence of undead
	std::set<FactionID> factions;
	bool hasUndead = false;

	const std::string undeadCacheKey = "type_UNDEAD";
	static const CSelector undeadSelector = Selector::type()(BonusType::UNDEAD);

	for(const auto & slot : Slots())
	{
		const CStackInstance * inst = slot.second;
		const auto * creature  = inst->getCreatureID().toEntity(VLC);

		factions.insert(creature->getFaction());
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

		for(auto f : factions)
		{
			if (VLC->factions()->getById(f)->getAlignment() != EAlignment::EVIL)
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
		b->val = 2 - static_cast<si32>(factionsInArmy);
		MetaString formatter;
		formatter.appendTextID("core.arraytxt.114"); //Troops of %d alignments %d
		formatter.replaceNumber(factionsInArmy);
		formatter.replaceNumber(b->val);

		description = formatter.toString();
		description = description.substr(0, description.size()-3);//trim value
	}
	
	boost::algorithm::trim(description);
	b->description = description;

	CBonusSystemNode::treeHasChanged();

	//-1 modifier for any Undead unit in army
	auto undeadModifier = getExportedBonusList().getFirst(Selector::source(BonusSource::ARMY, BonusCustomSource::undeadMoraleDebuff));
 	if(hasUndead)
	{
		if(!undeadModifier)
		{
			undeadModifier = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::MORALE, BonusSource::ARMY, -1, BonusCustomSource::undeadMoraleDebuff, VLC->generaltexth->arraytxt[116]);
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

CBonusSystemNode & CArmedInstance::whereShouldBeAttached(CGameState * gs)
{
	if(tempOwner.isValidPlayer())
		if(auto * where = gs->getPlayerState(tempOwner))
			return *where;

	return gs->globalEffects;
}

CBonusSystemNode & CArmedInstance::whatShouldBeAttached()
{
	return *this;
}

const IBonusBearer* CArmedInstance::getBonusBearer() const
{
	return this;
}

void CArmedInstance::serializeJsonOptions(JsonSerializeFormat & handler)
{
	CGObjectInstance::serializeJsonOptions(handler);
	CCreatureSet::serializeJson(handler, "army", 7);
}

VCMI_LIB_NAMESPACE_END
