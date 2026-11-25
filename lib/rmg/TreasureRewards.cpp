/*
 * TreasureRewards.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "TreasureRewards.h"

#include "../CCreatureHandler.h"
#include "../GameLibrary.h"
#include "../json/JsonNode.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../mapObjects/CGPandoraBox.h"
#include "../modding/IdentifierStorage.h"
#include "../modding/ModScope.h"
#include "../serializer/JsonDeserializer.h"

namespace TreasureRewards
{

namespace
{
/// rmg.nativeCreatures into `reward`; false (and an error log) when the entry must be ignored
bool readNativeCreatures(const JsonNode & nativeCreatures, const JsonNode & entry, const CustomRewardDefaults & defaults, CustomReward & reward)
{
	reward.nativeCreaturesLevel = nativeCreatures["level"].Integer();
	reward.rarity = defaults.creaturesRarity;

	const std::string kind = nativeCreatures["kind"].String();
	if(kind.empty())
		return true;

	if(kind != "basic" && kind != "upgrade")
	{
		logRmg->error("RMG %s entry is ignored: unknown nativeCreatures.kind '%s': %s", defaults.listName, kind, entry.toCompactString());
		return false;
	}

	if(*reward.nativeCreaturesLevel < 1)
		logRmg->warn("RMG %s nativeCreatures.kind is ignored: kind needs level 1 or more: %s", defaults.listName, entry.toCompactString());
	else
		reward.nativeCreaturesKind = kind == "basic" ? NativeCreatureKind::BASIC : NativeCreatureKind::UPGRADE;
	return true;
}

std::set<FactionID> readZoneFactions(const JsonNode & zoneFactions, const CustomRewardDefaults & defaults)
{
	std::set<FactionID> factions;
	for(const auto & faction : zoneFactions.Vector())
	{
		auto factionID = LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), "faction", faction.String());
		if(factionID)
			factions.insert(FactionID(*factionID));
		else
			logRmg->error("RMG %s entry: unknown zone faction '%s' is ignored", defaults.listName, faction.String());
	}
	return factions;
}

ui32 scaledAmount(si64 amount, int multiplier, ui32 amountStep)
{
	return static_cast<ui32>(static_cast<ui64>(amount) * multiplier / amountStep);
}
}

std::optional<CustomReward> readCustomReward(const JsonNode & entry, const CustomRewardDefaults & defaults)
{
	std::set<std::string> contentKeys;
	for(const auto & [key, value] : entry.Struct())
	{
		if(key != "rmg")
			contentKeys.insert(key);
	}

	const JsonNode & resources = entry["resources"];
	const bool goldOnly = contentKeys == std::set<std::string>{"resources"} && resources.Struct().size() == 1 && resources.Struct().count("gold");
	const bool experienceOnly = contentKeys == std::set<std::string>{"experience"};

	const JsonNode & rmg = entry["rmg"];
	const JsonNode & nativeCreatures = rmg["nativeCreatures"];
	if(!nativeCreatures.isNull() && !contentKeys.empty())
	{
		logRmg->error("RMG %s entry is ignored: rmg.nativeCreatures cannot be combined with other content: %s", defaults.listName, entry.toCompactString());
		return std::nullopt;
	}

	CustomReward reward;
	if(!nativeCreatures.isNull() && !readNativeCreatures(nativeCreatures, entry, defaults, reward))
		return std::nullopt;
	if(goldOnly)
	{
		reward.value = scaledAmount(resources["gold"].Integer(), defaults.goldMultiplier, defaults.amountStep);
		reward.rarity = defaults.goldRarity;
	}
	if(experienceOnly)
	{
		reward.value = scaledAmount(entry["experience"].Integer(), defaults.experienceMultiplier, defaults.amountStep);
		reward.rarity = defaults.experienceRarity;
	}

	if(!goldOnly && !experienceOnly && !reward.nativeCreaturesLevel && (rmg["value"].isNull() || rmg["rarity"].isNull()))
	{
		logRmg->error("RMG %s entry is ignored: only gold-only, experience-only and native creature boxes have a default rmg value and rarity: %s", defaults.listName, entry.toCompactString());
		return std::nullopt;
	}

	if(!rmg["value"].isNull())
		reward.value = rmg["value"].Integer();
	if(!rmg["rarity"].isNull())
		reward.rarity = rmg["rarity"].Integer();
	reward.zoneLimit = rmg["zoneLimit"].Integer();
	reward.zoneFactions = readZoneFactions(rmg["zoneFactions"], defaults);

	return reward;
}

std::vector<const CCreature *> selectNativeCreatures(const CustomReward & reward, const std::vector<const CCreature *> & natives)
{
	std::vector<const CCreature *> selected;
	if(!reward.nativeCreaturesLevel)
		return selected;

	for(const auto * creature : natives)
	{
		if(*reward.nativeCreaturesLevel && creature->getLevel() != *reward.nativeCreaturesLevel)
			continue;

		if(reward.nativeCreaturesKind != NativeCreatureKind::ALL)
		{
			const bool isUpgrade = std::any_of(natives.begin(), natives.end(), [creature](const CCreature * other)
			{
				return creature->isUpgradeOf(other->getId());
			});
			if(reward.nativeCreaturesKind == NativeCreatureKind::BASIC && isUpgrade)
				continue;
			if(reward.nativeCreaturesKind == NativeCreatureKind::UPGRADE && !isUpgrade)
				continue;
		}
		selected.push_back(creature);
	}
	return selected;
}

std::shared_ptr<CGPandoraBox> loadPandoraBoxOptions(const JsonNode & entry, IGameInfoCallback * cb)
{
	JsonNode objectJson;
	objectJson["options"] = entry;
	objectJson["options"].Struct().erase("rmg");
	objectJson.setModScope(ModScope::scopeGame());

	const auto & factory = LIBRARY->objtypeh->getHandlerFor(Obj::PANDORAS_BOX, 0);
	auto obj = std::dynamic_pointer_cast<CGPandoraBox>(factory.create(cb, nullptr));
	InstantIdentifierResolver resolver(*LIBRARY->identifiers());
	JsonDeserializer handler(nullptr, resolver, objectJson);
	static_cast<CGObjectInstance &>(*obj).serializeJson(handler);
	return obj;
}

std::optional<Rewardable::VisitInfo> readSingleReward(const JsonNode & entry, IGameInfoCallback * cb)
{
	auto box = loadPandoraBoxOptions(entry, cb);
	if(!box || box->configuration.info.size() != 1)
	{
		logRmg->error("RMG seerHuts entry is ignored: content must load to exactly one reward: %s", entry.toCompactString());
		return std::nullopt;
	}
	return box->configuration.info.front();
}

int nativeCreatureCount(const CCreature * creature, const std::vector<int> & tierValues)
{
	if(!creature->getAIValue() || tierValues.empty()) //bug #2681
		return 0; //this box won't be generated

	//Follow the rules from https://heroes.thelazy.net/index.php/Pandora%27s_Box

	int actualTier = creature->getLevel() > tierValues.size() ?
		tierValues.size() - 1 :
		creature->getLevel() - 1;
	float creaturesAmount = std::floor((static_cast<float>(tierValues[actualTier])) / creature->getAIValue());
	if (creaturesAmount < 1)
	{
		return 0;
	}
	else if(creaturesAmount <= 5)
	{
		//No change
	}
	else if(creaturesAmount <= 12)
	{
		creaturesAmount = std::ceil(creaturesAmount / 2) * 2;
	}
	else if(creaturesAmount <= 50)
	{
		creaturesAmount = std::round(creaturesAmount / 5) * 5;
	}
	else
	{
		creaturesAmount = std::round(creaturesAmount / 10) * 10;
	}
	return static_cast<int>(creaturesAmount);
}

Rewardable::VisitInfo goldReward(int amount)
{
	Rewardable::VisitInfo info;
	info.reward.resources[EGameResID::GOLD] = amount;
	info.visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
	return info;
}

Rewardable::VisitInfo experienceReward(int amount)
{
	Rewardable::VisitInfo info;
	info.reward.heroExperience = amount;
	info.visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
	return info;
}

Rewardable::VisitInfo creatureReward(const CCreature * creature, int count)
{
	Rewardable::VisitInfo info;
	info.reward.creatures.emplace_back(creature->getId(), count);
	info.visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
	return info;
}

}
