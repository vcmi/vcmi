#include "StdInc.h"
#include "CBattleStackAnimation.h"

#include "CBattleInterface.h"
#include "../../lib/BattleState.h"

CBattleStackAnimation::CBattleStackAnimation(CBattleInterface * _owner, const CStack * _stack)
: CBattleAnimation(_owner), stack(_stack)
{
}

bool CBattleStackAnimation::isToReverseHlp(SBattleHex hexFrom, SBattleHex hexTo, bool curDir)
{
	int fromMod = hexFrom % GameConstants::BFIELD_WIDTH;
	int fromDiv = hexFrom / GameConstants::BFIELD_WIDTH;
	int toMod = hexTo % GameConstants::BFIELD_WIDTH;

	if(curDir && fromMod < toMod)
		return false;
	else if(curDir && fromMod > toMod)
		return true;
	else if(curDir && fromMod == toMod)
	{
		return fromDiv % 2 == 0;
	}
	else if(!curDir && fromMod < toMod)
		return true;
	else if(!curDir && fromMod > toMod)
		return false;
	else if(!curDir && fromMod == toMod)
	{
		return fromDiv % 2 == 1;
	}
	tlog1 << "Catastrope in CBattleStackAnimation::isToReverse!" << std::endl;
	return false; //should never happen
}

bool CBattleStackAnimation::isToReverse(SBattleHex hexFrom, SBattleHex hexTo, bool curDir, bool toDoubleWide, bool toDir)
{
	if(hexTo < 0) //turret
		return false;

	if(toDoubleWide)
	{
		return isToReverseHlp(hexFrom, hexTo, curDir) &&
			(toDir ? isToReverseHlp(hexFrom, hexTo-1, curDir) : isToReverseHlp(hexFrom, hexTo+1, curDir) );
	}
	else
	{
		return isToReverseHlp(hexFrom, hexTo, curDir);
	}
}

CCreatureAnimation* CBattleStackAnimation::myAnim()
{
	return owner->creAnims[stack->ID];
}