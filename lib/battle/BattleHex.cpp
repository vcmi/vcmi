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

BattleHex & BattleHex::moveInDirection(EDir dir, bool hasToBeValid)
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

BattleHex & BattleHex::operator+=(BattleHex::EDir dir)
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

BattleHex::EDir BattleHex::mutualPosition(BattleHex hex1, BattleHex hex2)
{
	for(auto dir : hexagonalDirections())
		if(hex2 == hex1.cloneInDirection(dir, false))
			return dir;
	return NONE;
}

std::ostream & operator<<(std::ostream & os, const BattleHex & hex)
{
	return os << boost::str(boost::format("{BattleHex: x '%d', y '%d', hex '%d'}") % hex.getX() % hex.getY() % hex.hex);
}

VCMI_LIB_NAMESPACE_END
