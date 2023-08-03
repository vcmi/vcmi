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

VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;
class CTown;
class CRandomGenerator;
class CHeroClass;
class CGameState;
class CSimpleArmy;

class DLL_LINKAGE TavernHeroesPool
{
	struct TavernSlot
	{
		CGHeroInstance * hero;
		TavernHeroSlot slot;
		TavernSlotRole role;
		PlayerColor player;

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & hero;
			h & slot;
			h & role;
			h & player;
		}
	};

	/// list of all heroes in pool, including those currently present in taverns
	std::map<HeroTypeID, CGHeroInstance* > heroesPool;

	/// list of which players are able to purchase specific hero
	/// if hero is not present in list, he is available for everyone
	std::map<HeroTypeID, PlayerColor::Mask> perPlayerAvailability;

	/// list of heroes currently available in taverns
	std::vector<TavernSlot> currentTavern;

public:
	~TavernHeroesPool();

	/// Returns heroes currently availabe in tavern of a specific player
	std::vector<const CGHeroInstance *> getHeroesFor(PlayerColor color) const;

	/// returns heroes in pool without heroes that are available in taverns
	std::map<HeroTypeID, CGHeroInstance* > unusedHeroesFromPool() const;

	/// Returns true if hero is available to a specific player
	bool isHeroAvailableFor(HeroTypeID hero, PlayerColor color) const;

	TavernSlotRole getSlotRole(HeroTypeID hero) const;

	CGHeroInstance * takeHeroFromPool(HeroTypeID hero);

	/// reset mana and movement points for all heroes in pool
	void onNewDay();

	void addHeroToPool(CGHeroInstance * hero);

	/// Marks hero as available to only specific set of players
	void setAvailability(HeroTypeID hero, PlayerColor::Mask mask);

	/// Makes hero available in tavern of specified player
	void setHeroForPlayer(PlayerColor player, TavernHeroSlot slot, HeroTypeID hero, CSimpleArmy & army, TavernSlotRole role);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & heroesPool;
		h & perPlayerAvailability;
		h & currentTavern;
	}
};

VCMI_LIB_NAMESPACE_END
