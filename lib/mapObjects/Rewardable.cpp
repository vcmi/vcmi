/*
 * Rewardable.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "Rewardable.h"

#include "../CHeroHandler.h"
#include "../CSoundBase.h"
#include "../NetPacks.h"
#include "../IGameCallback.h"
#include "../CPlayerState.h"
#include "../spells/CSpellHandler.h"
#include "../spells/ISpellMechanics.h"
#include "../mapObjects/MiscObjects.h"

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

	for(const auto & reqStack : creatures)
	{
		size_t count = 0;
		for(const auto & slot : hero->Slots())
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

	if(heroLevel > static_cast<si32>(hero->level))
		return false;

	if(static_cast<TExpType>(heroExperience) > hero->exp)
		return false;

	if(manaPoints > hero->mana)
		return false;

	if(manaPercentage > 100 * hero->mana / hero->manaLimit())
		return false;

	for(size_t i=0; i<primary.size(); i++)
	{
		if(primary[i] > hero->getPrimSkillLevel(static_cast<PrimarySkill::PrimarySkill>(i)))
			return false;
	}

	for(const auto & skill : secondary)
	{
		if (skill.second > hero->getSecSkillLevel(skill.first))
			return false;
	}

	for(const auto & spell : spells)
	{
		if (!hero->spellbookContainsSpell(spell))
			return false;
	}

	for(const auto & art : artifacts)
	{
		if (!hero->hasArt(art))
			return false;
	}

	for(const auto & sublimiter : noneOf)
	{
		if (sublimiter->heroAllowed(hero))
			return false;
	}

	for(const auto & sublimiter : allOf)
	{
		if (!sublimiter->heroAllowed(hero))
			return false;
	}

	if(anyOf.empty())
		return true;

	for(const auto & sublimiter : anyOf)
	{
		if (sublimiter->heroAllowed(hero))
			return true;
	}
	return false;
}

si32 CRewardInfo::calculateManaPoints(const CGHeroInstance * hero) const
{
	si32 manaScaled = hero->mana;
	if (manaPercentage >= 0)
		manaScaled = hero->manaLimit() * manaPercentage / 100;

	si32 manaMissing   = std::max(0, hero->manaLimit() - manaScaled);
	si32 manaGranted   = std::min(manaMissing, manaDiff);
	si32 manaOverflow  = manaDiff - manaGranted;
	si32 manaOverLimit = manaOverflow * manaOverflowFactor / 100;
	si32 manaOutput    = manaScaled + manaGranted + manaOverLimit;

	return manaOutput;
}

Component CRewardInfo::getDisplayedComponent(const CGHeroInstance * h) const
{
	std::vector<Component> comps;
	loadComponents(comps, h);
	assert(!comps.empty());
	return comps.front();
}

void CRewardInfo::loadComponents(std::vector<Component> & comps,
								 const CGHeroInstance * h) const
{
	for (auto comp : extraComponents)
		comps.push_back(comp);

	if (heroExperience)
	{
		comps.emplace_back(Component::EComponentType::EXPERIENCE, 0, static_cast<si32>(h->calculateXp(heroExperience)), 0);
	}
	if (heroLevel)
		comps.emplace_back(Component::EComponentType::EXPERIENCE, 1, heroLevel, 0);

	if (manaDiff || manaPercentage >= 0)
		comps.emplace_back(Component::EComponentType::PRIM_SKILL, 5, calculateManaPoints(h) - h->mana, 0);

	for (size_t i=0; i<primary.size(); i++)
	{
		if (primary[i] != 0)
			comps.emplace_back(Component::EComponentType::PRIM_SKILL, static_cast<ui16>(i), primary[i], 0);
	}

	for(const auto & entry : secondary)
		comps.emplace_back(Component::EComponentType::SEC_SKILL, entry.first, entry.second, 0);

	for(const auto & entry : artifacts)
		comps.emplace_back(Component::EComponentType::ARTIFACT, entry, 1, 0);

	for(const auto & entry : spells)
		comps.emplace_back(Component::EComponentType::SPELL, entry, 1, 0);

	for(const auto & entry : creatures)
		comps.emplace_back(Component::EComponentType::CREATURE, entry.type->getId(), entry.count, 0);

	for (size_t i=0; i<resources.size(); i++)
	{
		if (resources[i] !=0)
			comps.emplace_back(Component::EComponentType::RESOURCE, static_cast<ui16>(i), resources[i], 0);
	}
}

std::vector<ui32> Rewardable::Interface::getAvailableRewards(const CGHeroInstance * hero, CRewardVisitInfo::ERewardEventType event) const
{
	std::vector<ui32> ret;

	for(size_t i = 0; i < _configuration.info.size(); i++)
	{
		const CRewardVisitInfo & visit = _configuration.info[i];

		if(event == visit.visitType && visit.limiter.heroAllowed(hero))
		{
			logGlobal->trace("Reward %d is allowed", i);
			ret.push_back(static_cast<ui32>(i));
		}
	}
	return ret;
}

Rewardable::EVisitMode Rewardable::Configuration::getVisitMode() const
{
	return static_cast<EVisitMode>(visitMode);
}

ui16 Rewardable::Configuration::getResetDuration() const
{
	return resetParameters.period;
}

const Rewardable::Configuration & Rewardable::Interface::getConfiguration() const
{
	return _configuration;
}

Rewardable::Configuration & Rewardable::Interface::configuration()
{
	return _configuration;
}

void Rewardable::Interface::grantRewardBeforeLevelup(IGameCallback * cb, const CRewardVisitInfo & info, const CGHeroInstance * hero) const
{
	assert(hero);
	assert(hero->tempOwner.isValidPlayer());
	assert(info.reward.creatures.size() <= GameConstants::ARMY_SIZE);

	cb->giveResources(hero->tempOwner, info.reward.resources);

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
		cb->changePrimSkill(hero, static_cast<PrimarySkill::PrimarySkill>(i), info.reward.primary[i], false);

	si64 expToGive = 0;

	if (info.reward.heroLevel > 0)
		expToGive += VLC->heroh->reqExp(hero->level+info.reward.heroLevel) - VLC->heroh->reqExp(hero->level);

	if (info.reward.heroExperience > 0)
		expToGive += hero->calculateXp(info.reward.heroExperience);

	if(expToGive)
		cb->changePrimSkill(hero, PrimarySkill::EXPERIENCE, expToGive);
}

void Rewardable::Interface::grantRewardAfterLevelup(IGameCallback * cb, const CRewardVisitInfo & info, const CArmedInstance * army, const CGHeroInstance * hero) const
{
	if(info.reward.manaDiff || info.reward.manaPercentage >= 0)
		cb->setManaPoints(hero->id, info.reward.calculateManaPoints(hero));

	if(info.reward.movePoints || info.reward.movePercentage >= 0)
	{
		SetMovePoints smp;
		smp.hid = hero->id;
		smp.val = hero->movement;

		if (info.reward.movePercentage >= 0) // percent from max
			smp.val = hero->maxMovePoints(hero->boat && hero->boat->layer == EPathfindingLayer::SAIL) * info.reward.movePercentage / 100;
		smp.val = std::max<si32>(0, smp.val + info.reward.movePoints);

		cb->setMovePoints(&smp);
	}

	for(const Bonus & bonus : info.reward.bonuses)
	{
		assert(bonus.source == Bonus::OBJECT);
		GiveBonus gb;
		gb.who = GiveBonus::ETarget::HERO;
		gb.bonus = bonus;
		gb.id = hero->id.getNum();
		cb->giveHeroBonus(&gb);
	}

	for(const ArtifactID & art : info.reward.artifacts)
		cb->giveHeroNewArtifact(hero, VLC->arth->objects[art],ArtifactPosition::FIRST_AVAILABLE);

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
		
		if(info.reward.removeObject)
			logMod->warn("Removal of object with spell casts is not supported!");
	}
	else if(info.reward.removeObject) //FIXME: object can't track spell cancel or finish, so removeObject leads to crash
		if(auto * instance = dynamic_cast<const CGObjectInstance*>(this))
			cb->removeObject(instance);
}

VCMI_LIB_NAMESPACE_END
