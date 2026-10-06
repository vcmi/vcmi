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
#include "../json/JsonNode.h"
#include "../spells/ISpellMechanics.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/MiscObjects.h"
#include "../mapping/TerrainTile.h"
#include "../networkPacks/StackLocation.h"
#include "../networkPacks/PacksForClient.h"

#include <vstd/RNG.h>

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

bool Rewardable::Interface::grantRewardBeforeLevelup(IGameEventCallback & gameEvents, const Rewardable::VisitInfo & info, const CGHeroInstance * hero) const
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

		FowTilesType tiles;
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
		bool canLearn = hero->getSecSkillLevel(entry.first) != 0 || hero->canLearnSkill();

		if(currentLevel != newLevelClamped && canLearn)
			gameEvents.changeSecSkill(hero, entry.first, newLevelClamped, ChangeValueMode::ABSOLUTE);
	}

	for(int i=0; i< info.reward.primary.size(); i++)
		if (info.reward.primary[i] != 0)
			gameEvents.changePrimSkill(hero, static_cast<PrimarySkill>(i), info.reward.primary[i], ChangeValueMode::RELATIVE);

	TExpType expToGive = 0;

	if (info.reward.heroLevel > 0)
		expToGive += hero->experienceToGainLevels(info.reward.heroLevel);

	if (info.reward.heroExperience > 0)
		expToGive += hero->calculateXp(info.reward.heroExperience);

	// A skill that grants a level also suspends the visit, but its reward is complete by now
	// and resumeAfterExperience() is then handed no state
	if(!expToGive)
		return false;

	gameEvents.giveExperience(hero, expToGive);
	return true;
}

bool Rewardable::Interface::grantRewardAfterLevelup(IGameEventCallback & gameEvents, const Rewardable::VisitInfo & info, const CGHeroInstance * hero, const std::vector<ui32> & pending) const
{
	auto cb = getObject()->cb;

	if(info.reward.manaDiff || info.reward.manaPercentage >= 0)
		gameEvents.setManaPoints(hero->id, info.reward.calculateManaPoints(hero));

	if(info.reward.movePoints != 0 || info.reward.movePercentage >= 0)
		gameEvents.setMovePoints(hero->id, info.reward.calculateMovePoints(hero));

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

	bool openedGarrison = false;

	if(!info.reward.creatures.empty())
	{
		CCreatureSet creatures;
		for(const auto & crea : info.reward.creatures)
			creatures.addToSlot(creatures.getFreeSlot(), std::make_unique<CStackInstance>(cb, crea.getId(), crea.getCount()));

		auto * army = dynamic_cast<const CArmedInstance*>(this);
		if (army)
		{
			// Same check that decides between a silent merge and a garrison window on joining
			openedGarrison = !hero->canBeMergedWith(creatures, true);
			gameEvents.giveCreatures(army, hero, creatures, false);
		}
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

	if(openedGarrison && !pending.empty())
		gameEvents.setVisitState(hero, toJson(pending));
	return openedGarrison;
}

bool Rewardable::Interface::grantReward(IGameEventCallback & gameEvents, ui32 rewardID, const CGHeroInstance * hero, const std::vector<ui32> & pending) const
{
	if(!grantRewardBeforeLevelup(gameEvents, configuration.info.at(rewardID), hero))
		return grantRewardAfterLevelup(gameEvents, configuration.info.at(rewardID), hero, pending);

	// Stored only now that the visit is known to be suspended - a visit that finishes inline
	// must not leave a state behind. The level-up routine can not have finished yet: it is
	// stepped once control returns to the activity processor.
	std::vector<ui32> state = {rewardID};
	state.insert(state.end(), pending.begin(), pending.end());
	gameEvents.setVisitState(hero, toJson(state));
	return true;
}

bool Rewardable::Interface::isRewardIndex(const JsonNode & node) const
{
	return node.isNumber() && node.Integer() >= 0 && node.Integer() < static_cast<si64>(configuration.info.size());
}

bool Rewardable::Interface::isRewardList(const JsonNode & node) const
{
	return node.isVector() && std::ranges::all_of(node.Vector(), [this](const JsonNode & entry){ return isRewardIndex(entry); });
}

JsonNode Rewardable::Interface::toJson(const std::vector<ui32> & rewardIndices)
{
	JsonNode result;
	for(ui32 index : rewardIndices)
		result.Vector().emplace_back(index);
	return result;
}

bool Rewardable::Interface::resumeAfterExperience(IGameEventCallback & gameEvents, const CGHeroInstance * hero, const JsonNode & visitState) const
{
	// Level gained from a learned skill, e.g. HotA Learning, and not from experience of a reward
	if(visitState.isNull())
		return false;

	if(!isRewardList(visitState) || visitState.Vector().empty())
		throw std::runtime_error("Object at " + getObject()->visitablePos().toString() + " can not resume its visit from state " + visitState.toCompactString());

	auto rewards = visitState.convertTo<std::vector<ui32>>();
	std::vector<ui32> pending(rewards.begin() + 1, rewards.end());
	return grantRewardAfterLevelup(gameEvents, configuration.info.at(rewards.front()), hero, pending) || grantRewardsWithMessage(gameEvents, hero, pending);
}

bool Rewardable::Interface::resumeAfterGarrison(IGameEventCallback & gameEvents, const CGHeroInstance * hero, const JsonNode & visitState) const
{
	// A garrison window that closes the last reward leaves nothing pending
	if(visitState.isNull())
		return false;

	if(!isRewardList(visitState))
		throw std::runtime_error("Object at " + getObject()->visitablePos().toString() + " can not resume its visit from state " + visitState.toCompactString());

	return grantRewardsWithMessage(gameEvents, hero, visitState.convertTo<std::vector<ui32>>());
}

void Rewardable::Interface::serializeJson(JsonSerializeFormat & handler)
{
	configuration.serializeJson(handler);
}

bool Rewardable::Interface::grantRewardWithMessage(IGameEventCallback & gameEvents, const CGHeroInstance * contextHero, int index, bool markAsVisit) const
{
	showRewardMessage(gameEvents, contextHero, index);

	// grant reward afterwards. Note that it may remove object
	if(markAsVisit)
		markAsVisited(gameEvents, contextHero);
	return grantReward(gameEvents, index, contextHero);
}

void Rewardable::Interface::showRewardMessage(IGameEventCallback & gameEvents, const CGHeroInstance * contextHero, int index) const
{
	auto vi = configuration.info.at(index);
	logGlobal->debug("Granting reward %d", index);

	InfoWindow iw;
	iw.player = contextHero->tempOwner;
	iw.text = vi.message;
	vi.reward.loadComponents(iw.components, contextHero);
	iw.type = configuration.infoWindowType;
	configureInfoWindow(iw, contextHero, index);
	gameEvents.showInfoDialog(&iw);
}

void Rewardable::Interface::configureInfoWindow(InfoWindow &, const CGHeroInstance *, int) const
{
}

void Rewardable::Interface::selectRewardWithMessage(IGameEventCallback & gameEvents, const CGHeroInstance * contextHero, const std::vector<ui32> & rewardIndices, const MetaString & dialog) const
{
	BlockingDialog sd(configuration.canRefuse, rewardIndices.size() > 1);
	sd.player = contextHero->tempOwner;
	sd.text = dialog;
	sd.components = loadComponents(contextHero, rewardIndices);
	gameEvents.showBlockingDialog(&sd);

	// The answer picks from what was offered, which may be a random subset of what is available
	gameEvents.setVisitState(contextHero, toJson(rewardIndices));
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

bool Rewardable::Interface::grantRewardsWithMessage(IGameEventCallback & gameEvents, const CGHeroInstance * contextHero, const std::vector<ui32> & rewardIndices) const
{
	for(size_t i = 0; i < rewardIndices.size(); ++i)
	{
		// TODO: Merge all rewards of same type, with single message?
		showRewardMessage(gameEvents, contextHero, rewardIndices[i]);
		if(grantReward(gameEvents, rewardIndices[i], contextHero, std::vector<ui32>(rewardIndices.begin() + i + 1, rewardIndices.end())))
			return true;
	}
	return false;
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
						// Marked up front, since the rewards may be granted across several suspensions
						markAsVisited(gameEvents, h);
						grantRewardsWithMessage(gameEvents, h, rewards);
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

bool Rewardable::Interface::onBlockingDialogAnswered(IGameEventCallback & gameEvents, const CGHeroInstance * hero, int32_t answer, const JsonNode & offeredRewards) const
{
	if (answer == 0)
		return false; //Player refused

	if(!isRewardList(offeredRewards))
		throw std::runtime_error("Object at " + getObject()->visitablePos().toString() + " offered invalid rewards " + offeredRewards.toCompactString());

	// The dialog checks the answer against what it showed, so a mismatch is ours
	if(answer < 0 || answer > static_cast<int32_t>(offeredRewards.Vector().size()))
		throw std::runtime_error("Object at " + getObject()->visitablePos().toString() + " got answer " + std::to_string(answer) + " to a reward choice that offered " + offeredRewards.toCompactString());

	markAsVisited(gameEvents, hero);
	return grantReward(gameEvents, offeredRewards.Vector().at(answer - 1).Integer(), hero);
}
