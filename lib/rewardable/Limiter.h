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
#include "../serializer/Serializeable.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;
class CStackBasicDescriptor;
struct Component;

namespace Rewardable {

struct Limiter;
using LimitersList = std::vector<std::shared_ptr<Rewardable::Limiter>>;

/// Limiters of rewards. Rewards will be granted to hero only if he satisfies requirements
/// Note: for this is only a test - it won't remove anything from hero (e.g. artifacts or creatures)
struct DLL_LINKAGE Limiter final : public Serializeable
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

	/// Number of free secondary slots that hero needs to have
	bool canLearnSkills;

	/// Hero has commander, and commander is currently alive
	bool commanderAlive;

	/// Hero has creatures other than those requested in 'creatures' field
	/// In other words, it is possible to take requested creatures from hero
	bool hasExtraCreatures;

	/// resources player needs to have in order to trigger reward
	TResources resources;

	/// skills hero needs to have
	std::vector<si32> primary;
	std::map<SecondarySkill, si32> secondary;

	/// artifacts that hero needs to have (equipped or in backpack) to trigger this
	/// checks for artifacts copies if same artifact id is included multiple times
	std::vector<ArtifactID> artifacts;

	/// artifact slots that hero needs to have available (not locked and without any artifact) to pass the limiter
	std::vector<ArtifactPosition> availableSlots;

	/// Spell scrolls that hero must have in inventory (equipped or in backpack)
	std::vector<SpellID> scrolls;

	/// Spells that hero must have in the spellbook
	std::vector<SpellID> spells;

	/// Spells that hero must be able to learn
	std::vector<SpellID> canLearnSpells;

	/// creatures that hero needs to have
	std::vector<CStackBasicDescriptor> creatures;
	
	/// creatures that hero needs to have
	std::vector<CStackBasicDescriptor> canReceiveCreatures;

	/// only heroes/hero classes from list could pass limiter
	std::vector<HeroTypeID> heroes;
	std::vector<HeroClassID> heroClasses;
	
	/// only player colors can pass limiter
	std::vector<PlayerColor> players;

	/// sub-limiters, all must pass for this limiter to pass
	LimitersList allOf;

	/// sub-limiters, at least one should pass for this limiter to pass
	LimitersList anyOf;

	/// sub-limiters, none should pass for this limiter to pass
	LimitersList noneOf;

	Limiter();
	~Limiter();

	bool heroAllowed(const CGHeroInstance * hero) const;
	
	/// Generates list of components that describes reward for a specific hero
	void loadComponents(std::vector<Component> & comps,
								const CGHeroInstance * h) const;

	template <typename Handler> void serialize(Handler &h)
	{
		h & dayOfWeek;
		h & daysPassed;
		h & heroExperience;
		h & heroLevel;
		h & manaPoints;
		h & manaPercentage;
		h & canLearnSkills;
		if (h.version >= Handler::Version::REWARDABLE_EXTENSIONS)
		{
			h & commanderAlive;
			h & hasExtraCreatures;
		}
		h & resources;
		h & primary;
		h & secondary;
		h & artifacts;
		if (h.version >= Handler::Version::REWARDABLE_EXTENSIONS)
		{
			h & availableSlots;
			h & scrolls;
		}
		h & spells;
		h & canLearnSpells;
		h & creatures;
		if (h.version >= Handler::Version::REWARDABLE_EXTENSIONS)
		{
			h & canReceiveCreatures;
		}
		h & heroes;
		h & heroClasses;
		h & players;
		h & allOf;
		h & anyOf;
		h & noneOf;
	}
	
	void serializeJson(JsonSerializeFormat & handler);
};

}

bool DLL_LINKAGE operator== (const Rewardable::Limiter & l, const Rewardable::Limiter & r);
bool DLL_LINKAGE operator!= (const Rewardable::Limiter & l, const Rewardable::Limiter & r);

VCMI_LIB_NAMESPACE_END
