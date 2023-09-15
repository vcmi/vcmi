/*
 * Reward.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "Reward.h"

#include "../mapObjects/CGHeroInstance.h"
#include "../serializer/JsonSerializeFormat.h"
#include "../constants/StringConstants.h"
#include "../CSkillHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

Rewardable::Reward::Reward()
	: heroExperience(0)
	, heroLevel(0)
	, manaDiff(0)
	, manaPercentage(-1)
	, movePoints(0)
	, movePercentage(-1)
	, primary(4, 0)
	, removeObject(false)
	, spellCast(SpellID::NONE, SecSkillLevel::NONE)
{
}

Rewardable::Reward::~Reward() = default;

si32 Rewardable::Reward::calculateManaPoints(const CGHeroInstance * hero) const
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

Component Rewardable::Reward::getDisplayedComponent(const CGHeroInstance * h) const
{
	std::vector<Component> comps;
	loadComponents(comps, h);
	assert(!comps.empty());
	return comps.front();
}

void Rewardable::Reward::loadComponents(std::vector<Component> & comps,
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

void Rewardable::Reward::serializeJson(JsonSerializeFormat & handler)
{
	resources.serializeJson(handler, "resources");
	handler.serializeBool("removeObject", removeObject);
	handler.serializeInt("manaPercentage", manaPercentage);
	handler.serializeInt("movePercentage", movePercentage);
	handler.serializeInt("heroExperience", heroExperience);
	handler.serializeInt("heroLevel", heroLevel);
	handler.serializeInt("manaDiff", manaDiff);
	handler.serializeInt("manaOverflowFactor", manaOverflowFactor);
	handler.serializeInt("movePoints", movePoints);
	handler.serializeIdArray("artifacts", artifacts);
	handler.serializeIdArray("spells", spells);
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
	
	{
		auto a = handler.enterArray("creaturesChange");
		std::vector<std::pair<CreatureID, CreatureID>> fieldValue;
		if(handler.saving)
		{
			for(auto & i : creaturesChange)
				fieldValue.push_back(i);
		}
		a.syncSize(fieldValue);
		for(int i = 0; i < fieldValue.size(); ++i)
		{
			auto e = a.enterStruct(i);
			e->serializeId("creature", fieldValue[i].first, CreatureID{});
			e->serializeId("amount", fieldValue[i].second, CreatureID{});
		}
		if(!handler.saving)
		{
			for(auto & i : fieldValue)
				creaturesChange[i.first] = i.second;
		}
	}
	
	{
		auto a = handler.enterStruct("spellCast");
		a->serializeId("spell", spellCast.first, SpellID{});
		a->serializeInt("level", spellCast.second);
	}
}

VCMI_LIB_NAMESPACE_END
