/*
 * Limiter.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "Limiter.h"

#include "../IGameCallback.h"
#include "../CPlayerState.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../serializer/JsonSerializeFormat.h"
#include "../constants/StringConstants.h"
#include "../CSkillHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

Rewardable::Limiter::Limiter()
	: dayOfWeek(0)
	, daysPassed(0)
	, heroExperience(0)
	, heroLevel(0)
	, manaPercentage(0)
	, manaPoints(0)
	, primary(GameConstants::PRIMARY_SKILLS, 0)
{
}

Rewardable::Limiter::~Limiter() = default;

bool Rewardable::Limiter::heroAllowed(const CGHeroInstance * hero) const
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
		if(primary[i] > hero->getPrimSkillLevel(static_cast<PrimarySkill>(i)))
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

void Rewardable::Limiter::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeInt("dayOfWeek", dayOfWeek);
	handler.serializeInt("daysPassed", daysPassed);
	resources.serializeJson(handler, "resources");
	handler.serializeInt("manaPercentage", manaPercentage);
	handler.serializeInt("heroExperience", heroExperience);
	handler.serializeInt("heroLevel", heroLevel);
	handler.serializeInt("manaPoints", manaPoints);
	handler.serializeIdArray("artifacts", artifacts);
	handler.enterArray("creatures").serializeStruct(creatures);
	{
		auto a = handler.enterArray("primary");
		a.syncSize(primary);
		for(int i = 0; i < primary.size(); ++i)
			a.serializeInt(i, primary[i]);
	}
	
	{
		auto a = handler.enterArray("secondary");
		std::vector<std::pair<std::string, std::string>> fieldValue;
		if(handler.saving)
		{
			for(auto & i : secondary)
			{
				auto key = VLC->skillh->encodeSkill(i.first);
				auto value = NSecondarySkill::levels.at(i.second);
				fieldValue.emplace_back(key, value);
			}
		}
		a.syncSize(fieldValue);
		for(int i = 0; i < fieldValue.size(); ++i)
		{
			auto e = a.enterStruct(i);
			e->serializeString("skill", fieldValue[i].first);
			e->serializeString("level", fieldValue[i].second);
		}
		if(!handler.saving)
		{
			for(auto & i : fieldValue)
			{
				const int skillId = VLC->skillh->decodeSkill(i.first);
				if(skillId < 0)
				{
					logGlobal->error("Invalid secondary skill %s", i.first);
					continue;
				}
				
				const int level = vstd::find_pos(NSecondarySkill::levels, i.second);
				if(level < 0)
				{
					logGlobal->error("Invalid secondary skill level%s", i.second);
					continue;
				}
				
				secondary[SecondarySkill(skillId)] = level;
			}
				
		}
	}
	
	//sublimiters
	auto serializeSublimitersList = [&handler](const std::string & field, LimitersList & container)
	{
		auto a = handler.enterArray(field);
		a.syncSize(container);
		for(int i = 0; i < container.size(); ++i)
		{
			if(!handler.saving)
				container[i] = std::make_shared<Rewardable::Limiter>();
			auto e = a.enterStruct(i);
			container[i]->serializeJson(handler);
		}
	};
	serializeSublimitersList("allOf", allOf);
	serializeSublimitersList("anyOf", anyOf);
	serializeSublimitersList("noneOf", noneOf);
}

VCMI_LIB_NAMESPACE_END
