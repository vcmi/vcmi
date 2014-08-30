#include "StdInc.h"
#include "BattleHex.h"

/*
 * BattleHex.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

BattleHex& BattleHex::moveInDir(EDir dir, bool hasToBeValid)
{
	si16 x = getX(),
		y = getY();

	switch(dir)
	{
	case TOP_LEFT:
		setXY((y%2) ? x-1 : x, y-1, hasToBeValid);
		break;
	case TOP_RIGHT:
		setXY((y%2) ? x : x+1, y-1, hasToBeValid);
		break;
	case RIGHT:
		setXY(x+1, y, hasToBeValid);
		break;
	case BOTTOM_RIGHT:
		setXY((y%2) ? x : x+1, y+1, hasToBeValid);
		break;
	case BOTTOM_LEFT:
		setXY((y%2) ? x-1 : x, y+1, hasToBeValid);
		break;
	case LEFT:
		setXY(x-1, y, hasToBeValid);
		break;
	default:
		throw std::runtime_error("Disaster: wrong direction in BattleHex::operator+=!\n");
		break;
	}

	return *this;
}

std::vector<BattleHex> BattleHex::neighbouringTiles() const
{
	std::vector<BattleHex> ret;
	const int WN = GameConstants::BFIELD_WIDTH;
	// H3 order : TR, R, BR, BL, L, TL (T = top, B = bottom ...)

	checkAndPush(hex - ( (hex/WN)%2 ? WN+1 : WN ), ret); // 1
	checkAndPush(hex + 1, ret); // 2
	checkAndPush(hex + ( (hex/WN)%2 ? WN : WN+1 ), ret); // 3
	checkAndPush(hex + ( (hex/WN)%2 ? WN-1 : WN ), ret); // 4
	checkAndPush(hex - 1, ret); // 5
	checkAndPush(hex - ( (hex/WN)%2 ? WN : WN-1 ), ret); // 6

	return ret;
}

signed char BattleHex::mutualPosition(BattleHex hex1, BattleHex hex2)
{
	if(hex2 == hex1 - ( (hex1/17)%2 ? 18 : 17 )) //top left
		return 0;
	if(hex2 == hex1 - ( (hex1/17)%2 ? 17 : 16 )) //top right
		return 1;
	if(hex2 == hex1 - 1 && hex1%17 != 0) //left
		return 5;
	if(hex2 == hex1 + 1 && hex1%17 != 16) //right
		return 2;
	if(hex2 == hex1 + ( (hex1/17)%2 ? 16 : 17 )) //bottom left
		return 4;
	if(hex2 == hex1 + ( (hex1/17)%2 ? 17 : 18 )) //bottom right
		return 3;
	return -1;
}

char BattleHex::getDistance(BattleHex hex1, BattleHex hex2)
{	
	int y1 = hex1.getY(), 
		y2 = hex2.getY();
	
	// FIXME: Omit floating point arithmetics
	int x1 = (int)(hex1.getX() + y1 * 0.5),
		x2 = (int)(hex2.getX() + y2 * 0.5);

	int xDst = x2 - x1,
		yDst = y2 - y1;

	if ((xDst >= 0 && yDst >= 0) || (xDst < 0 && yDst < 0)) 
		return std::max(std::abs(xDst), std::abs(yDst));
	
	return std::abs(xDst) + std::abs(yDst);
}

void BattleHex::checkAndPush(BattleHex tile, std::vector<BattleHex> & ret)
{
	if(tile.isAvailable())
		ret.push_back(tile);
}

bool BattleHex::isAvailable() const
{
	return isValid() && getX() > 0 && getX() < GameConstants::BFIELD_WIDTH-1;
}

BattleHex BattleHex::getClosestTile(bool attackerOwned, BattleHex initialPos, std::set<BattleHex> & possibilities)
{
	std::vector<BattleHex> sortedTiles (possibilities.begin(), possibilities.end()); //set can't be sorted properly :(

	BattleHex initialHex = BattleHex(initialPos);
	auto compareDistance = [initialHex](const BattleHex left, const BattleHex right) -> bool
	{
		return initialHex.getDistance (initialHex, left) < initialHex.getDistance (initialHex, right);
	};

	boost::sort (sortedTiles, compareDistance); //closest tiles at front

	int closestDistance = initialHex.getDistance(initialPos, sortedTiles.front()); //sometimes closest tiles can be many hexes away

	auto notClosest = [closestDistance, initialPos](const BattleHex here) -> bool
	{
		return closestDistance < here.getDistance (initialPos, here);
	};

	vstd::erase_if(sortedTiles, notClosest); //only closest tiles are interesting

	auto compareHorizontal = [attackerOwned, initialPos](const BattleHex left, const BattleHex right) -> bool
	{
		if(left.getX() != right.getX())
		{
			if (attackerOwned)
				return left.getX() > right.getX(); //find furthest right
			else
				return left.getX() < right.getX(); //find furthest left
		}
		else
		{
			//Prefer tiles in the same row.
			return std::abs(left.getY() - initialPos.getY()) < std::abs(right.getY() - initialPos.getY());
		}
	};

	boost::sort (sortedTiles, compareHorizontal);

	return sortedTiles.front();
}

std::ostream & operator<<(std::ostream & os, const BattleHex & hex)
{
	return os << boost::str(boost::format("{BattleHex: x '%d', y '%d', hex '%d'}") % hex.getX() % hex.getY() % hex.hex);
}
