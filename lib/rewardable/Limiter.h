/*
 * Limiter.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../GameConstants.h"
#include "../ResourceSet.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;
class CStackBasicDescriptor;

namespace Rewardable {

struct Limiter;
using LimitersList = std::vector<std::shared_ptr<Rewardable::Limiter>>;

/// Limiters of rewards. Rewards will be granted to hero only if he satisfies requirements
/// Note: for this is only a test - it won't remove anything from hero (e.g. artifacts or creatures)
/// NOTE: in future should (partially) replace seer hut/quest guard quests checks
struct DLL_LINKAGE Limiter
{
	/// day of week, unused if 0, 1-7 will test for current day of week
	si32 dayOfWeek;
	si32 daysPassed;

	/// total experience that hero needs to have
	si32 heroExperience;

	/// level that hero needs to have
	si32 heroLevel;

	/// mana points that hero needs to have
	si32 manaPoints;

	/// percentage of mana points that hero needs to have
	si32 manaPercentage;

	/// resources player needs to have in order to trigger reward
	TResources resources;

	/// skills hero needs to have
	std::vector<si32> primary;
	std::map<SecondarySkill, si32> secondary;

	/// artifacts that hero needs to have (equipped or in backpack) to trigger this
	/// Note: does not checks for multiple copies of the same arts
	std::vector<ArtifactID> artifacts;

	/// Spells that hero must have in the spellbook
	std::vector<SpellID> spells;

	/// creatures that hero needs to have
	std::vector<CStackBasicDescriptor> creatures;

	/// sub-limiters, all must pass for this limiter to pass
	LimitersList allOf;

	/// sub-limiters, at least one should pass for this limiter to pass
	LimitersList anyOf;

	/// sub-limiters, none should pass for this limiter to pass
	LimitersList noneOf;

	Limiter();
	~Limiter();

	bool heroAllowed(const CGHeroInstance * hero) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & dayOfWeek;
		h & daysPassed;
		h & heroExperience;
		h & heroLevel;
		h & manaPoints;
		h & manaPercentage;
		h & resources;
		h & primary;
		h & secondary;
		h & artifacts;
		h & creatures;
		h & allOf;
		h & anyOf;
		h & noneOf;
	}
};

}

VCMI_LIB_NAMESPACE_END
