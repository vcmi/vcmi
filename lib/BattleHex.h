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
#include "GameConstants.h"

// for battle stacks' positions
struct DLL_LINKAGE BattleHex //TODO: decide if this should be changed to class for better code design
{
	si16 hex;
	static const si16 INVALID = -1;
	enum EDir { RIGHT, BOTTOM_RIGHT, BOTTOM_LEFT, LEFT, TOP_LEFT, TOP_RIGHT };

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
	//moving to direction
	BattleHex& moveInDir(EDir dir, bool hasToBeValid = true);
	BattleHex& operator+=(EDir dir); //sugar for above
	//generates new BattleHex moved by given dir
	BattleHex movedInDir(EDir dir, bool hasToBeValid = true) const;
	BattleHex operator+(EDir dir) const;
	std::vector<BattleHex> neighbouringTiles() const;
	//returns info about mutual position of given hexes (-1 - they're distant, 0 - left top, 1 - right top, 2 - right, 3 - right bottom, 4 - left bottom, 5 - left)
	static signed char mutualPosition(BattleHex hex1, BattleHex hex2);
	//returns distance between given hexes
	static char getDistance(BattleHex hex1, BattleHex hex2);
	static void checkAndPush(BattleHex tile, std::vector<BattleHex> & ret);
	static BattleHex getClosestTile(bool attackerOwned, BattleHex initialPos, std::set<BattleHex> & possibilities); //TODO: vector or set? copying one to another is bad

	template <typename Handler>
	void serialize(Handler &h, const int version)
	{
		h & hex;
	}
};

DLL_EXPORT std::ostream & operator<<(std::ostream & os, const BattleHex & hex);
