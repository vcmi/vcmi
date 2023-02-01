/*
 * CRewardableObject.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CRewardableObject.h"
#include "../CHeroHandler.h"
#include "../CGeneralTextHandler.h"
#include "../CSoundBase.h"
#include "../NetPacks.h"
#include "../IGameCallback.h"
#include "../CGameState.h"
#include "../CPlayerState.h"

#include "CObjectClassesHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

bool CRewardLimiter::heroAllowed(const CGHeroInstance * hero) const
{
	if(dayOfWeek != 0)
	{
		if (IObjectInterface::cb->getDate(Date::DAY_OF_WEEK) != dayOfWeek)
			return false;
	}

	if(daysPassed != 0)
	{
		if (IObjectInterface::cb->getDate(Date::DAY) < daysPassed)
			return false;
	}

	for(auto & reqStack : creatures)
	{
		size_t count = 0;
		for (auto slot : hero->Slots())
		{
			const CStackInstance * heroStack = slot.second;
			if (heroStack->type == reqStack.type)
				count += heroStack->count;
		}
		if (count < reqStack.count) //not enough creatures of this kind
			return false;
	}

	if(!IObjectInterface::cb->getPlayerState(hero->tempOwner)->resources.canAfford(resources))
		return false;

	if(heroLevel > (si32)hero->level)
		return false;

	if((TExpType)heroExperience > hero->exp)
		return false;

	if(manaPoints > hero->mana)
		return false;

	if(manaPercentage > 100 * hero->mana / hero->manaLimit())
		return false;

	for(size_t i=0; i<primary.size(); i++)
	{
		if (primary[i] > hero->getPrimSkillLevel(PrimarySkill::PrimarySkill(i)))
			return false;
	}

	for(auto & skill : secondary)
	{
		if (skill.second > hero->getSecSkillLevel(skill.first))
			return false;
	}

	for(auto & spell : spells)
	{
		if (!hero->spellbookContainsSpell(spell))
			return false;
	}

	for(auto & art : artifacts)
	{
		if (!hero->hasArt(art))
			return false;
	}

	for(auto & sublimiter : noneOf)
	{
		if (sublimiter->heroAllowed(hero))
			return false;
	}

	for(auto & sublimiter : allOf)
	{
		if (!sublimiter->heroAllowed(hero))
			return false;
	}

	if ( anyOf.size() == 0 )
		return true;

	for(auto & sublimiter : anyOf)
	{
		if (sublimiter->heroAllowed(hero))
			return true;
	}
	return false;
}

std::vector<ui32> CRewardableObject::getAvailableRewards(const CGHeroInstance * hero, CRewardVisitInfo::ERewardEventType event) const
{
	std::vector<ui32> ret;

	for(size_t i=0; i<info.size(); i++)
	{
		const CRewardVisitInfo & visit = info[i];

		if(event == visit.visitType && visit.limiter.heroAllowed(hero))
		{
			logGlobal->trace("Reward %d is allowed", i);
			ret.push_back(static_cast<ui32>(i));
		}
	}
	return ret;
}

void CRewardableObject::onHeroVisit(const CGHeroInstance *h) const
{
	auto grantRewardWithMessage = [&](int index, bool markAsVisit) -> void
	{
		auto vi = info[index];
		logGlobal->debug("Granting reward %d. Message says: %s", index, vi.message.toString());
		// show message only if it is not empty
		if (!vi.message.toString().empty())
		{
			InfoWindow iw;
			iw.player = h->tempOwner;
			iw.text = vi.message;
			vi.reward.loadComponents(iw.components, h);
			cb->showInfoDialog(&iw);
		}
		// grant reward afterwards. Note that it may remove object
		grantReward(index, h, markAsVisit);
	};
	auto selectRewardsMessage = [&](std::vector<ui32> rewards, const MetaString & dialog) -> void
	{
		BlockingDialog sd(canRefuse, rewards.size() > 1);
		sd.player = h->tempOwner;
		sd.text = dialog;
		for (auto index : rewards)
			sd.components.push_back(info[index].reward.getDisplayedComponent(h));
		cb->showBlockingDialog(&sd);
	};

	if(!wasVisitedBefore(h))
	{
		auto rewards = getAvailableRewards(h, CRewardVisitInfo::EVENT_FIRST_VISIT);
		bool objectRemovalPossible = false;
		for(auto index : rewards)
		{
			if(info[index].reward.removeObject)
				objectRemovalPossible = true;
		}

		logGlobal->debug("Visiting object with %d possible rewards", rewards.size());
		switch (rewards.size())
		{
			case 0: // no available rewards, e.g. visiting School of War without gold
			{
				auto emptyRewards = getAvailableRewards(h, CRewardVisitInfo::EVENT_NOT_AVAILABLE);
				if (!emptyRewards.empty())
					grantRewardWithMessage(emptyRewards[0], false);
				else
					logMod->warn("No applicable message for visiting empty object!");
				break;
			}
			case 1: // one reward. Just give it with message
			{
				if (canRefuse)
					selectRewardsMessage(rewards, info[rewards[0]].message);
				else
					grantRewardWithMessage(rewards[0], true);
				break;
			}
			default: // multiple rewards. Act according to select mode
			{
				switch (selectMode) {
					case SELECT_PLAYER: // player must select
						selectRewardsMessage(rewards, onSelect);
						break;
					case SELECT_FIRST: // give first available
						grantRewardWithMessage(rewards[0], true);
						break;
				}
				break;
			}
		}

		if(!objectRemovalPossible && getAvailableRewards(h, CRewardVisitInfo::EVENT_FIRST_VISIT).size() == 0)
		{
			ChangeObjectVisitors cov(ChangeObjectVisitors::VISITOR_ADD_TEAM, id, h->id);
			cb->sendAndApply(&cov);
		}
	}
	else
	{
		logGlobal->debug("Revisiting already visited object");

		auto visitedRewards = getAvailableRewards(h, CRewardVisitInfo::EVENT_ALREADY_VISITED);
		if (!visitedRewards.empty())
			grantRewardWithMessage(visitedRewards[0], false);
		else
			logMod->warn("No applicable message for visiting already visited object!");
	}
}

void CRewardableObject::heroLevelUpDone(const CGHeroInstance *hero) const
{
	grantRewardAfterLevelup(info[selectedReward], hero);
}

void CRewardableObject::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const
{
	if(answer == 0)
		return; // player refused

	if(answer > 0 && answer-1 < info.size())
	{
		auto list = getAvailableRewards(hero, CRewardVisitInfo::EVENT_FIRST_VISIT);
		grantReward(list[answer - 1], hero, true);
	}
	else
	{
		throw std::runtime_error("Unhandled choice");
	}
}

void CRewardableObject::grantReward(ui32 rewardID, const CGHeroInstance * hero, bool markVisited) const
{
	if (markVisited)
	{
		cb->setObjProperty(id, ObjProperty::REWARD_CLEARED, true);

		ChangeObjectVisitors cov(ChangeObjectVisitors::VISITOR_ADD, id, hero->id);
		cb->sendAndApply(&cov);
	}

	cb->setObjProperty(id, ObjProperty::REWARD_SELECT, rewardID);
	grantRewardBeforeLevelup(info[rewardID], hero);
}

void CRewardableObject::grantRewardBeforeLevelup(const CRewardVisitInfo & info, const CGHeroInstance * hero) const
{
	assert(hero);
	assert(hero->tempOwner.isValidPlayer());
	assert(stacks.empty());
	assert(info.reward.creatures.size() <= GameConstants::ARMY_SIZE);
	assert(!cb->isVisitCoveredByAnotherQuery(this, hero));

	cb->giveResources(hero->tempOwner, info.reward.resources);

	for(auto & entry : info.reward.secondary)
	{
		int current = hero->getSecSkillLevel(entry.first);
		if( (current != 0 && current < entry.second) ||
			(hero->canLearnSkill() ))
		{
			cb->changeSecSkill(hero, entry.first, entry.second);
		}
	}

	for(int i=0; i< info.reward.primary.size(); i++)
		cb->changePrimSkill(hero, static_cast<PrimarySkill::PrimarySkill>(i), info.reward.primary[i], false);

	si64 expToGive = 0;

	if (info.reward.heroLevel > 0)
		expToGive += VLC->heroh->reqExp(hero->level+info.reward.heroLevel) - VLC->heroh->reqExp(hero->level);

	if (info.reward.heroExperience > 0)
		expToGive += hero->calculateXp(info.reward.heroExperience);

	if(expToGive)
		cb->changePrimSkill(hero, PrimarySkill::EXPERIENCE, expToGive);

	// hero is not blocked by levelup dialog - grant remainer immediately
	if(!cb->isVisitCoveredByAnotherQuery(this, hero))
	{
		grantRewardAfterLevelup(info, hero);
	}
}

void CRewardableObject::grantRewardAfterLevelup(const CRewardVisitInfo & info, const CGHeroInstance * hero) const
{
	if(info.reward.manaDiff || info.reward.manaPercentage >= 0)
	{
		si32 manaScaled = hero->mana;
		if (info.reward.manaPercentage >= 0)
			manaScaled = hero->manaLimit() * info.reward.manaPercentage / 100;

		si32 manaMissing   = std::max(0, hero->manaLimit() - manaScaled);
		si32 manaGranted   = std::min(manaMissing, info.reward.manaDiff);
		si32 manaOverflow  = info.reward.manaDiff - manaGranted;
		si32 manaOverLimit = manaOverflow * info.reward.manaOverflowFactor / 100;
		si32 manaOutput    = manaScaled + manaGranted + manaOverLimit;

		cb->setManaPoints(hero->id, manaOutput);
	}

	if(info.reward.movePoints || info.reward.movePercentage >= 0)
	{
		SetMovePoints smp;
		smp.hid = hero->id;
		smp.val = hero->movement;

		if (info.reward.movePercentage >= 0) // percent from max
			smp.val = hero->maxMovePoints(hero->boat != nullptr) * info.reward.movePercentage / 100;
		smp.val = std::max<si32>(0, smp.val + info.reward.movePoints);

		cb->setMovePoints(&smp);
	}

	for(const Bonus & bonus : info.reward.bonuses)
	{
		assert(bonus.source == Bonus::OBJECT);
		assert(bonus.sid == ID);
		GiveBonus gb;
		gb.who = GiveBonus::HERO;
		gb.bonus = bonus;
		gb.id = hero->id.getNum();
		cb->giveHeroBonus(&gb);
	}

	for(ArtifactID art : info.reward.artifacts)
		cb->giveHeroNewArtifact(hero, VLC->arth->objects[art],ArtifactPosition::FIRST_AVAILABLE);

	if(!info.reward.spells.empty())
	{
		std::set<SpellID> spellsToGive(info.reward.spells.begin(), info.reward.spells.end());
		cb->changeSpells(hero, true, spellsToGive);
	}

	if(!info.reward.creaturesChange.empty())
	{
		for (auto slot : hero->Slots())
		{
			const CStackInstance * heroStack = slot.second;

			for (auto & change : info.reward.creaturesChange)
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
		for (auto & crea : info.reward.creatures)
			creatures.addToSlot(creatures.getFreeSlot(), new CStackInstance(crea.type, crea.count));

		cb->giveCreatures(this, hero, creatures, false);
	}

	if(info.reward.removeObject)
		cb->removeObject(this);
}

bool CRewardableObject::wasVisitedBefore(const CGHeroInstance * contextHero) const
{
	switch (visitMode)
	{
		case VISIT_UNLIMITED:
			return false;
		case VISIT_ONCE:
			return onceVisitableObjectCleared;
		case VISIT_PLAYER:
			return vstd::contains(cb->getPlayerState(contextHero->getOwner())->visitedObjects, ObjectInstanceID(id));
		case VISIT_BONUS:
			return contextHero->hasBonusFrom(Bonus::OBJECT, ID);
		case VISIT_HERO:
			return contextHero->visitedObjects.count(ObjectInstanceID(id));
		default:
			return false;
	}

}

bool CRewardableObject::wasVisited(PlayerColor player) const
{
	switch (visitMode)
	{
		case VISIT_UNLIMITED:
		case VISIT_BONUS:
		case VISIT_HERO:
			return false;
		case VISIT_ONCE:
		case VISIT_PLAYER:
			return vstd::contains(cb->getPlayerState(player)->visitedObjects, ObjectInstanceID(id));
		default:
			return false;
	}
}

bool CRewardableObject::wasVisited(const CGHeroInstance * h) const
{
	switch (visitMode)
	{
		case VISIT_BONUS:
			return h->hasBonusFrom(Bonus::OBJECT, ID);
		case VISIT_HERO:
			return h->visitedObjects.count(ObjectInstanceID(id));
		default:
			return wasVisited(h->tempOwner);
	}
}

CRewardableObject::EVisitMode CRewardableObject::getVisitMode() const
{
	return EVisitMode(visitMode);
}

ui16 CRewardableObject::getResetDuration() const
{
	return resetParameters.period;
}

void CRewardInfo::loadComponents(std::vector<Component> & comps,
                                 const CGHeroInstance * h) const
{
	for (auto comp : extraComponents)
		comps.push_back(comp);

	if (heroExperience)
	{
		comps.push_back(Component(
			Component::EXPERIENCE, 0, (si32)h->calculateXp(heroExperience), 0));
	}
	if (heroLevel)
		comps.push_back(Component(Component::EXPERIENCE, 1, heroLevel, 0));

	if (manaDiff || manaPercentage >= 0)
		comps.push_back(Component(Component::PRIM_SKILL, 5, manaDiff, 0));

	for (size_t i=0; i<primary.size(); i++)
	{
		if (primary[i] != 0)
			comps.push_back(Component(Component::PRIM_SKILL, (ui16)i, primary[i], 0));
	}

	for (auto & entry : secondary)
		comps.push_back(Component(Component::SEC_SKILL, entry.first, entry.second, 0));

	for (auto & entry : artifacts)
		comps.push_back(Component(Component::ARTIFACT, entry, 1, 0));

	for (auto & entry : spells)
		comps.push_back(Component(Component::SPELL, entry, 1, 0));

	for (auto & entry : creatures)
		comps.push_back(Component(Component::CREATURE, entry.type->idNumber, entry.count, 0));

	for (size_t i=0; i<resources.size(); i++)
	{
		if (resources[i] !=0)
			comps.push_back(Component(Component::RESOURCE, (ui16)i, resources[i], 0));
	}
}

Component CRewardInfo::getDisplayedComponent(const CGHeroInstance * h) const
{
	std::vector<Component> comps;
	loadComponents(comps, h);
	assert(!comps.empty());
	return comps.front();
}

// FIXME: copy-pasted from CObjectHandler
static std::string visitedTxt(const bool visited)
{
	int id = visited ? 352 : 353;
	return VLC->generaltexth->allTexts[id];
}

std::string CRewardableObject::getHoverText(PlayerColor player) const
{
	if(visitMode == VISIT_PLAYER || visitMode == VISIT_ONCE)
		return getObjectName() + " " + visitedTxt(wasVisited(player));
	return getObjectName();
}

std::string CRewardableObject::getHoverText(const CGHeroInstance * hero) const
{
	if(visitMode != VISIT_UNLIMITED)
		return getObjectName() + " " + visitedTxt(wasVisited(hero));
	return getObjectName();
}

void CRewardableObject::setPropertyDer(ui8 what, ui32 val)
{
	switch (what)
	{
		case ObjProperty::REWARD_RANDOMIZE:
			initObj(cb->gameState()->getRandomGenerator());
			break;
		case ObjProperty::REWARD_SELECT:
			selectedReward = val;
			break;
		case ObjProperty::REWARD_CLEARED:
			onceVisitableObjectCleared = val;
			break;
	}
}

void CRewardableObject::triggerReset() const
{
	if (resetParameters.rewards)
	{
		cb->setObjProperty(id, ObjProperty::REWARD_RANDOMIZE, 0);
	}
	if (resetParameters.visitors)
	{
		cb->setObjProperty(id, ObjProperty::REWARD_CLEARED, false);

		ChangeObjectVisitors cov(ChangeObjectVisitors::VISITOR_CLEAR, id);
		cb->sendAndApply(&cov);
	}
}

void CRewardableObject::newTurn(CRandomGenerator & rand) const
{
	if (resetParameters.period != 0 && cb->getDate(Date::DAY) > 1 && ((cb->getDate(Date::DAY)-1) % resetParameters.period) == 0)
		triggerReset();
}

void CRewardableObject::initObj(CRandomGenerator & rand)
{
	VLC->objtypeh->getHandlerFor(ID, subID)->configureObject(this, rand);
}

CRewardableObject::CRewardableObject():
	selectMode(0),
	visitMode(0),
	selectedReward(0),
	onceVisitableObjectCleared(false),
	canRefuse(false)
{}

VCMI_LIB_NAMESPACE_END
