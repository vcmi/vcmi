/*
 * SeerHutTreasures.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "SeerHutTreasures.h"

#include "Functions.h"
#include "RmgMap.h"
#include "Zone.h"
#include "modificators/QuestArtifactPlacer.h"
#include "../CCreatureHandler.h"
#include "../GameLibrary.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../mapObjects/Quest.h"
#include "../mapping/CMap.h"

TreasureRewards::CustomRewardDefaults SeerHutTreasures::rewardDefaults()
{
	TreasureRewards::CustomRewardDefaults defaults;
	defaults.listName = "seerHuts";
	defaults.amountStep = 1;
	defaults.goldMultiplier = 1;
	defaults.experienceMultiplier = 1;
	defaults.goldRarity = GOLD_RARITY;
	defaults.experienceRarity = EXPERIENCE_RARITY;
	defaults.creaturesRarity = CREATURES_RARITY;
	return defaults;
}

void SeerHutTreasures::addQuest(SeerHut & hut, ArtifactID artifact, const Rewardable::VisitInfo & reward)
{
	Quest & quest = hut.addQuest();
	quest.mission.artifacts.push_back(artifact);
	quest.reward = reward;
}

std::vector<size_t> SeerHutTreasures::pickSlots(const std::vector<ObjectInfo> & candidates, size_t slots, vstd::RNG & rng)
{
	std::vector<size_t> picks;
	if(candidates.empty() || slots == 0)
		return picks;

	std::vector<ui32> remaining(candidates.size());
	for(size_t i = 0; i < candidates.size(); ++i)
		remaining[i] = candidates[i].maxPerZone;

	for(size_t n = 0; n < slots; ++n)
	{
		std::vector<int64_t> weights(candidates.size(), 0);
		int64_t total = 0;
		for(size_t i = 0; i < candidates.size(); ++i)
		{
			if(!remaining[i])
				continue;
			weights[i] = candidates[i].probability;
			total += weights[i];
		}
		if(total <= 0)
			break;

		const int64_t index = RandomGeneratorUtil::nextItemWeighted(weights, rng);
		if(index < 0)
			break;

		picks.push_back(static_cast<size_t>(index));
		--remaining[index];
	}
	return picks;
}

SeerHutTreasures::SeerHutTreasures(Zone & zone, RmgMap & map, const CMapGenerator::Config & config)
	: zone(zone)
	, map(map)
	, config(config)
{
}

ObjectInfo SeerHutTreasures::createHut(const Rewardable::VisitInfo & reward, ui32 rewardValue, int appearance, QuestArtifactPlacer & qap) const
{
	ObjectInfo oi(Obj::SEER_HUT, 0);
	oi.value = static_cast<ui32>(config.seerHutValue);
	oi.maxPerZone = 1;
	oi.setTemplates(Obj::SEER_HUT, appearance, zone.getTerrainType());

	oi.generateObject = [cb = map.mapInstance->cb, appearance, reward, rewardValue, qap = &qap]() -> std::shared_ptr<CGObjectInstance>
	{
		const auto & factory = LIBRARY->objtypeh->getHandlerFor(Obj::SEER_HUT, appearance);
		auto hut = std::dynamic_pointer_cast<SeerHut>(factory.create(cb, nullptr));
		ArtifactID art = qap->drawRandomArtifact();
		addQuest(*hut, art, reward);
		qap->addQuestArtifact(art, rewardValue);
		return hut;
	};
	oi.destroyObject = [qap = &qap](CGObjectInstance & obj)
	{
		auto & seer = dynamic_cast<SeerHut &>(obj);
		ArtifactID artid = seer.getQuest().mission.artifacts.front();
		qap->addRandomArtifact(artid);
		qap->removeQuestArtifact(artid);
	};
	return oi;
}

std::optional<ObjectInfo> SeerHutTreasures::createCreatureHut(const CCreature * creature, int appearance, QuestArtifactPlacer & qap) const
{
	int creaturesAmount = TreasureRewards::nativeCreatureCount(creature, config.pandoraCreatureValues);
	if(!creaturesAmount)
		return std::nullopt;

	const auto rewardValue = static_cast<ui32>(((2 * (creature->getAIValue()) * creaturesAmount * (1 + static_cast<float>(map.getZoneCount(creature->getFactionID())) / map.getTotalZoneCount())) - 4000) / 3);
	if(rewardValue > zone.getMaxTreasureValue())
		return std::nullopt;

	auto oi = createHut(TreasureRewards::creatureReward(creature, creaturesAmount), rewardValue, appearance, qap);
	if(oi.templates.empty())
		return std::nullopt;
	oi.probability = CREATURES_RARITY;
	return oi;
}

std::vector<ObjectInfo> SeerHutTreasures::customCandidates(const std::vector<const CCreature *> & natives, QuestArtifactPlacer & qap) const
{
	std::vector<ObjectInfo> candidates;
	auto addCandidate = [&candidates](ObjectInfo oi, const TreasureRewards::CustomReward & reward)
	{
		oi.probability = reward.rarity;
		oi.maxPerZone = reward.zoneLimit ? reward.zoneLimit : std::numeric_limits<ui32>::max();
		candidates.push_back(std::move(oi));
	};

	// getCustomObjects() returns a copy: keep it alive for the loop
	const auto customObjects = zone.getCustomObjects();
	TreasureRewards::forEachCustomReward(customObjects.getSeerHuts(), rewardDefaults(), zone.getTownType(), natives,
		[this, &qap, &addCandidate](const TreasureRewards::CustomReward & reward, const CCreature * creature)
		{
			const int count = TreasureRewards::nativeCreatureCount(creature, config.pandoraCreatureValues);
			if(!count)
				return;

			int appearance = chooseRandomAppearance(zone.getRand(), Obj::SEER_HUT, zone.getTerrainType());
			if(reward.value)
			{
				if(*reward.value > zone.getMaxTreasureValue())
					return;
				auto oi = createHut(TreasureRewards::creatureReward(creature, count), *reward.value, appearance, qap);
				if(!oi.templates.empty())
					addCandidate(std::move(oi), reward);
			}
			else if(auto oi = createCreatureHut(creature, appearance, qap))
			{
				addCandidate(std::move(*oi), reward);
			}
		},
		[this, &qap, &addCandidate](const TreasureRewards::CustomReward & reward, const JsonNode & entry)
		{
			auto visit = TreasureRewards::readSingleReward(entry, map.mapInstance->cb);
			if(!visit || !reward.value)
				return;
			if(*reward.value > zone.getMaxTreasureValue())
				return;

			int appearance = chooseRandomAppearance(zone.getRand(), Obj::SEER_HUT, zone.getTerrainType());
			auto oi = createHut(*visit, *reward.value, appearance, qap);
			if(!oi.templates.empty())
				addCandidate(std::move(oi), reward);
		});
	return candidates;
}

std::vector<ObjectInfo> SeerHutTreasures::builtInCandidates(std::vector<const CCreature *> & natives, QuestArtifactPlacer & qap) const
{
	std::vector<ObjectInfo> candidates;
	candidates.reserve(14 + 4 + 4);

	RandomGeneratorUtil::randomShuffle(natives, zone.getRand());

	for(const auto * creature : natives)
	{
		if(!TreasureRewards::nativeCreatureCount(creature, config.pandoraCreatureValues))
			continue;

		int randomAppearance = chooseRandomAppearance(zone.getRand(), Obj::SEER_HUT, zone.getTerrainType());
		auto oi = createCreatureHut(creature, randomAppearance, qap);
		if(!oi)
			continue;
		candidates.push_back(*oi);
	}

	const int seerLevels = std::min(config.questValues.size(), config.questRewardValues.size());
	for(int i = 0; i < seerLevels; i++)
	{
		int randomAppearance = chooseRandomAppearance(zone.getRand(), Obj::SEER_HUT, zone.getTerrainType());
		const auto rewardValue = static_cast<ui32>(config.questRewardValues[i]);
		if(rewardValue > zone.getMaxTreasureValue())
			continue;

		auto expHut = createHut(TreasureRewards::experienceReward(config.questRewardValues[i]), rewardValue, randomAppearance, qap);
		expHut.probability = EXPERIENCE_RARITY;
		if(!expHut.templates.empty())
			candidates.push_back(expHut);

		auto goldHut = createHut(TreasureRewards::goldReward(config.questRewardValues[i]), rewardValue, randomAppearance, qap);
		goldHut.probability = GOLD_RARITY;
		if(!goldHut.templates.empty())
			candidates.push_back(goldHut);
	}
	return candidates;
}

SeerHutTreasures::Selection SeerHutTreasures::select(std::vector<const CCreature *> & natives) const
{
	Selection selection;
	if(zone.getConnectedZoneIds().empty())
		return selection;

	auto * qap = zone.getModificator<QuestArtifactPlacer>();
	if(!qap)
		return selection;

	const int questArtsRemaining = qap->getMaxQuestArtifactCount();
	if(!questArtsRemaining)
		return selection;

	const auto budget = static_cast<size_t>(questArtsRemaining);
	const auto custom = customCandidates(natives, *qap);
	for(size_t index : pickSlots(custom, budget, zone.getRand()))
	{
		ObjectInfo copy = custom[index];
		copy.maxPerZone = 1;
		selection.custom.push_back(std::move(copy));
	}

	const size_t remaining = budget - selection.custom.size();
	if(remaining == 0)
		return selection;

	const auto builtIn = builtInCandidates(natives, *qap);
	if(builtIn.empty())
		return selection;

	for(size_t i = 0; i < remaining; i++)
		selection.builtIn.push_back(*RandomGeneratorUtil::nextItem(builtIn, zone.getRand()));
	return selection;
}
