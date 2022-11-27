/*
 * CBattleAnimations.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CBattleAnimations.h"

#include <boost/math/constants/constants.hpp>

#include "CBattleInterfaceClasses.h"
#include "CBattleInterface.h"
#include "CBattleProjectileController.h"
#include "CBattleSiegeController.h"
#include "CBattleFieldController.h"
#include "CBattleEffectsController.h"
#include "CBattleStacksController.h"
#include "CCreatureAnimation.h"

#include "../CGameInfo.h"
#include "../CMusicHandler.h"
#include "../CPlayerInterface.h"
#include "../Graphics.h"
#include "../gui/CAnimation.h"
#include "../gui/CCursorHandler.h"
#include "../gui/CGuiHandler.h"

#include "../../CCallback.h"
#include "../../lib/CStack.h"
#include "../../lib/CTownHandler.h"
#include "../../lib/mapObjects/CGTownInstance.h"

CBattleAnimation::CBattleAnimation(CBattleInterface * _owner)
	: owner(_owner), ID(_owner->stacksController->animIDhelper++)
{
	logAnim->trace("Animation #%d created", ID);
}

CBattleAnimation::~CBattleAnimation()
{
	logAnim->trace("Animation #%d deleted", ID);
}

std::list<std::pair<CBattleAnimation *, bool>> & CBattleAnimation::pendingAnimations()
{
	return owner->stacksController->pendingAnims;
}

std::shared_ptr<CCreatureAnimation> CBattleAnimation::stackAnimation(const CStack * stack)
{
	return owner->stacksController->creAnims[stack->ID];
}

bool CBattleAnimation::stackFacingRight(const CStack * stack)
{
	return owner->stacksController->creDir[stack->ID];
}

ui32 CBattleAnimation::maxAnimationID()
{
	return owner->stacksController->animIDhelper;
}

void CBattleAnimation::setStackFacingRight(const CStack * stack, bool facingRight)
{
	owner->stacksController->creDir[stack->ID] = facingRight;
}

void CBattleAnimation::endAnim()
{
	logAnim->trace("Animation #%d ended, type is %s", ID, typeid(this).name());
	for(auto & elem : pendingAnimations())
	{
		if(elem.first == this)
		{
			elem.first = nullptr;
		}
	}
}

bool CBattleAnimation::isEarliest(bool perStackConcurrency)
{
	int lowestMoveID = maxAnimationID() + 5;//FIXME: why 5?
	CBattleStackAnimation * thAnim = dynamic_cast<CBattleStackAnimation *>(this);
	CEffectAnimation * thSen = dynamic_cast<CEffectAnimation *>(this);

	for(auto & elem : pendingAnimations())
	{
		CBattleStackAnimation * stAnim = dynamic_cast<CBattleStackAnimation *>(elem.first);
		CEffectAnimation * sen = dynamic_cast<CEffectAnimation *>(elem.first);
		if(perStackConcurrency && stAnim && thAnim && stAnim->stack->ID != thAnim->stack->ID)
			continue;

		if(perStackConcurrency && sen && thSen && sen != thSen)
			continue;

		CReverseAnimation * revAnim = dynamic_cast<CReverseAnimation *>(stAnim);

		if(revAnim && thAnim && stAnim && stAnim->stack->ID == thAnim->stack->ID && revAnim->priority)
			return false;

		if(elem.first)
			vstd::amin(lowestMoveID, elem.first->ID);
	}
	return (ID == lowestMoveID) || (lowestMoveID == (maxAnimationID() + 5));
}

CBattleStackAnimation::CBattleStackAnimation(CBattleInterface * owner, const CStack * stack)
	: CBattleAnimation(owner),
	  myAnim(stackAnimation(stack)),
	  stack(stack)
{
	assert(myAnim);
}

void CBattleStackAnimation::shiftColor(const ColorShifter * shifter)
{
	assert(myAnim);
	myAnim->shiftColor(shifter);
}

void CAttackAnimation::nextFrame()
{
	if(myAnim->getType() != group)
	{
		myAnim->setType(group);
		myAnim->onAnimationReset += std::bind(&CAttackAnimation::endAnim, this);
	}

	if(!soundPlayed)
	{
		if(shooting)
			CCS->soundh->playSound(battle_sound(attackingStack->getCreature(), shoot));
		else
			CCS->soundh->playSound(battle_sound(attackingStack->getCreature(), attack));
		soundPlayed = true;
	}
	CBattleAnimation::nextFrame();
}

void CAttackAnimation::endAnim()
{
	myAnim->setType(CCreatureAnim::HOLDING);
	CBattleStackAnimation::endAnim();
}

bool CAttackAnimation::checkInitialConditions()
{
	for(auto & elem : pendingAnimations())
	{
		CBattleStackAnimation * stAnim = dynamic_cast<CBattleStackAnimation *>(elem.first);
		CReverseAnimation * revAnim = dynamic_cast<CReverseAnimation *>(stAnim);

		if(revAnim && attackedStack) // enemy must be fully reversed
		{
			if (revAnim->stack->ID == attackedStack->ID)
				return false;
		}
	}
	return isEarliest(false);
}

CAttackAnimation::CAttackAnimation(CBattleInterface *_owner, const CStack *attacker, BattleHex _dest, const CStack *defender)
	: CBattleStackAnimation(_owner, attacker),
		shooting(false), group(CCreatureAnim::SHOOT_FRONT),
		soundPlayed(false),
		dest(_dest), attackedStack(defender), attackingStack(attacker)
{
	assert(attackingStack && "attackingStack is nullptr in CBattleAttack::CBattleAttack !\n");
	attackingStackPosBeforeReturn = attackingStack->getPosition();
}

CDefenceAnimation::CDefenceAnimation(StackAttackedInfo _attackedInfo, CBattleInterface * _owner)
: CBattleStackAnimation(_owner, _attackedInfo.defender),
attacker(_attackedInfo.attacker), rangedAttack(_attackedInfo.indirectAttack),
killed(_attackedInfo.killed), timeToWait(0)
{
	logAnim->debug("Created defence anim for %s", _attackedInfo.defender->getName());
}

bool CDefenceAnimation::init()
{
	ui32 lowestMoveID = maxAnimationID() + 5;
	for(auto & elem : pendingAnimations())
	{

		CDefenceAnimation * defAnim = dynamic_cast<CDefenceAnimation *>(elem.first);
		if(defAnim && defAnim->stack->ID != stack->ID)
			continue;

		CAttackAnimation * attAnim = dynamic_cast<CAttackAnimation *>(elem.first);
		if(attAnim && attAnim->stack->ID != stack->ID)
			continue;

		CEffectAnimation * sen = dynamic_cast<CEffectAnimation *>(elem.first);
		if (sen && attacker == nullptr)
			return false;

		if (sen)
			continue;

		CReverseAnimation * animAsRev = dynamic_cast<CReverseAnimation *>(elem.first);

		if(animAsRev)
			return false;

		if(elem.first)
			vstd::amin(lowestMoveID, elem.first->ID);
	}

	if(ID > lowestMoveID)
		return false;


	//reverse unit if necessary
	if(attacker && owner->getCurrentPlayerInterface()->cb->isToReverse(stack->getPosition(), attacker->getPosition(), stackFacingRight(stack), attacker->doubleWide(), stackFacingRight(attacker)))
	{
		owner->stacksController->addNewAnim(new CReverseAnimation(owner, stack, stack->getPosition(), true));
		return false;
	}
	//unit reversed

	if(rangedAttack && attacker != nullptr && owner->projectilesController->hasActiveProjectile(attacker)) //delay hit animation
	{
		return false;
	}

	// synchronize animation with attacker, unless defending or attacked by shooter:
	// wait for 1/2 of attack animation
	if (!rangedAttack && getMyAnimType() != CCreatureAnim::DEFENCE)
	{
		float frameLength = AnimationControls::getCreatureAnimationSpeed(
								  stack->getCreature(), stackAnimation(stack).get(), getMyAnimType());

		timeToWait = myAnim->framesInGroup(getMyAnimType()) * frameLength / 2;

		myAnim->setType(CCreatureAnim::HOLDING);
	}
	else
	{
		timeToWait = 0;
		startAnimation();
	}

	return true; //initialized successfuly
}

std::string CDefenceAnimation::getMySound()
{
	if(killed)
		return battle_sound(stack->getCreature(), killed);
	else if(stack->defendingAnim)
		return battle_sound(stack->getCreature(), defend);
	else
		return battle_sound(stack->getCreature(), wince);
}

CCreatureAnim::EAnimType CDefenceAnimation::getMyAnimType()
{
	if(killed)
	{
		if(rangedAttack && myAnim->framesInGroup(CCreatureAnim::DEATH_RANGED) > 0)
			return CCreatureAnim::DEATH_RANGED;
		else
			return CCreatureAnim::DEATH;
	}

	if(stack->defendingAnim)
		return CCreatureAnim::DEFENCE;
	else
		return CCreatureAnim::HITTED;
}

void CDefenceAnimation::startAnimation()
{
	CCS->soundh->playSound(getMySound());
	myAnim->setType(getMyAnimType());
	myAnim->onAnimationReset += std::bind(&CDefenceAnimation::endAnim, this);
}

void CDefenceAnimation::nextFrame()
{
	if (timeToWait > 0)
	{
		timeToWait -= float(GH.mainFPSmng->getElapsedMilliseconds()) / 1000;
		if (timeToWait <= 0)
			startAnimation();
	}

	CBattleAnimation::nextFrame();
}

void CDefenceAnimation::endAnim()
{
	if(killed)
	{
		if(rangedAttack && myAnim->framesInGroup(CCreatureAnim::DEAD_RANGED) > 0)
			myAnim->setType(CCreatureAnim::DEAD_RANGED);
		else
			myAnim->setType(CCreatureAnim::DEAD);
	}
	else
	{
		myAnim->setType(CCreatureAnim::HOLDING);
	}


	CBattleAnimation::endAnim();

	delete this;
}

CDummyAnimation::CDummyAnimation(CBattleInterface * _owner, int howManyFrames)
: CBattleAnimation(_owner), counter(0), howMany(howManyFrames)
{
	logAnim->debug("Created dummy animation for %d frames", howManyFrames);
}

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
	if(!CAttackAnimation::checkInitialConditions())
		return false;

	if(!attackingStack || myAnim->isDead())
	{
		endAnim();
		return false;
	}

	bool toReverse = owner->getCurrentPlayerInterface()->cb->isToReverse(attackingStackPosBeforeReturn, attackedStack->getPosition(), stackFacingRight(stack), attackedStack->doubleWide(), stackFacingRight(attackedStack));

	if(toReverse)
	{
		owner->stacksController->addNewAnim(new CReverseAnimation(owner, stack, attackingStackPosBeforeReturn, true));
		return false;
	}

	// opponent must face attacker ( = different directions) before he can be attacked
	if(attackingStack && attackedStack &&
		stackFacingRight(attackingStack) == stackFacingRight(attackedStack))
		return false;

	//reversed

	shooting = false;

	static const CCreatureAnim::EAnimType mutPosToGroup[] =
	{
		CCreatureAnim::ATTACK_UP,
		CCreatureAnim::ATTACK_UP,
		CCreatureAnim::ATTACK_FRONT,
		CCreatureAnim::ATTACK_DOWN,
		CCreatureAnim::ATTACK_DOWN,
		CCreatureAnim::ATTACK_FRONT
	};

	static const CCreatureAnim::EAnimType mutPosToGroup2H[] =
	{
		CCreatureAnim::VCMI_2HEX_UP,
		CCreatureAnim::VCMI_2HEX_UP,
		CCreatureAnim::VCMI_2HEX_FRONT,
		CCreatureAnim::VCMI_2HEX_DOWN,
		CCreatureAnim::VCMI_2HEX_DOWN,
		CCreatureAnim::VCMI_2HEX_FRONT
	};

	int revShiftattacker = (attackingStack->side == BattleSide::ATTACKER ? -1 : 1);

	int mutPos = BattleHex::mutualPosition(attackingStackPosBeforeReturn, dest);
	if(mutPos == -1 && attackingStack->doubleWide())
	{
		mutPos = BattleHex::mutualPosition(attackingStackPosBeforeReturn + revShiftattacker, attackedStack->getPosition());
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
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		group = mutPosToGroup[mutPos];
		if(attackingStack->hasBonusOfType(Bonus::TWO_HEX_ATTACK_BREATH))
		{
			CCreatureAnim::EAnimType group2H = mutPosToGroup2H[mutPos];
			if(myAnim->framesInGroup(group2H)>0)
				group = group2H;
		}

		break;
	default:
		logGlobal->error("Critical Error! Wrong dest in stackAttacking! dest: %d; attacking stack pos: %d; mutual pos: %d", dest.hex, attackingStackPosBeforeReturn, mutPos);
		group = CCreatureAnim::ATTACK_FRONT;
		break;
	}

	return true;
}

CMeleeAttackAnimation::CMeleeAttackAnimation(CBattleInterface * _owner, const CStack * attacker, BattleHex _dest, const CStack * _attacked)
: CAttackAnimation(_owner, attacker, _dest, _attacked)
{
	logAnim->debug("Created melee attack anim for %s", attacker->getName());
}

void CMeleeAttackAnimation::endAnim()
{
	CAttackAnimation::endAnim();
	delete this;
}

bool CMovementAnimation::init()
{
	if( !isEarliest(false) )
		return false;

	if(!stack || myAnim->isDead())
	{
		endAnim();
		return false;
	}

	if(stackAnimation(stack)->framesInGroup(CCreatureAnim::MOVING) == 0 ||
	   stack->hasBonus(Selector::typeSubtype(Bonus::FLYING, 1)))
	{
		//no movement or teleport, end immediately
		endAnim();
		return false;
	}

	//reverse unit if necessary
	if(owner->stacksController->shouldRotate(stack, oldPos, nextHex))
	{
		// it seems that H3 does NOT plays full rotation animation here in most situations
		// Logical since it takes quite a lot of time
		if (curentMoveIndex == 0) // full rotation only for moving towards first tile.
		{
			owner->stacksController->addNewAnim(new CReverseAnimation(owner, stack, oldPos, true));
			return false;
		}
		else
		{
			rotateStack(oldPos);
		}
	}

	if(myAnim->getType() != CCreatureAnim::MOVING)
	{
		myAnim->setType(CCreatureAnim::MOVING);
	}

	if (owner->moveSoundHander == -1)
	{
		owner->moveSoundHander = CCS->soundh->playSound(battle_sound(stack->getCreature(), move), -1);
	}

	Point begPosition = owner->stacksController->getStackPositionAtHex(oldPos, stack);
	Point endPosition = owner->stacksController->getStackPositionAtHex(nextHex, stack);

	timeToMove = AnimationControls::getMovementDuration(stack->getCreature());

	begX = begPosition.x;
	begY = begPosition.y;
	progress = 0;
	distanceX = endPosition.x - begPosition.x;
	distanceY = endPosition.y - begPosition.y;

	if (stack->hasBonus(Selector::type()(Bonus::FLYING)))
	{
		float distance = static_cast<float>(sqrt(distanceX * distanceX + distanceY * distanceY));

		timeToMove *= AnimationControls::getFlightDistance(stack->getCreature()) / distance;
	}

	return true;
}

void CMovementAnimation::nextFrame()
{
	progress += float(GH.mainFPSmng->getElapsedMilliseconds()) / 1000 * timeToMove;

	//moving instructions
	myAnim->pos.x = static_cast<Sint16>(begX + distanceX * progress );
	myAnim->pos.y = static_cast<Sint16>(begY + distanceY * progress );

	CBattleAnimation::nextFrame();

	if(progress >= 1.0)
	{
		// Sets the position of the creature animation sprites
		Point coords = owner->stacksController->getStackPositionAtHex(nextHex, stack);
		myAnim->pos = coords;

		// true if creature haven't reached the final destination hex
		if ((curentMoveIndex + 1) < destTiles.size())
		{
			// update the next hex field which has to be reached by the stack
			curentMoveIndex++;
			oldPos = nextHex;
			nextHex = destTiles[curentMoveIndex];

			// re-init animation
			for(auto & elem : pendingAnimations())
			{
				if (elem.first == this)
				{
					elem.second = false;
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
	assert(stack);

	myAnim->pos = owner->stacksController->getStackPositionAtHex(nextHex, stack);
	CBattleAnimation::endAnim();

	owner->stacksController->addNewAnim(new CMovementEndAnimation(owner, stack, nextHex));

	if(owner->moveSoundHander != -1)
	{
		CCS->soundh->stopSound(owner->moveSoundHander);
		owner->moveSoundHander = -1;
	}
	delete this;
}

CMovementAnimation::CMovementAnimation(CBattleInterface *_owner, const CStack *_stack, std::vector<BattleHex> _destTiles, int _distance)
	: CBattleStackAnimation(_owner, _stack),
	  destTiles(_destTiles),
	  curentMoveIndex(0),
	  oldPos(stack->getPosition()),
	  begX(0), begY(0),
	  distanceX(0), distanceY(0),
	  timeToMove(0.0),
	  progress(0.0),
	  nextHex(destTiles.front())
{
	logAnim->debug("Created movement anim for %s", stack->getName());
}

CMovementEndAnimation::CMovementEndAnimation(CBattleInterface * _owner, const CStack * _stack, BattleHex destTile)
: CBattleStackAnimation(_owner, _stack), destinationTile(destTile)
{
	logAnim->debug("Created movement end anim for %s", stack->getName());
}

bool CMovementEndAnimation::init()
{
	if( !isEarliest(true) )
		return false;

	if(!stack || myAnim->framesInGroup(CCreatureAnim::MOVE_END) == 0 ||
		myAnim->isDead())
	{
		endAnim();

		return false;
	}

	CCS->soundh->playSound(battle_sound(stack->getCreature(), endMoving));

	myAnim->setType(CCreatureAnim::MOVE_END);

	myAnim->onAnimationReset += std::bind(&CMovementEndAnimation::endAnim, this);

	return true;
}

void CMovementEndAnimation::endAnim()
{
	CBattleAnimation::endAnim();

	if(myAnim->getType() != CCreatureAnim::DEAD)
		myAnim->setType(CCreatureAnim::HOLDING); //resetting to default

	CCS->curh->show();
	delete this;
}

CMovementStartAnimation::CMovementStartAnimation(CBattleInterface * _owner, const CStack * _stack)
: CBattleStackAnimation(_owner, _stack)
{
	logAnim->debug("Created movement start anim for %s", stack->getName());
}

bool CMovementStartAnimation::init()
{
	if( !isEarliest(false) )
		return false;


	if(!stack || myAnim->isDead())
	{
		CMovementStartAnimation::endAnim();
		return false;
	}

	CCS->soundh->playSound(battle_sound(stack->getCreature(), startMoving));
	myAnim->setType(CCreatureAnim::MOVE_START);
	myAnim->onAnimationReset += std::bind(&CMovementStartAnimation::endAnim, this);

	return true;
}

void CMovementStartAnimation::endAnim()
{
	CBattleAnimation::endAnim();

	delete this;
}

CReverseAnimation::CReverseAnimation(CBattleInterface * _owner, const CStack * stack, BattleHex dest, bool _priority)
: CBattleStackAnimation(_owner, stack), hex(dest), priority(_priority)
{
	logAnim->debug("Created reverse anim for %s", stack->getName());
}

bool CReverseAnimation::init()
{
	if(myAnim == nullptr || myAnim->isDead())
	{
		endAnim();
		return false; //there is no such creature
	}

	if(!priority && !isEarliest(false))
		return false;

	if(myAnim->framesInGroup(CCreatureAnim::TURN_L))
	{
		myAnim->setType(CCreatureAnim::TURN_L);
		myAnim->onAnimationReset += std::bind(&CReverseAnimation::setupSecondPart, this);
	}
	else
	{
		setupSecondPart();
	}
	return true;
}

void CReverseAnimation::endAnim()
{
	CBattleAnimation::endAnim();
	if( stack->alive() )//don't do that if stack is dead
		myAnim->setType(CCreatureAnim::HOLDING);

	delete this;
}

void CBattleStackAnimation::rotateStack(BattleHex hex)
{
	setStackFacingRight(stack, !stackFacingRight(stack));

	stackAnimation(stack)->pos = owner->stacksController->getStackPositionAtHex(hex, stack);
}

void CReverseAnimation::setupSecondPart()
{
	if(!stack)
	{
		endAnim();
		return;
	}

	rotateStack(hex);

	if(myAnim->framesInGroup(CCreatureAnim::TURN_R))
	{
		myAnim->setType(CCreatureAnim::TURN_R);
		myAnim->onAnimationReset += std::bind(&CReverseAnimation::endAnim, this);
	}
	else
		endAnim();
}

CRangedAttackAnimation::CRangedAttackAnimation(CBattleInterface * owner_, const CStack * attacker, BattleHex dest_, const CStack * defender)
	: CAttackAnimation(owner_, attacker, dest_, defender)
{

}


CShootingAnimation::CShootingAnimation(CBattleInterface * _owner, const CStack * attacker, BattleHex _dest, const CStack * _attacked, bool _catapult, int _catapultDmg)
	: CRangedAttackAnimation(_owner, attacker, _dest, _attacked),
	catapultDamage(_catapultDmg),
	projectileEmitted(false)
{
	logAnim->debug("Created shooting anim for %s", stack->getName());
}

bool CShootingAnimation::init()
{
	if( !CAttackAnimation::checkInitialConditions() )
		return false;

	if(!attackingStack || myAnim->isDead())
	{
		//FIXME: how is this possible?
		endAnim();
		return false;
	}

	//reverse unit if necessary
	if (attackingStack && attackedStack && owner->getCurrentPlayerInterface()->cb->isToReverse(attackingStack->getPosition(), attackedStack->getPosition(), stackFacingRight(attackingStack), attackingStack->doubleWide(), stackFacingRight(attackedStack)))
	{
		owner->stacksController->addNewAnim(new CReverseAnimation(owner, attackingStack, attackingStack->getPosition(), true));
		return false;
	}

	//FIXME: this cause freeze
	// opponent must face attacker ( = different directions) before he can be attacked
	//if (attackingStack && attackedStack && owner->creDir[attackingStack->ID] == owner->creDir[attackedStack->ID])
	//	return false;

	setAnimationGroup();
	shooting = true;
	return true;
}

void CShootingAnimation::setAnimationGroup()
{
	Point shooterPos = stackAnimation(attackingStack)->pos.topLeft();
	Point shotTarget = owner->stacksController->getStackPositionAtHex(dest, attackedStack) + Point(225, 225);

	//maximal angle in radians between straight horizontal line and shooting line for which shot is considered to be straight (absoulte value)
	static const double straightAngle = 0.2;

	double projectileAngle = atan2(shotTarget.y - shooterPos.y, std::abs(shotTarget.x - shooterPos.x));

	// Calculate projectile start position. Offsets are read out of the CRANIM.TXT.
	if (projectileAngle > straightAngle)
		group = CCreatureAnim::SHOOT_UP;
	else if (projectileAngle < -straightAngle)
		group = CCreatureAnim::SHOOT_DOWN;
	else
		group = CCreatureAnim::SHOOT_FRONT;
}

void CShootingAnimation::initializeProjectile()
{
	const CCreature *shooterInfo = attackingStack->getCreature();

	if(shooterInfo->idNumber == CreatureID::ARROW_TOWERS)
		shooterInfo = owner->siegeController->getTurretCreature();

	Point shotTarget = owner->stacksController->getStackPositionAtHex(dest, attackedStack) + Point(225, 225);
	Point shotOrigin = stackAnimation(attackingStack)->pos.topLeft() + Point(222, 265);
	int multiplier = stackFacingRight(attackingStack) ? 1 : -1;

	if (group == CCreatureAnim::SHOOT_UP)
	{
		shotOrigin.x += ( -25 + shooterInfo->animation.upperRightMissleOffsetX ) * multiplier;
		shotOrigin.y += shooterInfo->animation.upperRightMissleOffsetY;
	}
	else if (group == CCreatureAnim::SHOOT_DOWN)
	{
		shotOrigin.x += ( -25 + shooterInfo->animation.lowerRightMissleOffsetX ) * multiplier;
		shotOrigin.y += shooterInfo->animation.lowerRightMissleOffsetY;
	}
	else if (group == CCreatureAnim::SHOOT_FRONT)
	{
		shotOrigin.x += ( -25 + shooterInfo->animation.rightMissleOffsetX ) * multiplier;
		shotOrigin.y += shooterInfo->animation.rightMissleOffsetY;
	}
	else
	{
		assert(0);
	}

	owner->projectilesController->createProjectile(attackingStack, attackedStack, shotOrigin, shotTarget);
}

void CShootingAnimation::emitProjectile()
{
	//owner->projectilesController->fireStackProjectile(attackingStack);
	projectileEmitted = true;
}

void CShootingAnimation::nextFrame()
{
	for(auto & it : pendingAnimations())
	{
		CMovementStartAnimation * anim = dynamic_cast<CMovementStartAnimation *>(it.first);
		CReverseAnimation * anim2 = dynamic_cast<CReverseAnimation *>(it.first);
		if( (anim && anim->stack->ID == stack->ID) || (anim2 && anim2->stack->ID == stack->ID && anim2->priority ) )
			return;
	}

	if (!projectileEmitted)
	{
		const CCreature *shooterInfo = attackingStack->getCreature();

		if(shooterInfo->idNumber == CreatureID::ARROW_TOWERS)
			shooterInfo = owner->siegeController->getTurretCreature();

		// animation should be paused if there is an active projectile
		if ( stackAnimation(attackingStack)->getCurrentFrame() >= shooterInfo->animation.attackClimaxFrame )
		{
			initializeProjectile();
			emitProjectile();
			return;
		}
	}

	if (projectileEmitted && owner->projectilesController->hasActiveProjectile(attackingStack))
		return; // projectile in air, pause animation


	CAttackAnimation::nextFrame();
}

void CShootingAnimation::endAnim()
{
	// FIXME: is this possible? Animation is over but we're yet to fire projectile?
	if (!projectileEmitted)
	{
		initializeProjectile();
		emitProjectile();
	}

	// play wall hit/miss sound for catapult attack
	if(!attackedStack)
	{
		if(catapultDamage > 0)
		{
			CCS->soundh->playSound("WALLHIT");
		}
		else
		{
			CCS->soundh->playSound("WALLMISS");
		}
	}

	CAttackAnimation::endAnim();
	delete this;
}

CCastAnimation::CCastAnimation(CBattleInterface * owner_, const CStack * attacker, BattleHex dest_, const CStack * defender)
	: CRangedAttackAnimation(owner_, attacker, dest_, defender)
{
	if(!dest_.isValid() && defender)
		dest = defender->getPosition();
}

bool CCastAnimation::init()
{
	if(!CAttackAnimation::checkInitialConditions())
		return false;

	if(!attackingStack || myAnim->isDead())
	{
		endAnim();
		return false;
	}

	//reverse unit if necessary
	if(attackedStack)
	{
		if(owner->getCurrentPlayerInterface()->cb->isToReverse(attackingStack->getPosition(), attackedStack->getPosition(), stackFacingRight(attackingStack), attackingStack->doubleWide(), stackFacingRight(attackedStack)))
		{
			owner->stacksController->addNewAnim(new CReverseAnimation(owner, attackingStack, attackingStack->getPosition(), true));
			return false;
		}
	}
	else
	{
		if(dest.isValid() && owner->getCurrentPlayerInterface()->cb->isToReverse(attackingStack->getPosition(), dest, stackFacingRight(attackingStack), false, false))
		{
			owner->stacksController->addNewAnim(new CReverseAnimation(owner, attackingStack, attackingStack->getPosition(), true));
			return false;
		}
	}

	//TODO: display spell projectile here

	static const double straightAngle = 0.2;


	Point fromPos;
	Point destPos;

	// NOTE: two lines below return different positions (very notable with 2-hex creatures). Obtaining via creanims seems to be more precise
	fromPos = stackAnimation(attackingStack)->pos.topLeft();
	//xycoord = owner->stacksController->getStackPositionAtHex(shooter->getPosition(), shooter);

	destPos = owner->stacksController->getStackPositionAtHex(dest, attackedStack);


	double projectileAngle = atan2(fabs((double)destPos.y - fromPos.y), fabs((double)destPos.x - fromPos.x));
	if(attackingStack->getPosition() < dest)
		projectileAngle = -projectileAngle;


	if(projectileAngle > straightAngle)
		group = CCreatureAnim::VCMI_CAST_UP;
	else if(projectileAngle < -straightAngle)
		group = CCreatureAnim::VCMI_CAST_DOWN;
	else
		group = CCreatureAnim::VCMI_CAST_FRONT;

	//fall back to H3 cast/2hex
	//even if creature have 2hex attack instead of cast it is ok since we fall back to attack anyway
	if(myAnim->framesInGroup(group) == 0)
	{
		if(projectileAngle > straightAngle)
			group = CCreatureAnim::CAST_UP;
		else if(projectileAngle < -straightAngle)
			group = CCreatureAnim::CAST_DOWN;
		else
			group = CCreatureAnim::CAST_FRONT;
	}

	//fall back to ranged attack
	if(myAnim->framesInGroup(group) == 0)
	{
		if(projectileAngle > straightAngle)
			group = CCreatureAnim::SHOOT_UP;
		else if(projectileAngle < -straightAngle)
			group = CCreatureAnim::SHOOT_DOWN;
		else
			group = CCreatureAnim::SHOOT_FRONT;
	}

	//fall back to normal attack
	if(myAnim->framesInGroup(group) == 0)
	{
		if(projectileAngle > straightAngle)
			group = CCreatureAnim::ATTACK_UP;
		else if(projectileAngle < -straightAngle)
			group = CCreatureAnim::ATTACK_DOWN;
		else
			group = CCreatureAnim::ATTACK_FRONT;
	}

	return true;
}

void CCastAnimation::nextFrame()
{
	for(auto & it : pendingAnimations())
	{
		CReverseAnimation * anim = dynamic_cast<CReverseAnimation *>(it.first);
		if(anim && anim->stack->ID == stack->ID && anim->priority)
			return;
	}

	if(myAnim->getType() != group)
	{
		myAnim->setType(group);
		myAnim->onAnimationReset += std::bind(&CAttackAnimation::endAnim, this);
	}

	CBattleAnimation::nextFrame();
}


void CCastAnimation::endAnim()
{
	CAttackAnimation::endAnim();
	delete this;
}


CEffectAnimation::CEffectAnimation(CBattleInterface * _owner, std::string _customAnim, int _x, int _y, int _dx, int _dy, bool _Vflip, bool _alignToBottom)
	: CBattleAnimation(_owner),
	destTile(BattleHex::INVALID),
	x(_x),
	y(_y),
	dx(_dx),
	dy(_dy),
	Vflip(_Vflip),
	alignToBottom(_alignToBottom)
{
	logAnim->debug("Created effect animation %s", _customAnim);

	customAnim = std::make_shared<CAnimation>(_customAnim);
}

CEffectAnimation::CEffectAnimation(CBattleInterface * _owner, std::shared_ptr<CAnimation> _customAnim, int _x, int _y, int _dx, int _dy)
	: CBattleAnimation(_owner),
	destTile(BattleHex::INVALID),
	customAnim(_customAnim),
	x(_x),
	y(_y),
	dx(_dx),
	dy(_dy),
	Vflip(false),
	alignToBottom(false)
{
	logAnim->debug("Created custom effect animation");
}


CEffectAnimation::CEffectAnimation(CBattleInterface * _owner, std::string _customAnim, BattleHex _destTile, bool _Vflip, bool _alignToBottom)
	: CBattleAnimation(_owner),
	destTile(_destTile),
	x(-1),
	y(-1),
	dx(0),
	dy(0),
	Vflip(_Vflip),
	alignToBottom(_alignToBottom)
{
	logAnim->debug("Created effect animation %s", _customAnim);
	customAnim = std::make_shared<CAnimation>(_customAnim);
}

bool CEffectAnimation::init()
{
	if(!isEarliest(true))
		return false;

	const bool areaEffect = (!destTile.isValid() && x == -1 && y == -1);

	std::shared_ptr<CAnimation> animation = customAnim;

	animation->preload();
	if(Vflip)
		animation->verticalFlip();

	auto first = animation->getImage(0, 0, true);
	if(!first)
	{
		endAnim();
		return false;
	}

	if(areaEffect) //f.e. armageddon
	{
		for(int i=0; i * first->width() < owner->pos.w ; ++i)
		{
			for(int j=0; j * first->height() < owner->pos.h ; ++j)
			{
				BattleEffect be;
				be.effectID = ID;
				be.animation = animation;
				be.currentFrame = 0;

				be.x = i * first->width() + owner->pos.x;
				be.y = j * first->height() + owner->pos.y;
				be.position = BattleHex::INVALID;

				owner->effectsController->battleEffects.push_back(be);
			}
		}
	}
	else // Effects targeted at a specific creature/hex.
	{
		const CStack * destStack = owner->getCurrentPlayerInterface()->cb->battleGetStackByPos(destTile, false);
		BattleEffect be;
		be.effectID = ID;
		be.animation = animation;
		be.currentFrame = 0;


		//todo: lightning anim frame count override

//			if(effect == 1)
//				be.maxFrame = 3;

		be.x = x;
		be.y = y;
		if(destTile.isValid())
		{
			Rect tilePos = owner->fieldController->hexPosition(destTile);
			if(x == -1)
				be.x = tilePos.x + tilePos.w/2 - first->width()/2;
			if(y == -1)
			{
				if(alignToBottom)
					be.y = tilePos.y + tilePos.h - first->height();
				else
					be.y = tilePos.y - first->height()/2;
			}

			// Correction for 2-hex creatures.
			if(destStack != nullptr && destStack->doubleWide())
				be.x += (destStack->side == BattleSide::ATTACKER ? -1 : 1)*tilePos.w/2;
		}

		assert(be.x != -1 && be.y != -1);

		//Indicate if effect should be drawn on top of everything or just on top of the hex
		be.position = destTile;

		owner->effectsController->battleEffects.push_back(be);
	}
	return true;
}

void CEffectAnimation::nextFrame()
{
	//notice: there may be more than one effect in owner->battleEffects correcponding to this animation (ie. armageddon)
	for(auto & elem : owner->effectsController->battleEffects)
	{
		if(elem.effectID == ID)
		{
			elem.currentFrame += AnimationControls::getSpellEffectSpeed() * GH.mainFPSmng->getElapsedMilliseconds() / 1000;

			if(elem.currentFrame >= elem.animation->size())
			{
				endAnim();
				break;
			}
			else
			{
				elem.x += dx;
				elem.y += dy;
			}
		}
	}
}

void CEffectAnimation::endAnim()
{
	CBattleAnimation::endAnim();

	boost::range::remove_if(owner->effectsController->battleEffects,
		[&](const BattleEffect & elem)
		{
			return elem.effectID == ID;
		});

	delete this;
}
