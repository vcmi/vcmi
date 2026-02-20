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

#include "CStackInstance.h"

#include "../../CPlayerState.h"
#include "../../entities/faction/CTown.h"
#include "../../entities/faction/CTownHandler.h"
#include "../../mapping/TerrainTile.h"
#include "../../GameLibrary.h"
#include "../../gameState/CGameState.h"

VCMI_LIB_NAMESPACE_BEGIN

void CArmedInstance::randomizeArmy(FactionID type)
{
	for(auto & elem : stacks)
	{
		if(elem.second->randomStack)
		{
			int level = elem.second->randomStack->level;
			int upgrade = elem.second->randomStack->upgrade;
			elem.second->setType((*LIBRARY->townh)[type]->town->creatures[level][upgrade]);

			elem.second->randomStack = std::nullopt;
		}
		assert(elem.second->valid(false));
		assert(elem.second->getArmy() == this);
	}
}

CArmedInstance::CArmedInstance(IGameInfoCallback * cb)
	: CArmedInstance(cb, BonusNodeType::ARMY, false)
{
}

CArmedInstance::CArmedInstance(IGameInfoCallback * cb, BonusNodeType nodeType, bool isHypothetic)
	: CGObjectInstance(cb)
	, CBonusSystemNode(nodeType, isHypothetic)
	, nonEvilAlignmentMix(this, Selector::type()(BonusType::NONEVIL_ALIGNMENT_MIX)) // Take Angelic Alliance troop-mixing freedom of non-evil units into account.
	, battle(nullptr)
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

	for(const auto & slot : Slots())
	{
		const auto * creature = slot.second->getCreatureID().toEntity(LIBRARY);

		factions.insert(creature->getFactionID());
		// Check for undead flag instead of faction (undead mummies are neutral)
		if(!hasUndead)
		{
			//this is costly check, let's skip it at first undead
			hasUndead |= slot.second->hasBonusOfType(BonusType::UNDEAD);
		}
	}

	size_t factionsInArmy = factions.size(); //town garrison seems to take both sets into account

	if(nonEvilAlignmentMix.hasBonus())
	{
		size_t mixableFactions = 0;

		for(auto f : factions)
		{
			if(LIBRARY->factions()->getById(f)->getAlignment() != EAlignment::EVIL)
				mixableFactions++;
		}
		if(mixableFactions > 0)
			factionsInArmy -= mixableFactions - 1;
	}

	MetaString bonusDescription;

	if(factionsInArmy == 1)
	{
		b->val = +1;
		bonusDescription.appendTextID("core.arraytxt.115"); //All troops of one alignment +1
	}
	else if(!factions.empty()) // no bonus from empty garrison
	{
		b->val = 2 - static_cast<si32>(factionsInArmy);
		bonusDescription.appendTextID("core.arraytxt.114"); //Troops of %d alignments %d
		bonusDescription.replaceNumber(factionsInArmy);
	}

	b->description = bonusDescription;

	nodeHasChanged();

	//-1 modifier for any Undead unit in army
	auto undeadModifier = getExportedBonusList().getFirst(Selector::source(BonusSource::ARMY, BonusCustomSource::undeadMoraleDebuff));
	if(hasUndead)
	{
		if(!undeadModifier)
		{
			undeadModifier = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::MORALE, BonusSource::ARMY, -1, BonusCustomSource::undeadMoraleDebuff);
			undeadModifier->description.appendTextID("core.arraytxt.116");
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

CBonusSystemNode & CArmedInstance::whereShouldBeAttached(CGameState & gs)
{
	if(tempOwner.isValidPlayer())
		if(auto * where = gs.getPlayerState(tempOwner))
			return *where;

	return gs.globalEffects;
}

CBonusSystemNode & CArmedInstance::whatShouldBeAttached()
{
	return *this;
}

void CArmedInstance::attachToBonusSystem(CGameState & gs)
{
	whatShouldBeAttached().attachTo(whereShouldBeAttached(gs));
}

void CArmedInstance::restoreBonusSystem(CGameState & gs)
{
	whatShouldBeAttached().attachTo(whereShouldBeAttached(gs));
	for(const auto & elem : stacks)
		elem.second->artDeserializationFix(gs, elem.second.get());
}

void CArmedInstance::detachFromBonusSystem(CGameState & gs)
{
	whatShouldBeAttached().detachFrom(whereShouldBeAttached(gs));
}

void CArmedInstance::attachUnitsToArmy()
{
	assert(getArmy() != nullptr);

	for(const auto & elem : stacks)
		elem.second->setArmy(getArmy());
}

const IBonusBearer * CArmedInstance::getBonusBearer() const
{
	return this;
}

void CArmedInstance::serializeJsonOptions(JsonSerializeFormat & handler)
{
	CGObjectInstance::serializeJsonOptions(handler);
	CCreatureSet::serializeJson(handler, "army", 7);
}

TerrainId CArmedInstance::getCurrentTerrain() const
{
	if(anchorPos().isValid())
		return cb->getTile(visitablePos())->getTerrainID();
	else
		return TerrainId::NONE;
}

VCMI_LIB_NAMESPACE_END
