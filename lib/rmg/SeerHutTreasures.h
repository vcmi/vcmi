/*
 * SeerHutTreasures.h, part of VCMI engine
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

#include <vstd/RNG.h>

class CCreature;
class QuestArtifactPlacer;
class RmgMap;
class SeerHut;
class Zone;

/// Seer hut treasures of one zone. Each hut takes one quest artifact from the zone's budget:
/// customObjects.seerHuts fill the budget first, and the slots left go to built-in creature, experience and gold huts.
class SeerHutTreasures
{
public:
	static constexpr ui16 GOLD_RARITY = 10;
	static constexpr ui16 EXPERIENCE_RARITY = 10;
	static constexpr ui16 CREATURES_RARITY = 3;

	/// customObjects.seerHuts defaults: value = amount, the built-in hut rarities
	static TreasureRewards::CustomRewardDefaults rewardDefaults();
	/// gives the hut its one quest: bring artifact, receive reward (stored in Quest::reward)
	static void addQuest(SeerHut & hut, ArtifactID artifact, const Rewardable::VisitInfo & reward);
	/// candidate indices for slots, weighted by probability, each candidate at most maxPerZone times; stops early when every candidate is used up
	static std::vector<size_t> pickSlots(const std::vector<ObjectInfo> & candidates, size_t slots, vstd::RNG & rng);

	struct Selection
	{
		std::vector<ObjectInfo> custom; ///< one entry per hut (maxPerZone 1); added after zone bans, so bans never remove them
		std::vector<ObjectInfo> builtIn; ///< one entry per hut slot; added before zone bans
	};

	SeerHutTreasures(Zone & zone, RmgMap & map, const CMapGenerator::Config & config);

	/// Huts for this zone's quest artifact budget. Shuffles natives in place: built-in creature huts are drawn in shuffled order.
	Selection select(std::vector<const CCreature *> & natives) const;

private:
	ObjectInfo createHut(const Rewardable::VisitInfo & reward, ui32 rewardValue, int appearance, QuestArtifactPlacer & qap) const;
	/// Built-in creature hut for a native creature; nullopt when the creature gets no hut
	std::optional<ObjectInfo> createCreatureHut(const CCreature * creature, int appearance, QuestArtifactPlacer & qap) const;
	std::vector<ObjectInfo> customCandidates(const std::vector<const CCreature *> & natives, QuestArtifactPlacer & qap) const;
	std::vector<ObjectInfo> builtInCandidates(std::vector<const CCreature *> & natives, QuestArtifactPlacer & qap) const;

	Zone & zone;
	RmgMap & map;
	const CMapGenerator::Config & config;
};
