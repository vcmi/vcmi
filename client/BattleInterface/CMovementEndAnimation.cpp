#include "StdInc.h"
#include "CMovementEndAnimation.h"

#include "CCreatureAnimation.h"
#include "../CMusicHandler.h"
#include "../CGameInfo.h"
#include "../../lib/BattleState.h"
#include "../CCursorHandler.h"

bool CMovementEndAnimation::init()
{
	if( !isEarliest(true) )
		return false;

	if(!stack || myAnim()->framesInGroup(CCreatureAnim::MOVE_END) == 0 ||
		myAnim()->getType() == CCreatureAnim::DEATH)
	{
		endAnim();

		return false;
	}

	CCS->soundh->playSound(battle_sound(stack->getCreature(), endMoving));

	myAnim()->setType(CCreatureAnim::MOVE_END);

	return true;
}

void CMovementEndAnimation::nextFrame()
{
	if(myAnim()->onLastFrameInGroup())
	{
		endAnim();
	}
}

void CMovementEndAnimation::endAnim()
{
	CBattleAnimation::endAnim();

	if(myAnim()->getType() != CCreatureAnim::DEATH)
		myAnim()->setType(CCreatureAnim::HOLDING); //resetting to default

	CCS->curh->show();
	delete this;
}

CMovementEndAnimation::CMovementEndAnimation(CBattleInterface * _owner, const CStack * _stack, SBattleHex destTile)
: CBattleStackAnimation(_owner, _stack), destinationTile(destTile)
{
}