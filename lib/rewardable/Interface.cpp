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

#include "../TerrainHandler.h"
#include "../CPlayerState.h"
#include "../CSoundBase.h"
#include "../callback/IGameInfoCallback.h"
#include "../callback/IGameEventCallback.h"
#include "../entities/hero/CHeroHandler.h"
#include "../gameState/CGameState.h"
#include "../spells/CSpellHandler.h"
#include "../spells/ISpellMechanics.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/MiscObjects.h"
#include "../mapping/CMapDefines.h"
#include "../networkPacks/StackLocation.h"
#include "../networkPacks/PacksForClient.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

std::vector<ui32> Rewardable::Interface::getAvailableRewards(const CGHeroInstance * hero, Rewardable::EEventType event) const
{
	std::vector<ui32> ret;

	for(size_t i = 0; i < configuration.info.size(); i++)
	{
		const Rewardable::VisitInfo & visit = configuration.info[i];

		if(event == visit.visitType && (!hero || visit.limiter.heroAllowed(hero)))
			ret.push_back(static_cast<ui32>(i));
	}
	return ret;
}

void Rewardable::Interface::grantRewardBeforeLevelup(IGameEventCallback & gameEvents, const Rewardable::VisitInfo & info, const CGHeroInstance * hero) const
{
	auto cb = getObject()->cb;

	assert(hero);
	assert(hero->tempOwner.isValidPlayer());
	assert(info.reward.creatures.size() <= GameConstants::ARMY_SIZE);

	gameEvents.giveResources(hero->tempOwner, info.reward.resources);

	if (info.reward.revealTiles)
	{
		const auto & props = *info.reward.revealTiles;

		const auto functor = [&props](const TerrainTile * tile)
		{
			int score = 0;
			if (tile->getTerrain()->isSurface())
				score += props.scoreSurface;

			if (tile->getTerrain()->isUnderground())
				score += props.scoreSubterra;

			if (tile->getTerrain()->isWater())
				score += props.scoreWater;

			if (tile->getTerrain()->isRock())
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
			for (auto & player : cb->gameState().players)
			{
				if (cb->getPlayerStatus(player.first) == EPlayerStatus::INGAME && cb->getPlayerRelations(player.first, hero->getOwner()) == PlayerRelations::ENEMIES)
					gameEvents.changeFogOfWar(tiles, player.first, ETileVisibility::HIDDEN);
			}
		}
		else
		{
			gameEvents.changeFogOfWar(tiles, hero->getOwner(), ETileVisibility::REVEALED);
		}
	}

	for(const auto & entry : info.reward.secondary)
	{
		int currentLevel = hero->getSecSkillLevel(entry.first);
		int newLevel = currentLevel + entry.second;
		int newLevelClamped = std::clamp<int>(newLevel, MasteryLevel::NONE, MasteryLevel::EXPERT);

		if(currentLevel != newLevelClamped)
			gameEvents.changeSecSkill(hero, entry.first, newLevelClamped, ChangeValueMode::ABSOLUTE);
	}

	for(int i=0; i< info.reward.primary.size(); i++)
		if (info.reward.primary[i] != 0)
			gameEvents.changePrimSkill(hero, static_cast<PrimarySkill>(i), info.reward.primary[i], ChangeValueMode::RELATIVE);

	TExpType expToGive = 0;

	if (info.reward.heroLevel > 0)
		expToGive += LIBRARY->heroh->reqExp(hero->level+info.reward.heroLevel) - LIBRARY->heroh->reqExp(hero->level);

	if (info.reward.heroExperience > 0)
		expToGive += hero->calculateXp(info.reward.heroExperience);

	if(expToGive)
		gameEvents.giveExperience(hero, expToGive);
}

void Rewardable::Interface::grantRewardAfterLevelup(IGameEventCallback & gameEvents, const Rewardable::VisitInfo & info, const CArmedInstance * army, const CGHeroInstance * hero) const
{
	auto cb = getObject()->cb;

	if(info.reward.manaDiff || info.reward.manaPercentage >= 0)
		gameEvents.setManaPoints(hero->id, info.reward.calculateManaPoints(hero));

	if(info.reward.movePoints || info.reward.movePercentage >= 0)
	{
		SetMovePoints smp;
		smp.hid = hero->id;
		smp.val = hero->movementPointsRemaining();

		if (info.reward.movePercentage >= 0) // percent from max
			smp.val = hero->movementPointsLimit(hero->inBoat() && hero->getBoat()->layer == EPathfindingLayer::SAIL) * info.reward.movePercentage / 100;
		smp.val = std::max<si32>(0, smp.val + info.reward.movePoints);

		gameEvents.setMovePoints(&smp);
	}

	for(const auto & bonus : info.reward.heroBonuses)
	{
		GiveBonus gb(GiveBonus::ETarget::OBJECT, hero->id, *bonus);
		gameEvents.giveHeroBonus(&gb);
	}

	if (hero->getCommander())
	{
		for(const auto & bonus : info.reward.commanderBonuses)
		{
			GiveBonus gb(GiveBonus::ETarget::HERO_COMMANDER, hero->id, *bonus);
			gameEvents.giveHeroBonus(&gb);
		}
	}

	for(const auto & bonus : info.reward.playerBonuses)
	{
		GiveBonus gb(GiveBonus::ETarget::PLAYER, hero->getOwner(), *bonus);
		gameEvents.giveHeroBonus(&gb);
	}

	for(const ArtifactID & art : info.reward.takenArtifacts)
	{
		// hero does not have such artifact alone, but he might have it as part of assembled artifact
		if(!hero->hasArt(art))
		{
			const auto * assembly = hero->getCombinedArtWithPart(art);
			if (assembly)
			{
				DisassembledArtifact da;
				da.al = ArtifactLocation(hero->id, hero->getArtPos(assembly));
				gameEvents.sendAndApply(da);
			}
		}

		if(hero->hasArt(art))
			gameEvents.removeArtifact(ArtifactLocation(hero->id, hero->getArtPos(art, false)));
	}

	for(const ArtifactPosition & slot : info.reward.takenArtifactSlots)
	{
		const auto & slotContent = hero->getSlot(slot);

		if (!slotContent->locked && slotContent->artifactID.hasValue())
			gameEvents.removeArtifact(ArtifactLocation(hero->id, slot));

		// TODO: handle locked slots?
	}

	for(const SpellID & spell : info.reward.takenScrolls)
	{
		if(hero->hasScroll(spell, false))
			gameEvents.removeArtifact(ArtifactLocation(hero->id, hero->getScrollPos(spell, false)));
	}

	for(const ArtifactID & art : info.reward.grantedArtifacts)
		gameEvents.giveHeroNewArtifact(hero, art, ArtifactPosition::FIRST_AVAILABLE);

	for(const SpellID & spell : info.reward.grantedScrolls)
		gameEvents.giveHeroNewScroll(hero, spell, ArtifactPosition::FIRST_AVAILABLE);

	if(!info.reward.spells.empty())
	{
		std::set<SpellID> spellsToGive;

		for (auto const & spell : info.reward.spells)
			if (hero->canLearnSpell(spell.toEntity(LIBRARY), true))
				spellsToGive.insert(spell);

		if (!spellsToGive.empty())
			gameEvents.changeSpells(hero, true, spellsToGive);
	}

	if (!info.reward.takenCreatures.empty())
	{
		gameEvents.takeCreatures(hero->id, info.reward.takenCreatures, !info.reward.creatures.empty());
	}

	if(!info.reward.creaturesChange.empty())
	{
		for(const auto & slot : hero->Slots())
		{
			const auto & heroStack = slot.second;

			for(const auto & change : info.reward.creaturesChange)
			{
				if (heroStack->getId() == change.first)
				{
					StackLocation location(hero->id, slot.first);
					gameEvents.changeStackType(location, change.second.toCreature());
					break;
				}
			}
		}
	}

	if(!info.reward.creatures.empty())
	{
		CCreatureSet creatures;
		for(const auto & crea : info.reward.creatures)
			creatures.addToSlot(creatures.getFreeSlot(), std::make_unique<CStackInstance>(cb, crea.getId(), crea.getCount()));

		auto * army = dynamic_cast<const CArmedInstance*>(this);
		if (army)
			gameEvents.giveCreatures(army, hero, creatures, false);
		else
			gameEvents.giveCreatures(hero, creatures);
	}
	
	if(info.reward.spellCast.first != SpellID::NONE)
	{
		caster.setActualCaster(hero);
		caster.setSpellSchoolLevel(info.reward.spellCast.second);
		gameEvents.castSpell(&caster, info.reward.spellCast.first, int3{-1, -1, -1});
	}

	if(info.reward.removeObject)
		if(auto * instance = dynamic_cast<const CGObjectInstance*>(this))
			gameEvents.removeAfterVisit(instance->id);
}

void Rewardable::Interface::serializeJson(JsonSerializeFormat & handler)
{
	configuration.serializeJson(handler);
}

void Rewardable::Interface::grantRewardWithMessage(IGameEventCallback & gameEvents, const CGHeroInstance * contextHero, int index, bool markAsVisit) const
{
	auto vi = configuration.info.at(index);
	logGlobal->debug("Granting reward %d. Message says: %s", index, vi.message.toString());
	// show message only if it is not empty or in infobox
	if (configuration.infoWindowType != EInfoWindowMode::MODAL || !vi.message.toString().empty())
	{
		InfoWindow iw;
		iw.player = contextHero->tempOwner;
		iw.text = vi.message;
		vi.reward.loadComponents(iw.components, contextHero);
		iw.type = configuration.infoWindowType;
		if(!iw.components.empty() || !iw.text.toString().empty())
			gameEvents.showInfoDialog(&iw);
	}
	// grant reward afterwards. Note that it may remove object
	if(markAsVisit)
		markAsVisited(gameEvents, contextHero);
	grantReward(gameEvents, index, contextHero);
}

void Rewardable::Interface::selectRewardWithMessage(IGameEventCallback & gameEvents, const CGHeroInstance * contextHero, const std::vector<ui32> & rewardIndices, const MetaString & dialog) const
{
	BlockingDialog sd(configuration.canRefuse, rewardIndices.size() > 1);
	sd.player = contextHero->tempOwner;
	sd.text = dialog;
	sd.components = loadComponents(contextHero, rewardIndices);
	gameEvents.showBlockingDialog(getObject(), &sd);
}

std::vector<Component> Rewardable::Interface::loadComponents(const CGHeroInstance * contextHero, const std::vector<ui32> & rewardIndices) const
{
	std::vector<Component> result;

	if (rewardIndices.empty())
		return result;

	if (configuration.selectMode != Rewardable::SELECT_FIRST && rewardIndices.size() > 1)
	{
		for (auto index : rewardIndices)
			result.push_back(configuration.info.at(index).reward.getDisplayedComponent(contextHero));
	}
	else
	{
		configuration.info.at(rewardIndices.front()).reward.loadComponents(result, contextHero);
	}

	return result;
}

void Rewardable::Interface::grantAllRewardsWithMessage(IGameEventCallback & gameEvents, const CGHeroInstance * contextHero, const std::vector<ui32> & rewardIndices, bool markAsVisit) const
{
	if (rewardIndices.empty())
		return;

	for (auto index : rewardIndices)
	{
		// TODO: Merge all rewards of same type, with single message?
		grantRewardWithMessage(gameEvents, contextHero, index, false);
	}
	// Mark visited only after all rewards were processed
	if(markAsVisit)
		markAsVisited(gameEvents, contextHero);
}

void Rewardable::Interface::doHeroVisit(IGameEventCallback & gameEvents, const CGHeroInstance *h) const
{
	if(!wasVisitedBefore(h))
	{
		auto rewards = getAvailableRewards(h, Rewardable::EEventType::EVENT_FIRST_VISIT);
		bool objectRemovalPossible = false;
		for(auto index : rewards)
		{
			if(configuration.info.at(index).reward.removeObject)
				objectRemovalPossible = true;
		}

		logGlobal->debug("Visiting object with %d possible rewards", rewards.size());
		switch (rewards.size())
		{
			case 0: // no available rewards, e.g. visiting School of War without gold
			{
				auto emptyRewards = getAvailableRewards(h, Rewardable::EEventType::EVENT_NOT_AVAILABLE);
				if (!emptyRewards.empty())
					grantRewardWithMessage(gameEvents, h, emptyRewards[0], false);
				else
					logMod->warn("No applicable message for visiting empty object!");
				break;
			}
			case 1: // one reward. Just give it with message
			{
				if (configuration.canRefuse)
					selectRewardWithMessage(gameEvents, h, rewards, configuration.info.at(rewards.front()).message);
				else
					grantRewardWithMessage(gameEvents, h, rewards.front(), true);
				break;
			}
			default: // multiple rewards. Act according to select mode
			{
				switch (configuration.selectMode) {
					case Rewardable::SELECT_PLAYER: // player must select
						selectRewardWithMessage(gameEvents, h, rewards, configuration.onSelect);
						break;
					case Rewardable::SELECT_FIRST: // give first available
						if (configuration.canRefuse)
							selectRewardWithMessage(gameEvents, h, { rewards.front() }, configuration.info.at(rewards.front()).message);
						else
							grantRewardWithMessage(gameEvents, h, rewards.front(), true);
						break;
					case Rewardable::SELECT_RANDOM: // give random
					{
						ui32 rewardIndex = *RandomGeneratorUtil::nextItem(rewards, gameEvents.getRandomGenerator());
						if (configuration.canRefuse)
							selectRewardWithMessage(gameEvents, h, { rewardIndex }, configuration.info.at(rewardIndex).message);
						else
							grantRewardWithMessage(gameEvents, h, rewardIndex, true);
						break;
					}
					case Rewardable::SELECT_ALL: // grant all possible
						grantAllRewardsWithMessage(gameEvents, h, rewards, true);
						break;
				}
				break;
			}
		}

		if(!objectRemovalPossible && getAvailableRewards(h, Rewardable::EEventType::EVENT_FIRST_VISIT).empty())
			markAsScouted(gameEvents, h);
	}
	else
	{
		logGlobal->debug("Revisiting already visited object");

		if (!wasVisited(h->getOwner()))
			markAsScouted(gameEvents, h);

		auto visitedRewards = getAvailableRewards(h, Rewardable::EEventType::EVENT_ALREADY_VISITED);
		if (!visitedRewards.empty())
			grantRewardWithMessage(gameEvents, h, visitedRewards[0], false);
		else
			logMod->warn("No applicable message for visiting already visited object!");
	}
}

void Rewardable::Interface::onBlockingDialogAnswered(IGameEventCallback & gameEvents, const CGHeroInstance * hero, int32_t answer) const
{
	if (answer == 0)
		return; //Player refused

	if(answer > 0 && answer - 1 < configuration.info.size())
	{
		auto list = getAvailableRewards(hero, Rewardable::EEventType::EVENT_FIRST_VISIT);
		markAsVisited(gameEvents, hero);
		grantReward(gameEvents, list[answer - 1], hero);
	}
	else
	{
		throw std::runtime_error("Unhandled choice");
	}
}

VCMI_LIB_NAMESPACE_END
