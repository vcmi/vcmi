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
	NATIVE,
	RANDOM
};

class DLL_LINKAGE TavernHeroesPool
{
	CGameState * gameState;

	//[subID] - heroes available to buy; nullptr if not available
	std::map<HeroTypeID, CGHeroInstance* > heroesPool;

	// [subid] -> which players can recruit hero (binary flags)
	std::map<HeroTypeID, PlayerColor::Mask> pavailable;

	std::map<HeroTypeID, CGHeroInstance* > unusedHeroesFromPool(); //heroes pool without heroes that are available in taverns

	std::map<PlayerColor, std::map<TavernHeroSlot, CGHeroInstance*> > currentTavern;

	bool isHeroAvailableFor(HeroTypeID hero, PlayerColor color) const;
public:
	TavernHeroesPool();
	TavernHeroesPool(CGameState * gameState);
	~TavernHeroesPool();

	CGHeroInstance * pickHeroFor(TavernHeroSlot slot,
								 const PlayerColor & player,
								 const FactionID & faction,
								 CRandomGenerator & rand,
								 const CHeroClass * bannedClass = nullptr) const;

	std::vector<const CGHeroInstance *> getHeroesFor(PlayerColor color) const;

	CGHeroInstance * takeHero(HeroTypeID hero);

	/// reset mana and movement points for all heroes in pool
	void onNewDay();

	void addHeroToPool(CGHeroInstance * hero);
	void setAvailability(HeroTypeID hero, PlayerColor::Mask mask);
	void setHeroForPlayer(PlayerColor player, TavernHeroSlot slot, HeroTypeID hero, CSimpleArmy & army);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & gameState;
		h & heroesPool;
		h & pavailable;
	}
};

VCMI_LIB_NAMESPACE_END
