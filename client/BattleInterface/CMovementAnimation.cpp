#include "StdInc.h"
#include "CMovementAnimation.h"

#include "CBattleInterface.h"
#include "CCreatureAnimation.h"
#include "../../lib/BattleState.h"
#include "../CGameInfo.h"
#include "../CMusicHandler.h"
#include "CReverseAnimation.h"
#include "CMovementEndAnimation.h"
#include "CClickableHex.h"

bool CMovementAnimation::init()
{
	if( !isEarliest(false) )
		return false;

	//a few useful variables
	steps = static_cast<int>(myAnim()->framesInGroup(CCreatureAnim::MOVING) * owner->getAnimSpeedMultiplier() - 1);
	if(steps == 0) //this creature seems to have no move animation so we can end it immediately
	{
		endAnim();
		return false;
	}
	whichStep = 0;
	int hexWbase = 44, hexHbase = 42;
	const CStack * movedStack = stack;
	if(!movedStack || myAnim()->getType() == 5)
	{
		endAnim();
		return false;
	}
	//bool twoTiles = movedStack->doubleWide();

	SPoint begPosition = CClickableHex::getXYUnitAnim(curStackPos, movedStack->attackerOwned, movedStack, owner);
	SPoint endPosition = CClickableHex::getXYUnitAnim(nextHex, movedStack->attackerOwned, movedStack, owner);

	int mutPos = SBattleHex::mutualPosition(curStackPos, nextHex);

	//reverse unit if necessary
	if((begPosition.x > endPosition.x) && owner->creDir[stack->ID] == true)
	{
		owner->addNewAnim(new CReverseAnimation(owner, stack, curStackPos, true));
		return false;
	}
	else if ((begPosition.x < endPosition.x) && owner->creDir[stack->ID] == false)
	{
		owner->addNewAnim(new CReverseAnimation(owner, stack, curStackPos, true));
		return false;
	}

	if(myAnim()->getType() != CCreatureAnim::MOVING)
	{
		myAnim()->setType(CCreatureAnim::MOVING);
	}
	//unit reversed

	//	if(owner->moveSh <= 0)
	//		owner->moveSh = CCS->soundh->playSound(battle_sound(movedStack->getCreature(), move), -1);

	//step shift calculation
	posX = myAnim()->pos.x, posY = myAnim()->pos.y; // for precise calculations ;]
	if(mutPos == -1 && movedStack->hasBonusOfType(Bonus::FLYING)) 
	{
		steps *= distance;
		steps /= 2; //to make animation faster

		stepX = (endPosition.x - begPosition.x) / static_cast<double>(steps);
		stepY = (endPosition.y - begPosition.y) / static_cast<double>(steps);
	}
	else
	{
		switch(mutPos)
		{
		case 0:
			stepX = -1.0 * (hexWbase / (2.0 * steps));
			stepY = -1.0 * (hexHbase / (static_cast<double>(steps)));
			break;
		case 1:
			stepX = hexWbase / (2.0 * steps);
			stepY = -1.0 * hexHbase / (static_cast<double>(steps));
			break;
		case 2:
			stepX = hexWbase / static_cast<double>(steps);
			stepY = 0.0;
			break;
		case 3:
			stepX = hexWbase / (2.0 * steps);
			stepY = hexHbase / static_cast<double>(steps);
			break;
		case 4:
			stepX = -1.0 * hexWbase / (2.0 * steps);
			stepY = hexHbase / static_cast<double>(steps);
			break;
		case 5:
			stepX = -1.0 * hexWbase / static_cast<double>(steps);
			stepY = 0.0;
			break;
		}
	}
	//step shifts calculated

	return true;
}

void CMovementAnimation::nextFrame()
{
	//moving instructions
	posX += stepX;
	myAnim()->pos.x = static_cast<Sint16>(posX);
	posY += stepY;
	myAnim()->pos.y = static_cast<Sint16>(posY);

	// Increments step count and check if we are finished with current animation
	++whichStep;
	if(whichStep == steps)
	{
		// Sets the position of the creature animation sprites
		SPoint coords = CClickableHex::getXYUnitAnim(nextHex, owner->creDir[stack->ID], stack, owner);
		myAnim()->pos = coords;

		// true if creature haven't reached the final destination hex
		if ((nextPos + 1) < destTiles.size())
		{
			// update the next hex field which has to be reached by the stack
			nextPos++;
			curStackPos = nextHex;
			nextHex = destTiles[nextPos];

			// update position of double wide creatures
			bool twoTiles = stack->doubleWide();
			if(twoTiles && bool(stack->attackerOwned) && (owner->creDir[stack->ID] != bool(stack->attackerOwned) )) //big attacker creature is reversed
				myAnim()->pos.x -= 44;
			else if(twoTiles && (! bool(stack->attackerOwned) ) && (owner->creDir[stack->ID] != bool(stack->attackerOwned) )) //big defender creature is reversed
				myAnim()->pos.x += 44;

			// re-init animation
			for(std::list<std::pair<CBattleAnimation *, bool> >::iterator it = owner->pendingAnims.begin(); it != owner->pendingAnims.end(); ++it)
			{
				if (it->first == this)
				{
					it->second = false;
					break;
				}
			}
		}
		else
			endAnim();
	}
}

void CMovementAnimation::endAnim()
{
	const CStack * movedStack = stack;

	CBattleAnimation::endAnim();

	if(movedStack)
		owner->addNewAnim(new CMovementEndAnimation(owner, stack, nextHex));


	if(owner->moveSh >= 0)
	{
		CCS->soundh->stopSound(owner->moveSh);
		owner->moveSh = -1;
	}

	delete this;
}

CMovementAnimation::CMovementAnimation(CBattleInterface *_owner, const CStack *_stack, std::vector<SBattleHex> _destTiles, int _distance)
: CBattleStackAnimation(_owner, _stack), destTiles(_destTiles), nextPos(0), distance(_distance), stepX(0.0), stepY(0.0)
{
	curStackPos = stack->position;
	nextHex = destTiles.front();
}