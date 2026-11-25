/*
 * PandoraBoxTreasures.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "CMapGenerator.h"
#include "ObjectInfo.h"
#include "TreasureRewards.h"

class CCreature;
class RmgMap;
class Zone;

/// Pandora box treasure candidates of one zone: the built-in gold, experience, creature and spell boxes, and customObjects.pandoraBoxes
class PandoraBoxTreasures
{
public:
	/// Built-in gold and experience boxes hold i * AMOUNT_STEP; randomMap.json pandora multipliers are the value of one step
	static constexpr ui32 AMOUNT_STEP = 5000;
	static constexpr ui16 GOLD_RARITY = 5;
	static constexpr ui16 EXPERIENCE_RARITY = 20;
	static constexpr ui16 CREATURES_RARITY = 3;
	static constexpr ui16 SPELLS_RARITY = 2;

	/// customObjects.pandoraBoxes defaults: values scale like the built-in boxes, rarities are the built-in ones
	static TreasureRewards::CustomRewardDefaults rewardDefaults(const CMapGenerator::Config & config);

	PandoraBoxTreasures(Zone & zone, RmgMap & map, const CMapGenerator::Config & config);

	/// Built-in boxes in pool order: gold, experience, one per native creature, spells
	std::vector<ObjectInfo> builtInBoxes(const std::vector<const CCreature *> & natives) const;
	/// Boxes from customObjects.pandoraBoxes, in entry order
	std::vector<ObjectInfo> customBoxes(const std::vector<const CCreature *> & natives) const;

private:
	ObjectInfo box(std::function<std::shared_ptr<CGObjectInstance>()> generate, ui32 value, ui16 rarity) const;
	/// Built-in creature box content and value for a native creature (no rarity); nullopt when the creature gets no box
	std::optional<ObjectInfo> creatureBox(const CCreature * creature) const;
	void addSpellBoxes(std::vector<ObjectInfo> & boxes) const;

	Zone & zone;
	RmgMap & map;
	const CMapGenerator::Config & config;
};
