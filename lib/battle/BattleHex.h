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

class BattleHexArray;

// for battle stacks' positions
class DLL_LINKAGE BattleHex
{
public:

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

	BattleHex() 
		: hex(INVALID) 
	{}
	BattleHex(si16 _hex) 
		: hex(_hex) 
	{}
	BattleHex(si16 x, si16 y)
	{
		setXY(x, y);
	}
	BattleHex(std::pair<si16, si16> xy)
	{
		setXY(xy);
	}
	operator si16() const
	{
		return hex;
	}

	inline bool isValid() const
	{
		return hex >= 0 && hex < GameConstants::BFIELD_SIZE;
	}
	
	bool isAvailable() const //valid position not in first or last column
	{
		return isValid() && getX() > 0 && getX() < GameConstants::BFIELD_WIDTH - 1;
	}

	void setX(si16 x)
	{
		setXY(x, getY());
	}

	void setY(si16 y)
	{
		setXY(getX(), y);
	}

	void setXY(si16 x, si16 y, bool hasToBeValid = true)
	{
		if(hasToBeValid)
		{
			if(x < 0 || x >= GameConstants::BFIELD_WIDTH || y < 0 || y >= GameConstants::BFIELD_HEIGHT)
				throw std::runtime_error("Valid hex required");
		}

		hex = x + y * GameConstants::BFIELD_WIDTH;
	}

	void setXY(std::pair<si16, si16> xy)
	{
		setXY(xy.first, xy.second);
	}

	si16 getX() const
	{
		return hex % GameConstants::BFIELD_WIDTH;
	}

	si16 getY() const
	{
		return hex / GameConstants::BFIELD_WIDTH;
	}

	std::pair<si16, si16> getXY() const
	{
		return std::make_pair(getX(), getY());
	}

	BattleHex & moveInDirection(EDir dir, bool hasToBeValid = true)
	{
		si16 x = getX();
		si16 y = getY();
		switch(dir)
		{
		case TOP_LEFT:
			setXY((y % 2) ? x - 1 : x, y - 1, hasToBeValid);
			break;
		case TOP_RIGHT:
			setXY((y % 2) ? x : x + 1, y - 1, hasToBeValid);
			break;
		case RIGHT:
			setXY(x + 1, y, hasToBeValid);
			break;
		case BOTTOM_RIGHT:
			setXY((y % 2) ? x : x + 1, y + 1, hasToBeValid);
			break;
		case BOTTOM_LEFT:
			setXY((y % 2) ? x - 1 : x, y + 1, hasToBeValid);
			break;
		case LEFT:
			setXY(x - 1, y, hasToBeValid);
			break;
		case NONE:
			break;
		default:
			throw std::runtime_error("Disaster: wrong direction in BattleHex::operator+=!\n");
			break;
		}
		return *this;
	}

	BattleHex & operator+=(EDir dir)
	{
		return moveInDirection(dir);
	}

	BattleHex operator+(EDir dir) const
	{
		return cloneInDirection(dir);
	}

	BattleHex cloneInDirection(EDir dir, bool hasToBeValid = true) const
	{
		BattleHex result(hex);
		result.moveInDirection(dir, hasToBeValid);
		return result;
	}

	static uint8_t getDistance(BattleHex hex1, BattleHex hex2)
	{
		int y1 = hex1.getY();
		int y2 = hex2.getY();

		int x1 = hex1.getX() + y1 / 2;
		int x2 = hex2.getX() + y2 / 2;

		int xDst = x2 - x1;
		int yDst = y2 - y1;

		if((xDst >= 0 && yDst >= 0) || (xDst < 0 && yDst < 0))
			return std::max(std::abs(xDst), std::abs(yDst));

		return std::abs(xDst) + std::abs(yDst);
	}

	static BattleHex getClosestTile(const BattleHexArray & hexes, BattleSide side, BattleHex initialPos);

	//Constexpr defined array with all directions used in battle
	static constexpr auto hexagonalDirections() 
	{
		return std::array<EDir,6>{TOP_LEFT, TOP_RIGHT, RIGHT, BOTTOM_RIGHT, BOTTOM_LEFT, LEFT};
	}

	static EDir mutualPosition(BattleHex hex1, BattleHex hex2)
	{
		for(auto dir : hexagonalDirections())
			if(hex2 == hex1.cloneInDirection(dir, false))
				return dir;
		return NONE;
	}

	/// get (precomputed) all possible surrounding tiles
	const BattleHexArray & getAllNeighbouringTiles() const;

	/// get (precomputed) only valid and available surrounding tiles
	const BattleHexArray & getNeighbouringTiles() const;

	/// get (precomputed) only valid and available surrounding tiles for double wide creatures
	const BattleHexArray & getNeighbouringTilesDblWide(BattleSide side) const;

	template <typename Handler>
	void serialize(Handler & h)
	{
		h & hex;
	}

private:

	si16 hex;
};

DLL_EXPORT std::ostream & operator<<(std::ostream & os, const BattleHex & hex);

VCMI_LIB_NAMESPACE_END
