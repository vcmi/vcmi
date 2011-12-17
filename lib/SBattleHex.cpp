#include "StdInc.h"
#include "SBattleHex.h"

void SBattleHex::operator+=(EDir dir)
{
	si16 x = getX(),
		y = getY();

	switch(dir)
	{
	case TOP_LEFT:
		setXY(y%2 ? x-1 : x, y-1);
		break;
	case TOP_RIGHT:
		setXY(y%2 ? x : x+1, y-1);
		break;
	case RIGHT:
		setXY(x+1, y);
		break;
	case BOTTOM_RIGHT:
		setXY(y%2 ? x : x+1, y+1);
		break;
	case BOTTOM_LEFT:
		setXY(y%2 ? x-1 : x, y+1);
		break;
	case LEFT:
		setXY(x-1, y);
		break;
	default:
		throw std::string("Disaster: wrong direction in SBattleHex::operator+=!\n");
		break;
	}
}

SBattleHex SBattleHex::operator+(EDir dir) const
{
	SBattleHex ret(*this);
	ret += dir;
	return ret;
}

std::vector<SBattleHex> SBattleHex::neighbouringTiles() const
{
	std::vector<SBattleHex> ret;
	const int WN = GameConstants::BFIELD_WIDTH;
	checkAndPush(hex - ( (hex/WN)%2 ? WN+1 : WN ), ret);
	checkAndPush(hex - ( (hex/WN)%2 ? WN : WN-1 ), ret);
	checkAndPush(hex - 1, ret);
	checkAndPush(hex + 1, ret);
	checkAndPush(hex + ( (hex/WN)%2 ? WN-1 : WN ), ret);
	checkAndPush(hex + ( (hex/WN)%2 ? WN : WN+1 ), ret);

	return ret;
}

signed char SBattleHex::mutualPosition(SBattleHex hex1, SBattleHex hex2)
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

char SBattleHex::getDistance(SBattleHex hex1, SBattleHex hex2)
{
	int xDst = std::abs(hex1 % GameConstants::BFIELD_WIDTH - hex2 % GameConstants::BFIELD_WIDTH),
		yDst = std::abs(hex1 / GameConstants::BFIELD_WIDTH - hex2 / GameConstants::BFIELD_WIDTH);
	return std::max(xDst, yDst) + std::min(xDst, yDst) - (yDst + 1)/2;
}

void SBattleHex::checkAndPush(int tile, std::vector<SBattleHex> & ret)
{
	if( tile>=0 && tile<GameConstants::BFIELD_SIZE && (tile%GameConstants::BFIELD_WIDTH != (GameConstants::BFIELD_WIDTH - 1)) 
		&& (tile%GameConstants::BFIELD_WIDTH != 0) )
		ret.push_back(SBattleHex(tile));
}