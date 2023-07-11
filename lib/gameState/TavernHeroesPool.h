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

#include "../ConstTransitivePtr.h"
#include "../GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;
class CTown;
class CRandomGenerator;
class CHeroClass;
class CGameState;
class CSimpleArmy;

enum class TavernHeroSlot
{
	NATIVE, // 1st / left slot in tavern, contains hero native to player's faction on new week
	RANDOM  // 2nd / right slot in tavern, contains hero of random class
};

class DLL_LINKAGE TavernHeroesPool
{
	/// list of all heroes in pool, including those currently present in taverns
	std::map<HeroTypeID, CGHeroInstance* > heroesPool;

	/// list of which players are able to purchase specific hero
	/// if hero is not present in list, he is available for everyone
	std::map<HeroTypeID, PlayerColor::Mask> pavailable;

	/// list of heroes currently available in a tavern of a specific player
	std::map<PlayerColor, std::map<TavernHeroSlot, CGHeroInstance*> > currentTavern;

public:
	~TavernHeroesPool();

	/// Returns heroes currently availabe in tavern of a specific player
	std::vector<const CGHeroInstance *> getHeroesFor(PlayerColor color) const;

	/// returns heroes in pool without heroes that are available in taverns
	std::map<HeroTypeID, CGHeroInstance* > unusedHeroesFromPool() const;

	/// Returns true if hero is available to a specific player
	bool isHeroAvailableFor(HeroTypeID hero, PlayerColor color) const;

	CGHeroInstance * takeHero(HeroTypeID hero);

	/// reset mana and movement points for all heroes in pool
	void onNewDay();

	void addHeroToPool(CGHeroInstance * hero);
	void setAvailability(HeroTypeID hero, PlayerColor::Mask mask);
	void setHeroForPlayer(PlayerColor player, TavernHeroSlot slot, HeroTypeID hero, CSimpleArmy & army);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & heroesPool;
		h & pavailable;
	}
};

VCMI_LIB_NAMESPACE_END
