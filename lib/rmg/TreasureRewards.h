/*
 * TreasureRewards.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../constants/EntityIdentifiers.h"
#include "../rewardable/Configuration.h"

class CCreature;
class CGPandoraBox;
class IGameInfoCallback;
class JsonNode;

/// Rewards of RMG treasure objects: reading customObjects reward lists (pandoraBoxes, seerHuts) and building single first-visit rewards
namespace TreasureRewards
{
enum class NativeCreatureKind { ALL, BASIC, UPGRADE };

/// Defaults of one customObjects reward list; gold and experience value = amount * multiplier / amountStep
struct CustomRewardDefaults
{
	std::string listName; ///< "pandoraBoxes" or "seerHuts", for log messages
	ui32 amountStep = 1;
	int goldMultiplier = 1;
	int experienceMultiplier = 1;
	ui16 goldRarity = 0;
	ui16 experienceRarity = 0;
	ui16 creaturesRarity = 0;
};

/// "rmg" block of a customObjects.pandoraBoxes or seerHuts entry
struct CustomReward
{
	std::optional<ui32> value; ///< nullopt only for native creature entries without rmg.value: valued like the built-in creature object
	ui16 rarity = 0;
	ui32 zoneLimit = 0; ///< 0 = unlimited
	std::set<FactionID> zoneFactions; ///< empty = all
	std::optional<int> nativeCreaturesLevel; ///< set for rmg.nativeCreatures entries; 0 = all levels
	NativeCreatureKind nativeCreaturesKind = NativeCreatureKind::ALL; ///< ALL unless nativeCreaturesLevel >= 1
};

/// rmg fields of an entry with list defaults applied; nullopt (and an error log) when value or rarity is missing
std::optional<CustomReward> readCustomReward(const JsonNode & entry, const CustomRewardDefaults & defaults);

/// natives that get an rmg.nativeCreatures object, in natives order. An upgrade is a creature that isUpgradeOf another creature in natives; basic is every other creature
std::vector<const CCreature *> selectNativeCreatures(const CustomReward & reward, const std::vector<const CCreature *> & natives);

/// Reads each entry of a reward list in order and skips unreadable entries and entries limited to other zone factions.
/// An rmg.nativeCreatures entry calls onNativeCreature(const CustomReward &, const CCreature *) once per selected native creature;
/// any other entry calls onEntry(const CustomReward &, const JsonNode &) once.
template<typename NativeCreatureBuilder, typename EntryBuilder>
void forEachCustomReward(const std::vector<JsonNode> & entries, const CustomRewardDefaults & defaults, FactionID zoneFaction, const std::vector<const CCreature *> & natives, NativeCreatureBuilder && onNativeCreature, EntryBuilder && onEntry)
{
	for(const auto & entry : entries)
	{
		auto reward = readCustomReward(entry, defaults);
		if(!reward)
			continue;
		if(!reward->zoneFactions.empty() && !reward->zoneFactions.count(zoneFaction))
			continue;

		if(reward->nativeCreaturesLevel)
		{
			for(const auto * creature : selectNativeCreatures(*reward, natives))
				onNativeCreature(*reward, creature);
		}
		else
		{
			onEntry(*reward, entry);
		}
	}
}

/// the entry without "rmg", loaded as Pandora box map options exactly like the .vmap loader does
std::shared_ptr<CGPandoraBox> loadPandoraBoxOptions(const JsonNode & entry, IGameInfoCallback * cb);

/// the single reward of an entry's content; nullopt (and an error log) unless it loads to exactly one
std::optional<Rewardable::VisitInfo> readSingleReward(const JsonNode & entry, IGameInfoCallback * cb);

/// creatures a built-in creature object grants, from the tier values (randomMap.json pandoraCreatureValues); 0 = no object for this creature
int nativeCreatureCount(const CCreature * creature, const std::vector<int> & tierValues);

/// First-visit rewards with one kind of content
Rewardable::VisitInfo goldReward(int amount);
Rewardable::VisitInfo experienceReward(int amount);
Rewardable::VisitInfo creatureReward(const CCreature * creature, int count);
}
