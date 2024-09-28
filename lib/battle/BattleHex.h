/*
 * BattleHex.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "BattleSide.h"

VCMI_LIB_NAMESPACE_BEGIN

//TODO: change to enum class

namespace GameConstants
{
	const int BFIELD_WIDTH = 17;
	const int BFIELD_HEIGHT = 11;
	const int BFIELD_SIZE = BFIELD_WIDTH * BFIELD_HEIGHT;
}

// for battle stacks' positions
struct DLL_LINKAGE BattleHex //TODO: decide if this should be changed to class for better code design
{
	// helpers for siege
	static constexpr si16 CASTLE_CENTRAL_TOWER = -2;
	static constexpr si16 CASTLE_BOTTOM_TOWER = -3;
	static constexpr si16 CASTLE_UPPER_TOWER = -4;

	// hexes for interaction with heroes
	static constexpr si16 HERO_ATTACKER = 0;
	static constexpr si16 HERO_DEFENDER = GameConstants::BFIELD_WIDTH - 1;

	// helpers for rendering
	static constexpr si16 HEX_BEFORE_ALL = std::numeric_limits<si16>::min();
	static constexpr si16 HEX_AFTER_ALL = std::numeric_limits<si16>::max();

	static constexpr si16 DESTRUCTIBLE_WALL_1 = 29;
	static constexpr si16 DESTRUCTIBLE_WALL_2 = 78;
	static constexpr si16 DESTRUCTIBLE_WALL_3 = 130;
	static constexpr si16 DESTRUCTIBLE_WALL_4 = 182;
	static constexpr si16 GATE_BRIDGE = 94;
	static constexpr si16 GATE_OUTER = 95;
	static constexpr si16 GATE_INNER = 96;

	si16 hex;
	static constexpr si16 INVALID = -1;
	enum EDir
	{
		NONE = -1,

		TOP_LEFT,
		TOP_RIGHT,
		RIGHT,
		BOTTOM_RIGHT,
		BOTTOM_LEFT,
		LEFT,

		//Note: unused by BattleHex class, used by other code
		TOP,
		BOTTOM
	};

	BattleHex();
	BattleHex(si16 _hex);
	BattleHex(si16 x, si16 y);
	BattleHex(std::pair<si16, si16> xy);
	operator si16() const;
	inline bool isValid() const
	{
		return hex >= 0 && hex < GameConstants::BFIELD_SIZE;
	}
	
	bool isAvailable() const //valid position not in first or last column
	{
		return isValid() && getX() > 0 && getX() < GameConstants::BFIELD_WIDTH - 1;
	}

	void setX(si16 x);
	void setY(si16 y);
	void setXY(si16 x, si16 y, bool hasToBeValid = true);
	void setXY(std::pair<si16, si16> xy);
	si16 getX() const;
	si16 getY() const;
	std::pair<si16, si16> getXY() const;
	BattleHex& moveInDirection(EDir dir, bool hasToBeValid = true);
	BattleHex& operator+=(EDir dir);
	BattleHex cloneInDirection(EDir dir, bool hasToBeValid = true) const;
	BattleHex operator+(EDir dir) const;

	static EDir mutualPosition(BattleHex hex1, BattleHex hex2);
	static uint8_t getDistance(BattleHex hex1, BattleHex hex2)
	{
		int y1 = hex1.getY();
		int y2 = hex2.getY();

		// FIXME: why there was * 0.5 instead of / 2?
		int x1 = static_cast<int>(hex1.getX() + y1 / 2);
		int x2 = static_cast<int>(hex2.getX() + y2 / 2);

		int xDst = x2 - x1;
		int yDst = y2 - y1;

		if((xDst >= 0 && yDst >= 0) || (xDst < 0 && yDst < 0))
			return std::max(std::abs(xDst), std::abs(yDst));

		return std::abs(xDst) + std::abs(yDst);
	}
	
	template <typename Handler>
	void serialize(Handler &h)
	{
		h & hex;
	}

	//Constexpr defined array with all directions used in battle
	static constexpr auto hexagonalDirections() {
		return std::array<EDir,6>{BattleHex::TOP_LEFT, BattleHex::TOP_RIGHT, BattleHex::RIGHT, BattleHex::BOTTOM_RIGHT, BattleHex::BOTTOM_LEFT, BattleHex::LEFT};
	}
};

DLL_EXPORT std::ostream & operator<<(std::ostream & os, const BattleHex & hex);

VCMI_LIB_NAMESPACE_END
