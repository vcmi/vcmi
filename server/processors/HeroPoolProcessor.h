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

#include "../../lib/constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN

enum class TavernHeroSlot : int8_t;
enum class TavernSlotRole : int8_t;
class PlayerColor;
class CGHeroInstance;
class HeroTypeID;
class ObjectInstanceID;
class CHeroClass;
class CRandomGenerator;

namespace vstd
{
class RNG;
}

VCMI_LIB_NAMESPACE_END

class CGameHandler;

class HeroPoolProcessor : boost::noncopyable
{
	CGameHandler * gameHandler;

	/// per-player random generators
	std::map<PlayerColor, std::unique_ptr<CRandomGenerator>> playerSeed;

	void clearHeroFromSlot(const PlayerColor & color, TavernHeroSlot slot);
	void selectNewHeroForSlot(const PlayerColor & color, TavernHeroSlot slot, bool needNativeHero, bool giveStartingArmy, const HeroTypeID & nextHero = HeroTypeID::NONE);

	std::vector<const CHeroClass *> findAvailableClassesFor(const PlayerColor & player) const;
	std::vector<CGHeroInstance *> findAvailableHeroesFor(const PlayerColor & player, const CHeroClass * heroClass) const;

	const CHeroClass * pickClassFor(bool isNative, const PlayerColor & player);

	CGHeroInstance * pickHeroFor(bool isNative, const PlayerColor & player);

	vstd::RNG & getRandomGenerator(const PlayerColor & player);

	TavernHeroSlot selectSlotForRole(const PlayerColor & player, TavernSlotRole roleID);

public:
	HeroPoolProcessor(CGameHandler * gameHandler);

	void onHeroSurrendered(const PlayerColor & color, const CGHeroInstance * hero);
	void onHeroEscaped(const PlayerColor & color, const CGHeroInstance * hero);

	void onNewWeek(const PlayerColor & color);

	/// Incoming net pack handling
	bool hireHero(const ObjectInstanceID & objectID, const HeroTypeID & hid, const PlayerColor & player, const HeroTypeID & nextHero);

	template <typename Handler> void serialize(Handler &h)
	{
		h & playerSeed;
		if (!h.hasFeature(Handler::Version::RANDOMIZATION_REWORK))
		{
			std::map<HeroTypeID, std::unique_ptr<CRandomGenerator>> heroSeedUnused;
			h & heroSeedUnused;
		}

	}
};
