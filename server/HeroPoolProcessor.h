/*
 * HeroPoolProcessor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

enum class TavernHeroSlot;
class PlayerColor;
class CGHeroInstance;
class HeroTypeID;
class CGObjectInstance;
class FactionID;
class CRandomGenerator;
class CHeroClass;

VCMI_LIB_NAMESPACE_END

class CGameHandler;

class HeroPoolProcessor : boost::noncopyable
{
	CGameHandler * gameHandler;

	void clearHeroFromSlot(const PlayerColor & color, TavernHeroSlot slot);
	void selectNewHeroForSlot(const PlayerColor & color, TavernHeroSlot slot, bool needNativeHero, bool giveStartingArmy);

	std::set<const CHeroClass *> findAvailableClassesFor(const PlayerColor & player) const;
	std::set<CGHeroInstance *> findAvailableHeroesFor(const PlayerColor & player, const CHeroClass * heroClass) const;

	const CHeroClass * pickClassFor(bool isNative, const PlayerColor & player, const FactionID & faction, CRandomGenerator & rand) const;

	CGHeroInstance * pickHeroFor(bool isNative, const PlayerColor & player, const FactionID & faction, CRandomGenerator & rand, const CHeroClass * bannedClass) const;
public:
	HeroPoolProcessor();
	HeroPoolProcessor(CGameHandler * gameHandler);

	void onHeroSurrendered(const PlayerColor & color, const CGHeroInstance * hero);
	void onHeroEscaped(const PlayerColor & color, const CGHeroInstance * hero);

	void onNewWeek(const PlayerColor & color);

	bool hireHero(const CGObjectInstance *obj, const HeroTypeID & hid, const PlayerColor & player);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & gameHandler;
	}
};
