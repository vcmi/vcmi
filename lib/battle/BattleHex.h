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

/**
 * @brief Represents a battlefield hexagonal tile.
 *
 * Valid hexes are within the range 0 to 186, excluding some invalid values, ex. castle towers (-2, -3, -4).
 * Available hexes are those valid ones but NOT in the first or last column.
 */
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

	BattleHex() noexcept
		: hex(INVALID) 
	{}
	BattleHex(si16 _hex) noexcept
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

	[[nodiscard]] bool isValid() const noexcept
	{
		return hex >= 0 && hex < GameConstants::BFIELD_SIZE;
	}
	
	[[nodiscard]] bool isAvailable() const noexcept //valid position not in first or last column
	{
		return isValid() && getX() > 0 && getX() < GameConstants::BFIELD_WIDTH - 1;
	}

	[[nodiscard]] inline bool isTower() const noexcept
	{
		return hex == BattleHex::CASTLE_CENTRAL_TOWER || hex == BattleHex::CASTLE_UPPER_TOWER || hex == BattleHex::CASTLE_BOTTOM_TOWER;
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
				throw std::out_of_range("Hex at (" + std::to_string(x) + ", " + std::to_string(y) + ") is not valid!");
		}

		hex = x + y * GameConstants::BFIELD_WIDTH;
	}

	void setXY(std::pair<si16, si16> xy)
	{
		setXY(xy.first, xy.second);
	}

	[[nodiscard]] si16 getX() const noexcept
	{
		return hex % GameConstants::BFIELD_WIDTH;
	}

	[[nodiscard]] si16 getY() const noexcept
	{
		return hex / GameConstants::BFIELD_WIDTH;
	}

	[[nodiscard]] std::pair<si16, si16> getXY() const noexcept
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

	[[nodiscard]] BattleHex cloneInDirection(EDir dir, bool hasToBeValid = true) const
	{
		BattleHex result(hex);
		result.moveInDirection(dir, hasToBeValid);
		return result;
	}

	[[nodiscard]] static uint8_t getDistance(const BattleHex & hex1, const BattleHex & hex2) noexcept
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

	[[nodiscard]] static BattleHex getClosestTile(BattleSide side, const BattleHex & initialPos, const BattleHexArray & hexes);

	//Constexpr defined array with all directions used in battle
	[[nodiscard]] static constexpr auto hexagonalDirections() noexcept
	{
		return std::array<EDir,6>{TOP_LEFT, TOP_RIGHT, RIGHT, BOTTOM_RIGHT, BOTTOM_LEFT, LEFT};
	}

	[[nodiscard]] static EDir mutualPosition(const BattleHex & hex1, const BattleHex & hex2)
	{
		for(auto dir : hexagonalDirections())
			if(hex2 == hex1.cloneInDirection(dir, false))
				return dir;
		return NONE;
	}

	/// get (precomputed) all possible surrounding tiles
	[[nodiscard]] const BattleHexArray & getAllNeighbouringTiles() const noexcept;

	/// get (precomputed) only valid and available surrounding tiles
	[[nodiscard]] const BattleHexArray & getNeighbouringTiles() const noexcept;

	/// get (precomputed) only valid and available surrounding tiles for double wide creatures
	[[nodiscard]] const BattleHexArray & getNeighbouringTilesDoubleWide(BattleSide side) const noexcept;

	/// get integer hex value
	[[nodiscard]] si16 toInt() const noexcept
	{
		return hex;
	}

	BattleHex & operator+=(EDir dir)
	{
		return moveInDirection(dir);
	}

	[[nodiscard]] BattleHex operator+(EDir dir) const
	{
		return cloneInDirection(dir);
	}

	// Prefix increment
	BattleHex & operator++() noexcept
	{
		++hex;
		return *this;
	}

	// Postfix increment is deleted; use prefix increment as it has better performance
	BattleHex operator++(int) = delete;

	[[nodiscard]] bool operator ==(const BattleHex & other) const noexcept
	{
		return hex == other.hex;
	}

	[[nodiscard]] bool operator !=(const BattleHex & other) const noexcept
	{
		return hex != other.hex;
	}

	[[nodiscard]] bool operator <(const BattleHex & other) const noexcept
	{
		return hex < other.hex;
	}

	[[nodiscard]] bool operator <=(const BattleHex & other) const noexcept
	{
		return hex <= other.hex;
	}


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
