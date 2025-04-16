/*
 * TavernHeroesPool.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../GameConstants.h"
#include "TavernSlot.h"
#include "../mapObjects/CGObjectInstance.h"
#include "../serializer/Serializeable.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;
class CTown;
class CHeroClass;
class CGameState;
class CSimpleArmy;

class DLL_LINKAGE TavernHeroesPool : public Serializeable
{
	CGameState * owner;

	struct TavernSlot
	{
		HeroTypeID hero;
		TavernHeroSlot slot;
		TavernSlotRole role;
		PlayerColor player;

		template <typename Handler> void serialize(Handler &h)
		{
			if (h.hasFeature(Handler::Version::NO_RAW_POINTERS_IN_SERIALIZER))
			{
				h & hero;
			}
			else
			{
				std::shared_ptr<CGObjectInstance> pointer;
				h & pointer;
				hero = HeroTypeID(pointer->subID);
			}

			h & slot;
			h & role;
			h & player;
		}
	};

	/// list of all heroes in pool, including those currently present in taverns
	std::vector<HeroTypeID> heroesPool;

	/// list of which players are able to purchase specific hero
	/// if hero is not present in list, he is available for everyone
	std::map<HeroTypeID, std::set<PlayerColor>> perPlayerAvailability;

	/// list of heroes currently available in taverns
	std::vector<TavernSlot> currentTavern;

public:
	TavernHeroesPool() = default;
	TavernHeroesPool(CGameState * owner);

	void setGameState(CGameState * owner);

	/// Returns heroes currently available in tavern of a specific player
	std::vector<const CGHeroInstance *> getHeroesFor(PlayerColor color) const;

	/// returns heroes in pool without heroes that are available in taverns
	std::map<HeroTypeID, CGHeroInstance* > unusedHeroesFromPool() const;

	/// Returns true if hero is available to a specific player
	bool isHeroAvailableFor(HeroTypeID hero, PlayerColor color) const;

	TavernSlotRole getSlotRole(HeroTypeID hero) const;

	std::shared_ptr<CGHeroInstance> takeHeroFromPool(HeroTypeID hero);

	/// reset mana and movement points for all heroes in pool
	void onNewDay();

	void addHeroToPool(HeroTypeID hero);

	/// Marks hero as available to only specific set of players
	void setAvailability(HeroTypeID hero, std::set<PlayerColor> mask);

	/// Makes hero available in tavern of specified player
	void setHeroForPlayer(PlayerColor player, TavernHeroSlot slot, HeroTypeID hero, CSimpleArmy & army, TavernSlotRole role, bool replenishPoints);

	template <typename Handler> void serialize(Handler &h)
	{
		if (h.hasFeature(Handler::Version::NO_RAW_POINTERS_IN_SERIALIZER))
			h & heroesPool;
		else
		{
			std::map<HeroTypeID, std::shared_ptr<CGObjectInstance>> objectPtrs;
			h & objectPtrs;
			for (const auto & ptr : objectPtrs)
				heroesPool.push_back(ptr.first);
		}
		h & perPlayerAvailability;
		h & currentTavern;
	}
};

VCMI_LIB_NAMESPACE_END
