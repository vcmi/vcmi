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

VCMI_LIB_NAMESPACE_BEGIN

//TODO: change to enum class

namespace BattleSide
{
	enum
	{
		ATTACKER = 0,
		DEFENDER = 1
	};
}

namespace GameConstants
{
	const int BFIELD_WIDTH = 17;
	const int BFIELD_HEIGHT = 11;
	const int BFIELD_SIZE = BFIELD_WIDTH * BFIELD_HEIGHT;
}

typedef boost::optional<ui8> BattleSideOpt;

// for battle stacks' positions
struct DLL_LINKAGE BattleHex //TODO: decide if this should be changed to class for better code design
{
	si16 hex;
	static const si16 INVALID = -1;
	enum EDir
	{
		TOP_LEFT,
		TOP_RIGHT,
		RIGHT,
		BOTTOM_RIGHT,
		BOTTOM_LEFT,
		LEFT,
		NONE
	};

	BattleHex();
	BattleHex(si16 _hex);
	BattleHex(si16 x, si16 y);
	BattleHex(std::pair<si16, si16> xy);
	operator si16() const;
	bool isValid() const;
	bool isAvailable() const; //valid position not in first or last column
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
	std::vector<BattleHex> neighbouringTiles() const;
	static signed char mutualPosition(BattleHex hex1, BattleHex hex2);
	static char getDistance(BattleHex hex1, BattleHex hex2);
	static void checkAndPush(BattleHex tile, std::vector<BattleHex> & ret);
	static BattleHex getClosestTile(ui8 side, BattleHex initialPos, std::set<BattleHex> & possibilities); //TODO: vector or set? copying one to another is bad

	template <typename Handler>
	void serialize(Handler &h, const int version)
	{
		h & hex;
	}

    using NeighbouringTiles = std::array<BattleHex, 6>;
    using NeighbouringTilesCache = std::vector<NeighbouringTiles>;

    static const NeighbouringTilesCache neighbouringTilesCache;
};

DLL_EXPORT std::ostream & operator<<(std::ostream & os, const BattleHex & hex);

VCMI_LIB_NAMESPACE_END
