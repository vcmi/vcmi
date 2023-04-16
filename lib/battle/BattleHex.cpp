/*
 * BattleHex.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleHex.h"

VCMI_LIB_NAMESPACE_BEGIN

BattleHex::BattleHex() : hex(INVALID) {}

BattleHex::BattleHex(si16 _hex) : hex(_hex) {}

BattleHex::BattleHex(si16 x, si16 y)
{
	setXY(x, y);
}

BattleHex::BattleHex(std::pair<si16, si16> xy)
{
	setXY(xy);
}

BattleHex::operator si16() const
{
	return hex;
}

bool BattleHex::isValid() const
{
	return hex >= 0 && hex < GameConstants::BFIELD_SIZE;
}

bool BattleHex::isAvailable() const
{
	return isValid() && getX() > 0 && getX() < GameConstants::BFIELD_WIDTH-1;
}

void BattleHex::setX(si16 x)
{
	setXY(x, getY());
}

void BattleHex::setY(si16 y)
{
	setXY(getX(), y);
}

void BattleHex::setXY(si16 x, si16 y, bool hasToBeValid)
{
	if(hasToBeValid)
	{
		if(x < 0 || x >= GameConstants::BFIELD_WIDTH || y < 0 || y >= GameConstants::BFIELD_HEIGHT)
			throw std::runtime_error("Valid hex required");
	}

	hex = x + y * GameConstants::BFIELD_WIDTH;
}

void BattleHex::setXY(std::pair<si16, si16> xy)
{
	setXY(xy.first, xy.second);
}

si16 BattleHex::getX() const
{
	return hex % GameConstants::BFIELD_WIDTH;
}

si16 BattleHex::getY() const
{
	return hex / GameConstants::BFIELD_WIDTH;
}

std::pair<si16, si16> BattleHex::getXY() const
{
	return std::make_pair(getX(), getY());
}

BattleHex& BattleHex::moveInDirection(EDir dir, bool hasToBeValid)
{
	si16 x = getX();
	si16 y = getY();
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
	case NONE:
		break;
	default:
		throw std::runtime_error("Disaster: wrong direction in BattleHex::operator+=!\n");
		break;
	}
	return *this;
}

BattleHex &BattleHex::operator+=(BattleHex::EDir dir)
{
	return moveInDirection(dir);
}

BattleHex BattleHex::cloneInDirection(BattleHex::EDir dir, bool hasToBeValid) const
{
	BattleHex result(hex);
	result.moveInDirection(dir, hasToBeValid);
	return result;
}

BattleHex BattleHex::operator+(BattleHex::EDir dir) const
{
	return cloneInDirection(dir);
}

std::vector<BattleHex> BattleHex::neighbouringTiles() const
{
	std::vector<BattleHex> ret;
	ret.reserve(6);
	for(auto dir : hexagonalDirections())
		checkAndPush(cloneInDirection(dir, false), ret);
	return ret;
}

std::vector<BattleHex> BattleHex::allNeighbouringTiles() const
{
	std::vector<BattleHex> ret;
	ret.resize(6);

	for(auto dir : hexagonalDirections())
		ret[dir] = cloneInDirection(dir, false);

	return ret;
}

BattleHex::EDir BattleHex::mutualPosition(BattleHex hex1, BattleHex hex2)
{
	for(auto dir : hexagonalDirections())
		if(hex2 == hex1.cloneInDirection(dir, false))
			return dir;
	return NONE;
}

uint8_t BattleHex::getDistance(BattleHex hex1, BattleHex hex2)
{
	int y1 = hex1.getY();
	int y2 = hex2.getY();

	// FIXME: why there was * 0.5 instead of / 2?
	int x1 = static_cast<int>(hex1.getX() + y1 / 2);
	int x2 = static_cast<int>(hex2.getX() + y2 / 2);

	int xDst = x2 - x1;
	int yDst = y2 - y1;

	if ((xDst >= 0 && yDst >= 0) || (xDst < 0 && yDst < 0))
		return std::max(std::abs(xDst), std::abs(yDst));

	return std::abs(xDst) + std::abs(yDst);
}

void BattleHex::checkAndPush(BattleHex tile, std::vector<BattleHex> & ret)
{
	if(tile.isAvailable())
		ret.push_back(tile);
}

BattleHex BattleHex::getClosestTile(ui8 side, BattleHex initialPos, std::set<BattleHex> & possibilities)
{
	std::vector<BattleHex> sortedTiles (possibilities.begin(), possibilities.end()); //set can't be sorted properly :(
	auto initialHex = BattleHex(initialPos);
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
	auto compareHorizontal = [side, initialPos](const BattleHex left, const BattleHex right) -> bool
	{
		if(left.getX() != right.getX())
		{
			if(side == BattleSide::ATTACKER)
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

static BattleHex::NeighbouringTilesCache calculateNeighbouringTiles()
{
	BattleHex::NeighbouringTilesCache ret;
	ret.resize(GameConstants::BFIELD_SIZE);

	for(si16 hex = 0; hex < GameConstants::BFIELD_SIZE; hex++)
	{
		auto hexes = BattleHex(hex).neighbouringTiles();

		size_t index = 0;
		for(auto neighbour : hexes)
			ret[hex].at(index++) = neighbour;
	}

	return ret;
}

const BattleHex::NeighbouringTilesCache BattleHex::neighbouringTilesCache = calculateNeighbouringTiles();

VCMI_LIB_NAMESPACE_END
