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
#include "../constants/StringConstants.h"
#include "../modding/IdentifierResolver.h"
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

void PandoraBoxConfig::loadReward(JsonSerializeFormat & handler)
{
	auto rewardStruct = handler.enterStruct("reward");

	reward.resources.resolveFromJson(handler.getCurrent()["resources"], handler.getIdentifierResolver());
	handler.serializeBool("removeObject", reward.removeObject);
	handler.serializeInt("manaPercentage", reward.manaPercentage);
	handler.serializeInt("movePercentage", reward.movePercentage);
	handler.serializeInt("heroExperience", reward.heroExperience);
	handler.serializeInt("heroLevel", reward.heroLevel);
	handler.serializeInt("manaDiff", reward.manaDiff);
	handler.serializeInt("manaOverflowFactor", reward.manaOverflowFactor);
	handler.serializeInt("movePoints", reward.movePoints);
	handler.serializeInt("moveOverflowFactor", reward.moveOverflowFactor);
	handler.serializeIdArray("artifacts", reward.grantedArtifacts);
	handler.serializeIdArray("takenArtifacts", reward.takenArtifacts);
	handler.serializeIdArray("takenArtifactSlots", reward.takenArtifactSlots);
	handler.serializeIdArray("scrolls", reward.grantedScrolls);
	handler.serializeIdArray("takenScrolls", reward.takenScrolls);
	handler.serializeIdArray("spells", reward.spells);

	const JsonNode & creaturesNode = handler.getCurrent()["creatures"];
	if(creaturesNode.isVector() && !creaturesNode.Vector().empty())
	{
		reward.creatures.resize(creaturesNode.Vector().size());
		auto creaturesArray = handler.enterArray("creatures");
		for(size_t i = 0; i < reward.creatures.size(); ++i)
		{
			auto creatureStruct = creaturesArray.enterStruct(i);
			si32 amount = 0;
			creatureStruct->serializeInt("amount", amount);
			const JsonNode & typeNode = handler.getCurrent()["type"];
			if(!typeNode.String().empty())
			{
				handler.getIdentifierResolver().requestIdentifier("creature", typeNode, [this, i, amount](int32_t index)
				{
					reward.creatures[i] = CStackBasicDescriptor(CreatureID(index), amount);
				});
			}
			else
				reward.creatures[i].setCount(amount);
		}
	}

	if(handler.getCurrent()["primary"].isVector())
		handler.enterArray("primary").serializeArray(reward.primary);

	{
		auto secondaryArray = handler.enterArray("secondary");
		std::vector<std::pair<SecondarySkill, si32>> fieldValue(reward.secondary.begin(), reward.secondary.end());
		secondaryArray.serializeStruct<std::pair<SecondarySkill, si32>>(fieldValue, [](JsonSerializeFormat & h, std::pair<SecondarySkill, si32> & e)
		{
			h.serializeId("skill", e.first);
			h.serializeId("level", e.second, 0, [](const std::string & i){return vstd::find_pos(NSecondarySkill::levels, i);}, [](si32 i){return NSecondarySkill::levels.at(i);});
		});
		secondaryArray.syncSize(fieldValue);
		reward.secondary = std::map<SecondarySkill, si32>(fieldValue.begin(), fieldValue.end());
	}

	{
		auto changeArray = handler.enterArray("creaturesChange");
		std::vector<std::pair<CreatureID, CreatureID>> fieldValue(reward.creaturesChange.begin(), reward.creaturesChange.end());
		changeArray.serializeStruct<std::pair<CreatureID, CreatureID>>(fieldValue, [](JsonSerializeFormat & h, std::pair<CreatureID, CreatureID> & e)
		{
			h.serializeId("creature", e.first, CreatureID{});
			h.serializeId("amount", e.second, CreatureID{});
		});
		reward.creaturesChange = std::map<CreatureID, CreatureID>(fieldValue.begin(), fieldValue.end());
	}

	{
		auto spellCastStruct = handler.enterStruct("spellCast");
		spellCastStruct->serializeId("spell", reward.spellCast.first, SpellID{});
		spellCastStruct->serializeInt("level", reward.spellCast.second);
	}
}

void PandoraBoxConfig::serializeJson(JsonSerializeFormat & handler)
{
	if(handler.saving)
		handler.serializeStruct("reward", reward);
	else if(!handler.getCurrent()["reward"].isNull())
		loadReward(handler);

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
