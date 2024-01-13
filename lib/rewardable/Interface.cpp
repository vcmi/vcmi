/*
 * Interface.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "Interface.h"

#include "../CHeroHandler.h"
#include "../TerrainHandler.h"
#include "../CPlayerState.h"
#include "../CSoundBase.h"
#include "../gameState/CGameState.h"
#include "../spells/CSpellHandler.h"
#include "../spells/ISpellMechanics.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/MiscObjects.h"
#include "../mapping/CMapDefines.h"
#include "../networkPacks/StackLocation.h"
#include "../networkPacks/PacksForClient.h"
#include "../IGameCallback.h"

VCMI_LIB_NAMESPACE_BEGIN

std::vector<ui32> Rewardable::Interface::getAvailableRewards(const CGHeroInstance * hero, Rewardable::EEventType event) const
{
	std::vector<ui32> ret;

	for(size_t i = 0; i < configuration.info.size(); i++)
	{
		const Rewardable::VisitInfo & visit = configuration.info[i];

		if(event == visit.visitType && (!hero || visit.limiter.heroAllowed(hero)))
		{
			logGlobal->trace("Reward %d is allowed", i);
			ret.push_back(static_cast<ui32>(i));
		}
	}
	return ret;
}

void Rewardable::Interface::grantRewardBeforeLevelup(IGameCallback * cb, const Rewardable::VisitInfo & info, const CGHeroInstance * hero) const
{
	assert(hero);
	assert(hero->tempOwner.isValidPlayer());
	assert(info.reward.creatures.size() <= GameConstants::ARMY_SIZE);

	cb->giveResources(hero->tempOwner, info.reward.resources);

	if (info.reward.revealTiles)
	{
		const auto & props = *info.reward.revealTiles;

		const auto functor = [&props](const TerrainTile * tile)
		{
			int score = 0;
			if (tile->terType->isSurface())
				score += props.scoreSurface;

			if (tile->terType->isUnderground())
				score += props.scoreSubterra;

			if (tile->terType->isWater())
				score += props.scoreWater;

			if (tile->terType->isRock())
				score += props.scoreRock;

			return score > 0;
		};

		std::unordered_set<int3> tiles;
		if (props.radius > 0)
		{
			cb->getTilesInRange(tiles, hero->getSightCenter(), props.radius, ETileVisibility::HIDDEN, hero->getOwner());
			if (props.hide)
				cb->getTilesInRange(tiles, hero->getSightCenter(), props.radius, ETileVisibility::REVEALED, hero->getOwner());

			vstd::erase_if(tiles, [&](const int3 & coord){
				return !functor(cb->getTile(coord));
			});
		}
		else
		{
			cb->getAllTiles(tiles, hero->tempOwner, -1, functor);
		}

		if (props.hide)
		{
			for (auto & player : cb->gameState()->players)
			{
				if (cb->getPlayerStatus(player.first) == EPlayerStatus::INGAME && cb->getPlayerRelations(player.first, hero->getOwner()) == PlayerRelations::ENEMIES)
					cb->changeFogOfWar(tiles, player.first, ETileVisibility::HIDDEN);
			}
		}
		else
		{
			cb->changeFogOfWar(tiles, hero->getOwner(), ETileVisibility::REVEALED);
		}
	}

	for(const auto & entry : info.reward.secondary)
	{
		int current = hero->getSecSkillLevel(entry.first);
		if( (current != 0 && current < entry.second) ||
			(hero->canLearnSkill() ))
		{
			cb->changeSecSkill(hero, entry.first, entry.second);
		}
	}

	for(int i=0; i< info.reward.primary.size(); i++)
		cb->changePrimSkill(hero, static_cast<PrimarySkill>(i), info.reward.primary[i], false);

	TExpType expToGive = 0;

	if (info.reward.heroLevel > 0)
		expToGive += VLC->heroh->reqExp(hero->level+info.reward.heroLevel) - VLC->heroh->reqExp(hero->level);

	if (info.reward.heroExperience > 0)
		expToGive += hero->calculateXp(info.reward.heroExperience);

	if(expToGive)
		cb->giveExperience(hero, expToGive);
}

void Rewardable::Interface::grantRewardAfterLevelup(IGameCallback * cb, const Rewardable::VisitInfo & info, const CArmedInstance * army, const CGHeroInstance * hero) const
{
	if(info.reward.manaDiff || info.reward.manaPercentage >= 0)
		cb->setManaPoints(hero->id, info.reward.calculateManaPoints(hero));

	if(info.reward.movePoints || info.reward.movePercentage >= 0)
	{
		SetMovePoints smp;
		smp.hid = hero->id;
		smp.val = hero->movementPointsRemaining();

		if (info.reward.movePercentage >= 0) // percent from max
			smp.val = hero->movementPointsLimit(hero->boat && hero->boat->layer == EPathfindingLayer::SAIL) * info.reward.movePercentage / 100;
		smp.val = std::max<si32>(0, smp.val + info.reward.movePoints);

		cb->setMovePoints(&smp);
	}

	for(const Bonus & bonus : info.reward.bonuses)
	{
		GiveBonus gb;
		gb.who = GiveBonus::ETarget::OBJECT;
		gb.bonus = bonus;
		gb.id = hero->id;
		cb->giveHeroBonus(&gb);
	}

	for(const ArtifactID & art : info.reward.artifacts)
		cb->giveHeroNewArtifact(hero, art.toArtifact(), ArtifactPosition::FIRST_AVAILABLE);

	if(!info.reward.spells.empty())
	{
		std::set<SpellID> spellsToGive(info.reward.spells.begin(), info.reward.spells.end());
		cb->changeSpells(hero, true, spellsToGive);
	}

	if(!info.reward.creaturesChange.empty())
	{
		for(const auto & slot : hero->Slots())
		{
			const CStackInstance * heroStack = slot.second;

			for(const auto & change : info.reward.creaturesChange)
			{
				if (heroStack->type->getId() == change.first)
				{
					StackLocation location(hero, slot.first);
					cb->changeStackType(location, change.second.toCreature());
					break;
				}
			}
		}
	}

	if(!info.reward.creatures.empty())
	{
		CCreatureSet creatures;
		for(const auto & crea : info.reward.creatures)
			creatures.addToSlot(creatures.getFreeSlot(), new CStackInstance(crea.type, crea.count));

		if(auto * army = dynamic_cast<const CArmedInstance*>(this)) //TODO: to fix that, CArmedInstance must be splitted on map instance part and interface part
			cb->giveCreatures(army, hero, creatures, false);
	}
	
	if(info.reward.spellCast.first != SpellID::NONE)
	{
		caster.setActualCaster(hero);
		caster.setSpellSchoolLevel(info.reward.spellCast.second);
		cb->castSpell(&caster, info.reward.spellCast.first, int3{-1, -1, -1});
	}

	if(info.reward.removeObject)
		if(auto * instance = dynamic_cast<const CGObjectInstance*>(this))
			cb->removeAfterVisit(instance);
}

void Rewardable::Interface::serializeJson(JsonSerializeFormat & handler)
{
	configuration.serializeJson(handler);
}

VCMI_LIB_NAMESPACE_END
