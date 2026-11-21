/*
 * PandoraBoxConfig.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "PandoraBoxConfig.h"

#include "../CCreatureHandler.h"
#include "../serializer/JsonSerializeFormat.h"

bool PandoraBoxConfig::isAllowedForFaction(FactionID zoneFaction) const
{
	return zoneFactions.empty() || zoneFactions.count(zoneFaction);
}

ui16 PandoraBoxConfig::getRarity() const
{
	if(rarity)
		return *rarity;

	std::optional<ui16> lowest;
	const auto consider = [&](ui16 candidate)
	{
		if(!lowest || candidate < *lowest)
			lowest = candidate;
	};

	if(reward.heroExperience)
		consider(20);
	if(reward.resources[EGameResID::GOLD])
		consider(5);
	if(!reward.creatures.empty() || nativeCreatures)
		consider(3);
	if(randomSpells)
		consider(2);

	return lowest.value_or(2);
}

ui32 PandoraBoxConfig::getValueFromMultipliers(
	int pandoraMultiplierGold,
	int pandoraMultiplierExperience,
	int pandoraMultiplierSpells,
	int pandoraSpellSchool,
	int pandoraSpell60,
	ui32 nativeCreaturesValue) const
{
	if(value)
		return *value;

	ui32 result = 0;
	const auto gold = reward.resources[EGameResID::GOLD];
	if(gold)
		result += static_cast<ui32>(gold) * pandoraMultiplierGold / 5000;
	if(reward.heroExperience)
		result += static_cast<ui32>(reward.heroExperience) * pandoraMultiplierExperience / 5000;

	for(const auto & stack : reward.creatures)
	{
		if(const auto * creature = stack.getCreature())
			result += static_cast<ui32>(creature->getAIValue() * stack.getCount());
	}

	result += nativeCreaturesValue;

	if(randomSpells)
	{
		const auto & spells = *randomSpells;
		if(spells.level)
			result += (*spells.level + 1) * pandoraMultiplierSpells * spells.count / 12;
		else if(spells.school)
			result += pandoraSpellSchool * spells.count / 15;
		else
			result += pandoraSpell60 * spells.count / 60;
	}

	return result;
}

void PandoraBoxConfig::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeStruct("reward", reward);

	if(handler.saving)
	{
		if(randomSpells)
		{
			auto spellsStruct = handler.enterStruct("randomSpells");
			spellsStruct->serializeInt("count", randomSpells->count);
			if(randomSpells->level)
			{
				ui8 level = *randomSpells->level;
				spellsStruct->serializeInt("level", level);
			}
			if(randomSpells->school)
			{
				SpellSchool school = *randomSpells->school;
				spellsStruct->serializeId("school", school, SpellSchool::ANY);
			}
		}

		if(nativeCreatures)
		{
			auto creaturesStruct = handler.enterStruct("nativeCreatures");
			creaturesStruct->serializeInt("level", nativeCreatures->level);
			creaturesStruct->serializeInt("amount", nativeCreatures->amount);
		}

		if(value || rarity || zoneLimit != 0)
		{
			auto rmgStruct = handler.enterStruct("rmg");
			rmgStruct->serializeInt("value", value);
			rmgStruct->serializeInt("rarity", rarity);
			rmgStruct->serializeInt("zoneLimit", zoneLimit, 0);
		}
	}
	else
	{
		if(!handler.getCurrent()["randomSpells"].isNull())
		{
			RandomSpells loaded;
			auto spellsStruct = handler.enterStruct("randomSpells");
			spellsStruct->serializeInt("count", loaded.count);
			if(!handler.getCurrent()["level"].isNull())
			{
				ui8 level = 0;
				spellsStruct->serializeInt("level", level);
				loaded.level = level;
			}
			if(!handler.getCurrent()["school"].isNull())
			{
				SpellSchool school;
				spellsStruct->serializeId("school", school, SpellSchool::ANY);
				loaded.school = school;
			}
			if(loaded.count == 0)
			{
				if(loaded.level)
					loaded.count = 12;
				else if(loaded.school)
					loaded.count = 15;
				else
					loaded.count = 60;
			}
			randomSpells = loaded;
		}

		if(!handler.getCurrent()["nativeCreatures"].isNull())
		{
			NativeCreatures loaded;
			auto creaturesStruct = handler.enterStruct("nativeCreatures");
			creaturesStruct->serializeInt("level", loaded.level);
			creaturesStruct->serializeInt("amount", loaded.amount);
			nativeCreatures = loaded;
		}

		if(!handler.getCurrent()["rmg"].isNull())
		{
			auto rmgStruct = handler.enterStruct("rmg");
			rmgStruct->serializeInt("value", value);
			rmgStruct->serializeInt("rarity", rarity);
			rmgStruct->serializeInt("zoneLimit", zoneLimit, 0);
		}
	}

	handler.serializeIdArray("zoneFactions", zoneFactions);
}
