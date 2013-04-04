#include "StdInc.h"
#include "CBattleAnimations.h"

#include <boost/math/constants/constants.hpp>
#include "../CMusicHandler.h"
#include "../CGameInfo.h"
#include "CBattleInterface.h"
#include "CBattleInterfaceClasses.h"
#include "CCreatureAnimation.h"
#include "../../lib/BattleState.h"
#include "../CPlayerInterface.h"
#include "../../CCallback.h"
#include "../UIFramework/SDL_Extensions.h"
#include "../Graphics.h"
#include "../UIFramework/CCursorHandler.h"
#include "../../lib/CTownHandler.h"


CBattleAnimation::CBattleAnimation(CBattleInterface * _owner)
: owner(_owner), ID(_owner->animIDhelper++) 
{}

void CBattleAnimation::endAnim()
{
	for(std::list<std::pair<CBattleAnimation *, bool> >::iterator it = owner->pendingAnims.begin(); it != owner->pendingAnims.end(); ++it)
	{
		if(it->first == this)
		{
			it->first = NULL;
		}
	}

}

bool CBattleAnimation::isEarliest(bool perStackConcurrency)
{
	int lowestMoveID = owner->animIDhelper + 5;
	CBattleStackAnimation * thAnim = dynamic_cast<CBattleStackAnimation *>(this);
	CSpellEffectAnimation * thSen = dynamic_cast<CSpellEffectAnimation *>(this);

	for(std::list<std::pair<CBattleAnimation *, bool> >::iterator it = owner->pendingAnims.begin(); it != owner->pendingAnims.end(); ++it)
	{
		CBattleStackAnimation * stAnim = dynamic_cast<CBattleStackAnimation *>(it->first);
		CSpellEffectAnimation * sen = dynamic_cast<CSpellEffectAnimation *>(it->first);
		if(perStackConcurrency && stAnim && thAnim && stAnim->stack->ID != thAnim->stack->ID)
			continue;

		if(sen && thSen && sen != thSen && perStackConcurrency)
			continue;

		CReverseAnimation * revAnim = dynamic_cast<CReverseAnimation *>(stAnim);

		if(revAnim && thAnim && stAnim && stAnim->stack->ID == thAnim->stack->ID && revAnim->priority)
			return false;

		if(it->first)
			vstd::amin(lowestMoveID, it->first->ID);
	}
	return (ID == lowestMoveID) || (lowestMoveID == (owner->animIDhelper + 5));
}

CBattleStackAnimation::CBattleStackAnimation(CBattleInterface * _owner, const CStack * _stack)
: CBattleAnimation(_owner), stack(_stack) 
{}

CCreatureAnimation* CBattleStackAnimation::myAnim()
{
	return owner->creAnims[stack->ID];
}

void CAttackAnimation::nextFrame()
{
	if(myAnim()->getType() != group)
		myAnim()->setType(group);

	if(myAnim()->onFirstFrameInGroup())
	{
		if(shooting)
			CCS->soundh->playSound(battle_sound(attackingStack->getCreature(), shoot));
		else
			CCS->soundh->playSound(battle_sound(attackingStack->getCreature(), attack));
	}
	else if(myAnim()->onLastFrameInGroup())
	{
		myAnim()->setType(CCreatureAnim::HOLDING);
		endAnim();
		return; //execution of endAnim deletes this !!!
	}
}

bool CAttackAnimation::checkInitialConditions()
{
	return isEarliest(false);
}

CAttackAnimation::CAttackAnimation(CBattleInterface *_owner, const CStack *attacker, BattleHex _dest, const CStack *defender)
: CBattleStackAnimation(_owner, attacker), dest(_dest), attackedStack(defender), attackingStack(attacker)
{

	assert(attackingStack && "attackingStack is NULL in CBattleAttack::CBattleAttack !\n");
	bool isCatapultAttack = attackingStack->hasBonusOfType(Bonus::CATAPULT) 
							&& owner->curInt->cb->battleHexToWallPart(_dest) >= 0;

	assert(attackedStack || isCatapultAttack);
	attackingStackPosBeforeReturn = attackingStack->position;
}

CDefenceAnimation::CDefenceAnimation(StackAttackedInfo _attackedInfo, CBattleInterface * _owner)
: CBattleStackAnimation(_owner, _attackedInfo.defender),
attacker(_attackedInfo.attacker), byShooting(_attackedInfo.byShooting),
killed(_attackedInfo.killed) 
{}

bool CDefenceAnimation::init()
{
	//checking initial conditions

	//if(owner->creAnims[stackID]->getType() != 2)
	//{
	//	return false;
	//}

	if(attacker == NULL && owner->battleEffects.size() > 0)
		return false;

	ui32 lowestMoveID = owner->animIDhelper + 5;
	for(std::list<std::pair<CBattleAnimation *, bool> >::iterator it = owner->pendingAnims.begin(); it != owner->pendingAnims.end(); ++it)
	{
		CDefenceAnimation * defAnim = dynamic_cast<CDefenceAnimation *>(it->first);
		if(defAnim && defAnim->stack->ID != stack->ID)
			continue;

		CAttackAnimation * attAnim = dynamic_cast<CAttackAnimation *>(it->first);
		if(attAnim && attAnim->stack->ID != stack->ID)
			continue;

		if(attacker != NULL)
		{
			int attackerAnimType = owner->creAnims[attacker->ID]->getType();
			if( ( attackerAnimType == CCreatureAnim::ATTACK_UP ||
			    attackerAnimType == CCreatureAnim::ATTACK_FRONT ||
			    attackerAnimType == CCreatureAnim::ATTACK_DOWN ) &&
			    owner->creAnims[attacker->ID]->getFrame() < attacker->getCreature()->animation.attackClimaxFrame )
				return false;
		}

		CReverseAnimation * animAsRev = dynamic_cast<CReverseAnimation *>(it->first);

		if(animAsRev && animAsRev->priority)
			return false;

		if(it->first)
			vstd::amin(lowestMoveID, it->first->ID);
	}
	if(ID > lowestMoveID)
		return false;



	//reverse unit if necessary
	if (attacker && owner->curInt->cb->isToReverse(stack->position, attacker->position, owner->creDir[stack->ID], attacker->doubleWide(), owner->creDir[attacker->ID]))
	{
		owner->addNewAnim(new CReverseAnimation(owner, stack, stack->position, true));
		return false;
	}
	//unit reversed

	if(byShooting) //delay hit animation
	{		
		for(std::list<ProjectileInfo>::const_iterator it = owner->projectiles.begin(); it != owner->projectiles.end(); ++it)
		{
			if(it->creID == attacker->getCreature()->idNumber)
			{
				return false;
			}
		}
	}

	//initializing
	if(killed)
	{
		CCS->soundh->playSound(battle_sound(stack->getCreature(), killed));
		myAnim()->setType(CCreatureAnim::DEATH); //death
	}
	else
	{
		// TODO: this block doesn't seems correct if the unit is defending.
		CCS->soundh->playSound(battle_sound(stack->getCreature(), wince));
		myAnim()->setType(CCreatureAnim::HITTED); //getting hit
	}

	return true; //initialized successfuly
}

void CDefenceAnimation::nextFrame()
{
	if(!killed && myAnim()->getType() != CCreatureAnim::HITTED)
	{
		myAnim()->setType(CCreatureAnim::HITTED);
	}

	if(!myAnim()->onLastFrameInGroup())
	{
		if( myAnim()->getType() == CCreatureAnim::DEATH && (owner->animCount+1)%(4/owner->getAnimSpeed())==0
			&& !myAnim()->onLastFrameInGroup() )
		{
			myAnim()->incrementFrame();
		}
	}
	else
	{
		endAnim();
	}

}

void CDefenceAnimation::endAnim()
{
	//restoring animType

	if(myAnim()->getType() == CCreatureAnim::HITTED)
		myAnim()->setType(CCreatureAnim::HOLDING);

	//printing info to console

	//if(attacker!=NULL)
	//	owner->printConsoleAttacked(stack, dmg, amountKilled, attacker);

	//const CStack * attacker = owner->curInt->cb->battleGetStackByID(IDby, false);
	//const CStack * attacked = owner->curInt->cb->battleGetStackByID(stackID, false);

	CBattleAnimation::endAnim();

	delete this;
}

CDummyAnimation::CDummyAnimation(CBattleInterface * _owner, int howManyFrames) 
: CBattleAnimation(_owner), counter(0), howMany(howManyFrames) 
{}

bool CDummyAnimation::init()
{
	return true;
}

void CDummyAnimation::nextFrame()
{
	counter++;
	if(counter > howMany)
		endAnim();
}

void CDummyAnimation::endAnim()
{
	CBattleAnimation::endAnim();

	delete this;
}

bool CMeleeAttackAnimation::init()
{
	if( !CAttackAnimation::checkInitialConditions() )
		return false;

	//if(owner->creAnims[stackID]->getType()!=2)
	//{
	//	return false;
	//}

	if(!attackingStack || myAnim()->getType() == 5)
	{
		endAnim();
		
		return false;
	}

	bool toReverse = owner->curInt->cb->isToReverse(attackingStackPosBeforeReturn, dest, owner->creDir[stack->ID], attackedStack->doubleWide(), owner->creDir[attackedStack->ID]);

	if (toReverse)
	{

		owner->addNewAnim(new CReverseAnimation(owner, stack, attackingStackPosBeforeReturn, true));
		return false;
	}
	//reversed

	shooting = false;

	static const CCreatureAnim::EAnimType mutPosToGroup[] = {CCreatureAnim::ATTACK_UP, CCreatureAnim::ATTACK_UP,
		CCreatureAnim::ATTACK_FRONT, CCreatureAnim::ATTACK_DOWN, CCreatureAnim::ATTACK_DOWN, CCreatureAnim::ATTACK_FRONT};

	int revShiftattacker = (attackingStack->attackerOwned ? -1 : 1);

	int mutPos = BattleHex::mutualPosition(attackingStackPosBeforeReturn, dest);
	if(mutPos == -1 && attackingStack->doubleWide())
	{
		mutPos = BattleHex::mutualPosition(attackingStackPosBeforeReturn + revShiftattacker, attackedStack->position);
	}
	if (mutPos == -1 && attackedStack->doubleWide())
	{
		mutPos = BattleHex::mutualPosition(attackingStackPosBeforeReturn, attackedStack->occupiedHex());
	}
	if (mutPos == -1 && attackedStack->doubleWide() && attackingStack->doubleWide())
	{
		mutPos = BattleHex::mutualPosition(attackingStackPosBeforeReturn + revShiftattacker, attackedStack->occupiedHex());
	}


	switch(mutPos) //attack direction
	{
	case 0: case 1: case 2: case 3: case 4: case 5:
		group = mutPosToGroup[mutPos];
		break;
	default:
		tlog1<<"Critical Error! Wrong dest in stackAttacking! dest: "<<dest<<" attacking stack pos: "<<attackingStackPosBeforeReturn<<" mutual pos: "<<mutPos<<std::endl;
		group = CCreatureAnim::ATTACK_FRONT;
		break;
	}

	return true;
}

CMeleeAttackAnimation::CMeleeAttackAnimation(CBattleInterface * _owner, const CStack * attacker, BattleHex _dest, const CStack * _attacked)
: CAttackAnimation(_owner, attacker, _dest, _attacked) 
{}

void CMeleeAttackAnimation::nextFrame()
{
	/*for(std::list<std::pair<CBattleAnimation *, bool> >::const_iterator it = owner->pendingAnims.begin(); it != owner->pendingAnims.end(); ++it)
	{
		CBattleMoveStart * anim = dynamic_cast<CBattleMoveStart *>(it->first);
		CReverseAnim * anim2 = dynamic_cast<CReverseAnim *>(it->first);
		if( (anim && anim->stackID == stackID) || (anim2 && anim2->stackID == stackID ) )
			return;
	}*/

	CAttackAnimation::nextFrame();
}

void CMeleeAttackAnimation::endAnim()
{
	CBattleAnimation::endAnim();

	delete this;
}

bool CMovementAnimation::init()
{
if( !isEarliest(false) )
		return false;

	//a few useful variables
	steps = static_cast<int>(myAnim()->framesInGroup(CCreatureAnim::MOVING) * owner->getAnimSpeedMultiplier() - 1);

	const CStack * movedStack = stack;
	if(!movedStack || myAnim()->getType() == 5)
	{
		endAnim();
		return false;
	}

	Point begPosition = CClickableHex::getXYUnitAnim(curStackPos, movedStack->attackerOwned, movedStack, owner);
	Point endPosition = CClickableHex::getXYUnitAnim(nextHex, movedStack->attackerOwned, movedStack, owner);

	if(steps < 0 || stack->hasBonus(Selector::typeSubtype(Bonus::FLYING, 1))) //no movement or teleport
	{
		//this creature seems to have no move animation so we can end it immediately
		endAnim();
		return false;
	}
	whichStep = 0;
	int hexWbase = 44, hexHbase = 42;

	//bool twoTiles = movedStack->doubleWide();

	int mutPos = BattleHex::mutualPosition(curStackPos, nextHex);

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

	if (owner->moveSh == -1)
	{
		owner->moveSh = CCS->soundh->playSound(battle_sound(movedStack->getCreature(), move), -1);
	}

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
		Point coords = CClickableHex::getXYUnitAnim(nextHex, owner->creDir[stack->ID], stack, owner);
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

	myAnim()->pos = CClickableHex::getXYUnitAnim(nextHex, movedStack->attackerOwned, movedStack, owner);
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

CMovementAnimation::CMovementAnimation(CBattleInterface *_owner, const CStack *_stack, std::vector<BattleHex> _destTiles, int _distance)
: CBattleStackAnimation(_owner, _stack), destTiles(_destTiles), nextPos(0), distance(_distance), stepX(0.0), stepY(0.0)
{
	curStackPos = stack->position;
	nextHex = destTiles.front();
}

CMovementEndAnimation::CMovementEndAnimation(CBattleInterface * _owner, const CStack * _stack, BattleHex destTile)
: CBattleStackAnimation(_owner, _stack), destinationTile(destTile) 
{}

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

CMovementStartAnimation::CMovementStartAnimation(CBattleInterface * _owner, const CStack * _stack)
: CBattleStackAnimation(_owner, _stack) 
{}

bool CMovementStartAnimation::init()
{
	if( !isEarliest(false) )
		return false;


	if(!stack || myAnim()->getType() == CCreatureAnim::DEATH)
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
		if((owner->animCount+1)%(4/owner->getAnimSpeed())==0)
			myAnim()->incrementFrame();
	}
}

void CMovementStartAnimation::endAnim()
{
	CBattleAnimation::endAnim();

	delete this;
}

CReverseAnimation::CReverseAnimation(CBattleInterface * _owner, const CStack * stack, BattleHex dest, bool _priority)
: CBattleStackAnimation(_owner, stack), partOfAnim(1), secondPartSetup(false), hex(dest), priority(_priority) 
{}

bool CReverseAnimation::init()
{
	if(myAnim() == NULL || myAnim()->getType() == CCreatureAnim::DEATH)
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

void CReverseAnimation::setupSecondPart()
{
	if(!stack)
	{
		endAnim();
		return;
	}

	owner->creDir[stack->ID] = !owner->creDir[stack->ID];

	Point coords = CClickableHex::getXYUnitAnim(hex, owner->creDir[stack->ID], stack, owner);
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

CShootingAnimation::CShootingAnimation(CBattleInterface * _owner, const CStack * attacker, BattleHex _dest, const CStack * _attacked, bool _catapult, int _catapultDmg)
: CAttackAnimation(_owner, attacker, _dest, _attacked), catapultDamage(_catapultDmg)
{}

bool CShootingAnimation::init()
{
	if( !CAttackAnimation::checkInitialConditions() )
		return false;

	const CStack * shooter = attackingStack;

	if(!shooter || myAnim()->getType() == CCreatureAnim::DEATH)
	{
		endAnim();
		return false;
	}

	//reverse unit if necessary
	if (attackingStack && attackedStack && owner->curInt->cb->isToReverse(attackingStack->position, attackedStack->position, owner->creDir[attackingStack->ID], attackingStack->doubleWide(), owner->creDir[attackedStack->ID]))
	{
		owner->addNewAnim(new CReverseAnimation(owner, attackingStack, attackingStack->position, true));
		return false;
	}

	// Create the projectile animation

	//maximal angle in radians between straight horizontal line and shooting line for which shot is considered to be straight (absoulte value)
	static const double straightAngle = 0.2;

	// Get further info about the shooter e.g. relative pos of projectile to unit.
	// If the creature id is 149 then it's a arrow tower which has no additional info so get the
	// actual arrow tower shooter instead.
	const CCreature *shooterInfo = shooter->getCreature();
	if (shooterInfo->idNumber == CreatureID::ARROW_TOWERS)
	{
		int creID = owner->siegeH->town->town->clientInfo.siegeShooter;
		shooterInfo = CGI->creh->creatures[creID];
	}

	ProjectileInfo spi;
	spi.creID = shooter->getCreature()->idNumber;
	spi.stackID = shooter->ID;
	// reverse if creature is facing right OR this is non-existing stack that is not tower (war machines)
	spi.reverse = attackingStack ? !owner->creDir[attackingStack->ID] : shooter->getCreature()->idNumber != CreatureID::ARROW_TOWERS ;

	spi.step = 0;
	spi.frameNum = 0;

	Point fromPos;
	Point destPos;

	// NOTE: two lines below return different positions (very notable with 2-hex creatures). Obtaining via creanims seems to be more precise
	fromPos = owner->creAnims[spi.stackID]->pos.topLeft();
	//xycoord = CClickableHex::getXYUnitAnim(shooter->position, true, shooter, owner);

	destPos = CClickableHex::getXYUnitAnim(dest, false, attackedStack, owner);

	// to properly translate coordinates when shooter is rotated
	int multiplier = spi.reverse ? -1 : 1;

	double projectileAngle = atan2(fabs(destPos.y - fromPos.y), fabs(destPos.x - fromPos.x));
	if(shooter->position < dest)
		projectileAngle = -projectileAngle;

	// Calculate projectile start position. Offsets are read out of the CRANIM.TXT.
	if (projectileAngle > straightAngle)
	{
		//upper shot
		spi.x = fromPos.x + 222 + ( -25 + shooterInfo->animation.upperRightMissleOffsetX ) * multiplier;
		spi.y = fromPos.y + 265 + shooterInfo->animation.upperRightMissleOffsetY;
	}
	else if (projectileAngle < -straightAngle)
	{
		//lower shot
		spi.x = fromPos.x + 222 + ( -25 + shooterInfo->animation.lowerRightMissleOffsetX ) * multiplier;
		spi.y = fromPos.y + 265 + shooterInfo->animation.lowerRightMissleOffsetY;
	}
	else
	{
		//straight shot
		spi.x = fromPos.x + 222 + ( -25 + shooterInfo->animation.rightMissleOffsetX ) * multiplier;
		spi.y = fromPos.y + 265 + shooterInfo->animation.rightMissleOffsetY;
	}

	destPos += Point(225, 225);

	// recalculate angle taking in account offsets
	//projectileAngle = atan2(fabs(destPos.y - spi.y), fabs(destPos.x - spi.x));
	//if(shooter->position < dest)
	//	projectileAngle = -projectileAngle;

	if (attackedStack)
	{
		double animSpeed = 23.0 * owner->getAnimSpeed(); // flight speed of projectile
		spi.lastStep = static_cast<int>(sqrt(static_cast<double>((destPos.x - spi.x) * (destPos.x - spi.x) + (destPos.y - spi.y) * (destPos.y - spi.y))) / animSpeed);
		if(spi.lastStep == 0)
			spi.lastStep = 1;
		spi.dx = (destPos.x - spi.x) / spi.lastStep;
		spi.dy = (destPos.y - spi.y) / spi.lastStep;
	}
	else 
	{
		// Catapult attack
		spi.catapultInfo.reset(new CatapultProjectileInfo(Point(spi.x, spi.y), destPos));

		double animSpeed = 3.318 * owner->getAnimSpeed();
		spi.lastStep = abs((destPos.x - spi.x) / animSpeed);
		spi.dx = animSpeed;
		spi.dy = 0;

		SDL_Surface * img = owner->idToProjectile[spi.creID]->ourImages[0].bitmap;

		// Add explosion anim
		Point animPos(destPos.x - 126 + img->w / 2,
		              destPos.y - 105 + img->h / 2);

		owner->addNewAnim( new CSpellEffectAnimation(owner, catapultDamage ? "SGEXPL.DEF" : "CSGRCK.DEF", animPos.x, animPos.y));
	}

	auto & angles = shooterInfo->animation.missleFrameAngles;
	double pi = boost::math::constants::pi<double>();

	// only frames below maxFrame are usable: anything  higher is either no present or we don't know when it should be used
	size_t maxFrame = std::min<size_t>(angles.size(), owner->idToProjectile[spi.creID]->ourImages.size());

	assert(maxFrame > 0);

	// values in angles array indicate position from which this frame was rendered, in degrees.
	// find frame that has closest angle to one that we need for this shot
	size_t bestID = 0;
	double bestDiff = fabs( angles[0] / 180 * pi - projectileAngle );

	for (size_t i=1; i<maxFrame; i++)
	{
		double currentDiff = fabs( angles[i] / 180 * pi - projectileAngle );
		if (currentDiff < bestDiff)
		{
			bestID = i;
			bestDiff = currentDiff;
		}
	}

	spi.frameNum = bestID;

	// Set projectile animation start delay which is specified in frames
	spi.animStartDelay = shooterInfo->animation.attackClimaxFrame;
	owner->projectiles.push_back(spi);

	//attack animation

	shooting = true;

	if(projectileAngle > straightAngle) //upper shot
		group = CCreatureAnim::SHOOT_UP;
	else if(projectileAngle < -straightAngle) //lower shot
		group = CCreatureAnim::SHOOT_DOWN;
	else //straight shot
		group = CCreatureAnim::SHOOT_FRONT;

	return true;
}

void CShootingAnimation::nextFrame()
{
	for(std::list<std::pair<CBattleAnimation *, bool> >::const_iterator it = owner->pendingAnims.begin(); it != owner->pendingAnims.end(); ++it)
	{
		CMovementStartAnimation * anim = dynamic_cast<CMovementStartAnimation *>(it->first);
		CReverseAnimation * anim2 = dynamic_cast<CReverseAnimation *>(it->first);
		if( (anim && anim->stack->ID == stack->ID) || (anim2 && anim2->stack->ID == stack->ID && anim2->priority ) )
			return;
	}

	CAttackAnimation::nextFrame();
}

void CShootingAnimation::endAnim()
{
	CBattleAnimation::endAnim();
	delete this;
}

CSpellEffectAnimation::CSpellEffectAnimation(CBattleInterface * _owner, ui32 _effect, BattleHex _destTile, int _dx, int _dy, bool _Vflip)
:CBattleAnimation(_owner), effect(_effect), destTile(_destTile), customAnim(""), x(0), y(0), dx(_dx), dy(_dy), Vflip(_Vflip) 
{}

CSpellEffectAnimation::CSpellEffectAnimation(CBattleInterface * _owner, std::string _customAnim, int _x, int _y, int _dx, int _dy, bool _Vflip)
:CBattleAnimation(_owner), effect(-1), destTile(0), customAnim(_customAnim), x(_x), y(_y), dx(_dx), dy(_dy), Vflip(_Vflip) 
{}

bool CSpellEffectAnimation::init()
{
	if(!isEarliest(true))
		return false;

	if(effect == 12) //armageddon
	{
		if(effect == -1 || graphics->battleACToDef[effect].size() != 0)
		{
			CDefHandler * anim;
			if(customAnim.size())
				anim = CDefHandler::giveDef(customAnim);
			else
				anim = CDefHandler::giveDef(graphics->battleACToDef[effect][0]);

			if (Vflip)
			{
				for (size_t v = 0; v < anim->ourImages.size(); ++v)
				{
					CSDL_Ext::VflipSurf(anim->ourImages[v].bitmap);
				}
			}

			for(int i=0; i * anim->width < owner->pos.w ; ++i)
			{
				for(int j=0; j * anim->height < owner->pos.h ; ++j)
				{
					BattleEffect be;
					be.effectID = ID;
					be.anim = CDefHandler::giveDef(graphics->battleACToDef[effect][0]);
					if (Vflip)
					{
						for (size_t v = 0; v < be.anim->ourImages.size(); ++v)
						{
							CSDL_Ext::VflipSurf(be.anim->ourImages[v].bitmap);
						}
					}
					be.frame = 0;
					be.maxFrame = be.anim->ourImages.size();
					be.x = i * anim->width + owner->pos.x;
					be.y = j * anim->height + owner->pos.y;

					owner->battleEffects.push_back(be);
				}
			}
		}
		else //there is nothing to play
		{
			endAnim();
			return false;
		}
	}
	else // Effects targeted at a specific creature/hex.
	{
		if(effect == -1 || graphics->battleACToDef[effect].size() != 0)
		{
			const CStack* destStack = owner->curInt->cb->battleGetStackByPos(destTile, false);
			Rect &tilePos = owner->bfield[destTile]->pos;
			BattleEffect be;
			be.effectID = ID;
			if(customAnim.size())
				be.anim = CDefHandler::giveDef(customAnim);
			else
				be.anim = CDefHandler::giveDef(graphics->battleACToDef[effect][0]);

			if (Vflip)
			{
				for (size_t v = 0; v < be.anim->ourImages.size(); ++v)
				{
					CSDL_Ext::VflipSurf(be.anim->ourImages[v].bitmap);
				}
			}

			be.frame = 0;
			be.maxFrame = be.anim->ourImages.size();
			if(effect == 1)
				be.maxFrame = 3;

			switch (effect)
			{
			case ui32(-1):
				be.x = x;
				be.y = y;
				break;
			case 0: // Prayer and Lightning Bolt.
			case 1:
				// Position effect with it's bottom center touching the bottom center of affected tile(s).
				be.x = tilePos.x + tilePos.w/2 - be.anim->width/2;
				be.y = tilePos.y + tilePos.h - be.anim->height;
				break;

			default:
				// Position effect with it's center touching the top center of affected tile(s).
				be.x = tilePos.x + tilePos.w/2 - be.anim->width/2;
				be.y = tilePos.y - be.anim->height/2;
				break;
			}

			// Correction for 2-hex creatures.
			if (destStack != NULL && destStack->doubleWide())
				be.x += (destStack->attackerOwned ? -1 : 1)*tilePos.w/2;

			owner->battleEffects.push_back(be);
		}
		else //there is nothing to play
		{
			endAnim();
			return false;
		}
	}
	//battleEffects 
	return true;
}

void CSpellEffectAnimation::nextFrame()
{
	//notice: there may be more than one effect in owner->battleEffects correcponding to this animation (ie. armageddon)
	for(std::list<BattleEffect>::iterator it = owner->battleEffects.begin(); it != owner->battleEffects.end(); ++it)
	{
		if(it->effectID == ID)
		{
			++(it->frame);

			if(it->frame == it->maxFrame)
			{
				endAnim();
				break;
			}
			else
			{
				it->x += dx;
				it->y += dy;
			}
		}
	}
}

void CSpellEffectAnimation::endAnim()
{
	CBattleAnimation::endAnim();

	std::vector<std::list<BattleEffect>::iterator> toDel;

	for(std::list<BattleEffect>::iterator it = owner->battleEffects.begin(); it != owner->battleEffects.end(); ++it)
	{
		if(it->effectID == ID)
		{
			toDel.push_back(it);
		}
	}

	for(size_t b = 0; b < toDel.size(); ++b)
	{
		delete toDel[b]->anim;
		owner->battleEffects.erase(toDel[b]);
	}

	delete this;
}