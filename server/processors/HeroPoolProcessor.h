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

enum class TavernHeroSlot : int8_t;
enum class TavernSlotRole : int8_t;
class PlayerColor;
class CGHeroInstance;
class HeroTypeID;
class ObjectInstanceID;
class CRandomGenerator;
class CHeroClass;

VCMI_LIB_NAMESPACE_END

class CGameHandler;

class HeroPoolProcessor : boost::noncopyable
{
	/// per-player random generators
	std::map<PlayerColor, std::unique_ptr<CRandomGenerator>> playerSeed;

	/// per-hero random generators used to randomize skills
	std::map<HeroTypeID, std::unique_ptr<CRandomGenerator>> heroSeed;

	void clearHeroFromSlot(const PlayerColor & color, TavernHeroSlot slot);
	void selectNewHeroForSlot(const PlayerColor & color, TavernHeroSlot slot, bool needNativeHero, bool giveStartingArmy);

	std::vector<const CHeroClass *> findAvailableClassesFor(const PlayerColor & player) const;
	std::vector<CGHeroInstance *> findAvailableHeroesFor(const PlayerColor & player, const CHeroClass * heroClass) const;

	const CHeroClass * pickClassFor(bool isNative, const PlayerColor & player);

	CGHeroInstance * pickHeroFor(bool isNative, const PlayerColor & player);

	CRandomGenerator & getRandomGenerator(const PlayerColor & player);

	TavernHeroSlot selectSlotForRole(const PlayerColor & player, TavernSlotRole roleID);

public:
	CGameHandler * gameHandler;

	HeroPoolProcessor();
	HeroPoolProcessor(CGameHandler * gameHandler);

	void onHeroSurrendered(const PlayerColor & color, const CGHeroInstance * hero);
	void onHeroEscaped(const PlayerColor & color, const CGHeroInstance * hero);

	void onNewWeek(const PlayerColor & color);

	CRandomGenerator & getHeroSkillsRandomGenerator(const HeroTypeID & hero);

	/// Incoming net pack handling
	bool hireHero(const ObjectInstanceID & objectID, const HeroTypeID & hid, const PlayerColor & player);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		// h & gameHandler; // FIXME: make this work instead of using deserializationFix in gameHandler
		h & playerSeed;
		h & heroSeed;
	}
};
