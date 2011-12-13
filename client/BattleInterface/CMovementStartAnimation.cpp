#include "StdInc.h"
#include "CMovementStartAnimation.h"

#include "../CMusicHandler.h"
#include "CBattleInterface.h"
#include "../CGameInfo.h"
#include "../CCreatureAnimation.h"
#include "../../lib/BattleState.h"
#include "../CPlayerInterface.h"

bool CMovementStartAnimation::init()
{
	if( !isEarliest(false) )
		return false;


	if(!stack || myAnim()->getType() == 5)
	{
		CMovementStartAnimation::endAnim();
		return false;
	}

	CCS->soundh->playSound(battle_sound(stack->getCreature(), startMoving));
	myAnim()->setType(CCreatureAnim::MOVE_START);

	return true;
}

void CMovementStartAnimation::nextFrame()
{
	if(myAnim()->onLastFrameInGroup())
	{
		endAnim();
	}
	else
	{
		if((owner->animCount+1)%(4/owner->curInt->sysOpts.animSpeed)==0)
			myAnim()->incrementFrame();
	}
}

void CMovementStartAnimation::endAnim()
{
	CBattleAnimation::endAnim();

	delete this;
}

CMovementStartAnimation::CMovementStartAnimation(CBattleInterface * _owner, const CStack * _stack)
: CBattleStackAnimation(_owner, _stack)
{
}