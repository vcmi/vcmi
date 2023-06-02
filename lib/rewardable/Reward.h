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

VCMI_LIB_NAMESPACE_BEGIN

struct Bonus;
struct Component;
class CStackBasicDescriptor;
class CGHeroInstance;

namespace Rewardable
{

struct Reward;
using RewardsList = std::vector<std::shared_ptr<Rewardable::Reward>>;

/// Reward that can be granted to a hero
/// NOTE: eventually should replace seer hut rewards and events/pandoras
struct DLL_LINKAGE Reward
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

	/// list of bonuses, e.g. morale/luck
	std::vector<Bonus> bonuses;

	/// skills that hero may receive or lose
	std::vector<si32> primary;
	std::map<SecondarySkill, si32> secondary;

	/// creatures that will be changed in hero's army
	std::map<CreatureID, CreatureID> creaturesChange;

	/// objects that hero may receive
	std::vector<ArtifactID> artifacts;
	std::vector<SpellID> spells;
	std::vector<CStackBasicDescriptor> creatures;
	
	/// actions that hero may execute and object caster. Pair of spellID and school level
	std::pair<SpellID, int> spellCast;

	/// list of components that will be added to reward description. First entry in list will override displayed component
	std::vector<Component> extraComponents;

	/// if set to true, object will be removed after granting reward
	bool removeObject;

	/// Generates list of components that describes reward for a specific hero
	virtual void loadComponents(std::vector<Component> & comps,
								const CGHeroInstance * h) const;
	
	Component getDisplayedComponent(const CGHeroInstance * h) const;

	si32 calculateManaPoints(const CGHeroInstance * h) const;

	Reward();
	~Reward();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & resources;
		h & extraComponents;
		h & removeObject;
		h & manaPercentage;
		h & movePercentage;
		h & heroExperience;
		h & heroLevel;
		h & manaDiff;
		h & manaOverflowFactor;
		h & movePoints;
		h & primary;
		h & secondary;
		h & bonuses;
		h & artifacts;
		h & spells;
		h & creatures;
		h & creaturesChange;
		if(version >= 821)
			h & spellCast;
	}
};
}

VCMI_LIB_NAMESPACE_END
