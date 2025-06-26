/*
 * Reward.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../ResourceSet.h"
#include "../bonuses/Bonus.h"
#include "../CCreatureSet.h"
#include "../networkPacks/Component.h"

VCMI_LIB_NAMESPACE_BEGIN

struct Bonus;
struct Component;
class CStackBasicDescriptor;
class CGHeroInstance;

namespace Rewardable
{

struct Reward;
using RewardsList = std::vector<std::shared_ptr<Rewardable::Reward>>;

struct RewardRevealTiles
{
	/// Reveal distance, if not positive - reveal entire map
	int radius;
	/// Reveal score of terrains with "surface" flag set
	int scoreSurface;
	/// Reveal score of terrains with "subterra" flag set
	int scoreSubterra;
	/// Reveal score of terrains with "water" flag set
	int scoreWater;
	/// Reveal score of terrains with "rock" flag set
	int scoreRock;
	/// If set, then terrain will be instead hidden for all enemies (Cover of Darkness)
	bool hide;

	void serializeJson(JsonSerializeFormat & handler);

	template <typename Handler> void serialize(Handler &h)
	{
		h & radius;
		h & scoreSurface;
		h & scoreSubterra;
		h & scoreWater;
		h & scoreRock;
		h & hide;
	}
};

/// Reward that can be granted to a hero
struct DLL_LINKAGE Reward final
{
	/// resources that will be given to player
	TResources resources;

	/// received experience
	si32 heroExperience;

	/// received levels (converted into XP during grant)
	si32 heroLevel;

	/// mana given to/taken from hero, fixed value
	si32 manaDiff;

	/// if giving mana points puts hero above mana pool, any overflow will be multiplied by specified percentage
	si32 manaOverflowFactor;

	/// fixed value, in form of percentage from max
	si32 manaPercentage;

	/// movement points, only for current day. Bonuses should be used to grant MP on any other day
	si32 movePoints;
	/// fixed value, in form of percentage from max
	si32 movePercentage;

	/// Guards that must be defeated in order to access this reward, empty if not guarded
	std::vector<CStackBasicDescriptor> guards;

	/// list of bonuses, e.g. morale/luck
	std::vector<std::shared_ptr<Bonus>> heroBonuses;
	std::vector<std::shared_ptr<Bonus>> commanderBonuses;
	std::vector<std::shared_ptr<Bonus>> playerBonuses;

	/// skills that hero may receive or lose
	std::vector<si32> primary;
	std::map<SecondarySkill, si32> secondary;

	/// creatures that will be changed in hero's army
	std::map<CreatureID, CreatureID> creaturesChange;

	/// objects that hero may receive
	std::vector<ArtifactID> grantedArtifacts;
	std::vector<ArtifactID> takenArtifacts;
	std::vector<ArtifactPosition> takenArtifactSlots;
	std::vector<SpellID> grantedScrolls;
	std::vector<SpellID> takenScrolls;
	std::vector<SpellID> spells;
	std::vector<CStackBasicDescriptor> creatures;
	std::vector<CStackBasicDescriptor> takenCreatures;
	
	/// actions that hero may execute and object caster. Pair of spellID and school level
	std::pair<SpellID, int> spellCast;

	/// list of components that will be added to reward description. First entry in list will override displayed component
	std::vector<Component> extraComponents;

	std::optional<RewardRevealTiles> revealTiles;

	/// if set to true, object will be removed after granting reward
	bool removeObject;

	/// Generates list of components that describes reward for a specific hero
	/// If hero is nullptr, then rewards will be generated without accounting for hero
	void loadComponents(std::vector<Component> & comps, const CGHeroInstance * h) const;
	
	Component getDisplayedComponent(const CGHeroInstance * h) const;

	si32 calculateManaPoints(const CGHeroInstance * h) const;

	Reward();
	~Reward();

	template <typename Handler> void serialize(Handler &h)
	{
		h & resources;
		h & extraComponents;
		h & removeObject;
		h & manaPercentage;
		h & movePercentage;
		h & guards;
		h & heroExperience;
		h & heroLevel;
		h & manaDiff;
		h & manaOverflowFactor;
		h & movePoints;
		h & primary;
		h & secondary;
		h & heroBonuses;
		if (h.version >= Handler::Version::REWARDABLE_EXTENSIONS)
		{
			h & playerBonuses;
			h & commanderBonuses;
		}
		h & grantedArtifacts;
		if (h.version >= Handler::Version::REWARDABLE_EXTENSIONS)
		{
			h & takenArtifacts;
			h & takenArtifactSlots;
			h & grantedScrolls;
			h & takenScrolls;
		}
		h & spells;
		h & creatures;
		if (h.version >= Handler::Version::REWARDABLE_EXTENSIONS)
			h & takenCreatures;
		h & creaturesChange;
		h & revealTiles;
		h & spellCast;
	}
	
	void serializeJson(JsonSerializeFormat & handler);
};
}

VCMI_LIB_NAMESPACE_END
