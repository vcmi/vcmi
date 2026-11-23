/*
 * PandoraBoxTreasures.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "PandoraBoxTreasures.h"

#include "RmgMap.h"
#include "Zone.h"
#include "../CCreatureHandler.h"
#include "../GameLibrary.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../mapObjects/CGPandoraBox.h"
#include "../mapping/CMap.h"
#include "../spells/CSpellHandler.h"

#include <vstd/RNG.h>

namespace
{
/// Value of a gold or experience Pandora box: the randomMap.json multiplier is the value of one AMOUNT_STEP
ui32 amountValue(ui32 amount, int multiplier)
{
	return static_cast<ui32>(static_cast<ui64>(amount) * multiplier / PandoraBoxTreasures::AMOUNT_STEP);
}

std::shared_ptr<CGPandoraBox> createPandoraBox(IGameInfoCallback * cb)
{
	const auto & factory = LIBRARY->objtypeh->getHandlerFor(Obj::PANDORAS_BOX, 0);
	return std::dynamic_pointer_cast<CGPandoraBox>(factory.create(cb, nullptr));
}

std::shared_ptr<CGObjectInstance> createPandoraBox(IGameInfoCallback * cb, const Rewardable::VisitInfo & reward)
{
	auto obj = createPandoraBox(cb);
	obj->configuration.info.push_back(reward);
	return obj;
}

/// A box with up to `count` random allowed spells that pass `filter`, drawn when the box is generated
std::shared_ptr<CGObjectInstance> createSpellBox(IGameInfoCallback * cb, RmgMap & map, Zone & zone, int count, const std::function<bool(const CSpell *)> & filter)
{
	auto obj = createPandoraBox(cb);

	std::vector <const CSpell *> spells;
	for(auto spellID : LIBRARY->spellh->getDefaultAllowed())
	{
		if(map.isAllowedSpell(spellID) && filter(spellID.toSpell()))
			spells.push_back(spellID.toSpell());
	}

	RandomGeneratorUtil::randomShuffle(spells, zone.getRand());
	Rewardable::VisitInfo reward;
	for(int j = 0; j < std::min(count, static_cast<int>(spells.size())); j++)
	{
		reward.reward.spells.push_back(spells[j]->id);
	}
	reward.visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
	obj->configuration.info.push_back(reward);

	return obj;
}
}

TreasureRewards::CustomRewardDefaults PandoraBoxTreasures::rewardDefaults(const CMapGenerator::Config & config)
{
	TreasureRewards::CustomRewardDefaults defaults;
	defaults.listName = "pandoraBoxes";
	defaults.amountStep = AMOUNT_STEP;
	defaults.goldMultiplier = config.pandoraMultiplierGold;
	defaults.experienceMultiplier = config.pandoraMultiplierExperience;
	defaults.goldRarity = GOLD_RARITY;
	defaults.experienceRarity = EXPERIENCE_RARITY;
	defaults.creaturesRarity = CREATURES_RARITY;
	return defaults;
}

PandoraBoxTreasures::PandoraBoxTreasures(Zone & zone, RmgMap & map, const CMapGenerator::Config & config)
	: zone(zone)
	, map(map)
	, config(config)
{
}

ObjectInfo PandoraBoxTreasures::box(std::function<std::shared_ptr<CGObjectInstance>()> generate, ui32 value, ui16 rarity) const
{
	ObjectInfo oi(Obj::PANDORAS_BOX, 0);
	oi.generateObject = std::move(generate);
	oi.setTemplates(Obj::PANDORAS_BOX, 0, zone.getTerrainType());
	oi.value = value;
	oi.probability = rarity;
	return oi;
}

std::vector<ObjectInfo> PandoraBoxTreasures::builtInBoxes(const std::vector<const CCreature *> & natives) const
{
	std::vector<ObjectInfo> boxes;
	auto addBox = [&boxes](ObjectInfo oi)
	{
		if(!oi.templates.empty())
			boxes.push_back(std::move(oi));
	};
	auto * cb = map.mapInstance->cb;

	for(int i = 1; i < 5; i++)
	{
		const auto reward = TreasureRewards::goldReward(i * AMOUNT_STEP);
		addBox(box([cb, reward]() { return createPandoraBox(cb, reward); }, amountValue(i * AMOUNT_STEP, config.pandoraMultiplierGold), GOLD_RARITY));
	}

	for(int i = 1; i < 5; i++)
	{
		const auto reward = TreasureRewards::experienceReward(i * AMOUNT_STEP);
		addBox(box([cb, reward]() { return createPandoraBox(cb, reward); }, amountValue(i * AMOUNT_STEP, config.pandoraMultiplierExperience), EXPERIENCE_RARITY));
	}

	for(const auto * creature : natives)
	{
		auto oi = creatureBox(creature);
		if(!oi)
			continue;
		oi->probability = CREATURES_RARITY;
		boxes.push_back(std::move(*oi));
	}

	addSpellBoxes(boxes);
	return boxes;
}

std::optional<ObjectInfo> PandoraBoxTreasures::creatureBox(const CCreature * creature) const
{
	int creaturesAmount = TreasureRewards::nativeCreatureCount(creature, config.pandoraCreatureValues);
	if(!creaturesAmount)
		return std::nullopt;

	const auto value = static_cast<ui32>(creature->getAIValue() * creaturesAmount * (1 + static_cast<float>(map.getZoneCount(creature->getFactionID())) / map.getTotalZoneCount()));
	const auto reward = TreasureRewards::creatureReward(creature, creaturesAmount);
	auto oi = box([cb = map.mapInstance->cb, reward]() { return createPandoraBox(cb, reward); }, value, 0);
	if(oi.templates.empty())
		return std::nullopt;
	return oi;
}

void PandoraBoxTreasures::addSpellBoxes(std::vector<ObjectInfo> & boxes) const
{
	auto * cb = map.mapInstance->cb;
	auto addBox = [this, &boxes](std::function<std::shared_ptr<CGObjectInstance>()> generate, ui32 value)
	{
		auto oi = box(std::move(generate), value, SPELLS_RARITY);
		if(!oi.templates.empty())
			boxes.push_back(std::move(oi));
	};

	//Pandora with 12 spells of certain level
	for(int i = 1; i <= GameConstants::SPELL_LEVELS; i++)
	{
		addBox([cb, &map = map, &zone = zone, i]()
		{
			return createSpellBox(cb, map, zone, 12, [i](const CSpell * spell) { return spell->getLevel() == i; });
		}, (i + 1) * config.pandoraMultiplierSpells); //5000 - 15000
	}

	//Pandora with 15 spells of certain school
	for(int i = 0; i < 4; i++)
	{
		addBox([cb, &map = map, &zone = zone, i]()
		{
			return createSpellBox(cb, map, zone, 15, [i](const CSpell * spell) { return spell->hasSchool(SpellSchool(i)); });
		}, config.pandoraSpellSchool);
	}

	// Pandora box with 60 random spells
	addBox([cb, &map = map, &zone = zone]()
	{
		return createSpellBox(cb, map, zone, 60, [](const CSpell *) { return true; });
	}, config.pandoraSpell60);
}

std::vector<ObjectInfo> PandoraBoxTreasures::customBoxes(const std::vector<const CCreature *> & natives) const
{
	std::vector<ObjectInfo> boxes;
	// getCustomObjects() returns a copy: keep it alive for the loop
	const auto customObjects = zone.getCustomObjects();
	TreasureRewards::forEachCustomReward(customObjects.getPandoraBoxes(), rewardDefaults(config), zone.getTownType(), natives,
		[this, &boxes](const TreasureRewards::CustomReward & reward, const CCreature * creature)
		{
			auto oi = creatureBox(creature);
			if(!oi)
				return;
			if(reward.value)
				oi->value = *reward.value;
			oi->probability = reward.rarity;
			if(reward.zoneLimit)
				oi->maxPerZone = reward.zoneLimit;
			boxes.push_back(std::move(*oi));
		},
		[this, &boxes](const TreasureRewards::CustomReward & reward, const JsonNode & entry)
		{
			auto oi = box([entry, cb = map.mapInstance->cb]() -> std::shared_ptr<CGObjectInstance>
			{
				return TreasureRewards::loadPandoraBoxOptions(entry, cb);
			}, *reward.value, reward.rarity);
			if(reward.zoneLimit)
				oi.maxPerZone = reward.zoneLimit;
			if(!oi.templates.empty())
				boxes.push_back(std::move(oi));
		});
	return boxes;
}
