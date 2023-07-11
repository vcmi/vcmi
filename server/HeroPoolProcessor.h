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

VCMI_LIB_NAMESPACE_END

class CGameHandler;

class HeroPoolProcessor : boost::noncopyable
{
	CGameHandler * gameHandler;

	void clearHeroFromSlot(const PlayerColor & color, TavernHeroSlot slot);
	void selectNewHeroForSlot(const PlayerColor & color, TavernHeroSlot slot);
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
