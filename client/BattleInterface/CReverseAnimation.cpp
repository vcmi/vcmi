#include "StdInc.h"
#include "CReverseAnimation.h"

#include "../CCreatureAnimation.h"
#include "../../lib/BattleState.h"
#include "CBattleInterface.h"
#include "CHexFieldControl.h"

bool CReverseAnimation::init()
{
	if(myAnim() == NULL || myAnim()->getType() == 5)
	{
		endAnim();

		return false; //there is no such creature
	}

	if(!priority && !isEarliest(false))
		return false;

	if(myAnim()->framesInGroup(CCreatureAnim::TURN_R))
		myAnim()->setType(CCreatureAnim::TURN_R);
	else
		setupSecondPart();


	return true;
}

void CReverseAnimation::nextFrame()
{
	if(partOfAnim == 1) //first part of animation
	{
		if(myAnim()->onLastFrameInGroup())
		{
			partOfAnim = 2;
		}
	}
	else if(partOfAnim == 2)
	{
		if(!secondPartSetup)
		{
			setupSecondPart();
		}
		if(myAnim()->onLastFrameInGroup())
		{
			endAnim();
		}
	}
}

void CReverseAnimation::endAnim()
{
	CBattleAnimation::endAnim();
	if( stack->alive() )//don't do that if stack is dead
		myAnim()->setType(CCreatureAnim::HOLDING);

	delete this;
}

CReverseAnimation::CReverseAnimation(CBattleInterface * _owner, const CStack * stack, SHexField dest, bool _priority)
: CBattleStackAnimation(_owner, stack), partOfAnim(1), secondPartSetup(false), hex(dest), priority(_priority)
{
}

void CReverseAnimation::setupSecondPart()
{
	owner->creDir[stack->ID] = !owner->creDir[stack->ID];

	if(!stack)
	{
		endAnim();
		return;
	}

	Point coords = CHexFieldControl::getXYUnitAnim(hex, owner->creDir[stack->ID], stack, owner);
	myAnim()->pos.x = coords.x;
	//creAnims[stackID]->pos.y = coords.second;

	if(stack->doubleWide())
	{
		if(stack->attackerOwned)
		{
			if(!owner->creDir[stack->ID])
				myAnim()->pos.x -= 44;
		}
		else
		{
			if(owner->creDir[stack->ID])
				myAnim()->pos.x += 44;
		}
	}

	secondPartSetup = true;

	if(myAnim()->framesInGroup(CCreatureAnim::TURN_L))
		myAnim()->setType(CCreatureAnim::TURN_L);
	else
		endAnim();
}