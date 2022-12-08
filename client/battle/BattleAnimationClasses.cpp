/*
 * BattleAnimationClasses.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleAnimationClasses.h"

#include <boost/math/constants/constants.hpp>

#include "BattleInterfaceClasses.h"
#include "BattleInterface.h"
#include "BattleProjectileController.h"
#include "BattleSiegeController.h"
#include "BattleFieldController.h"
#include "BattleEffectsController.h"
#include "BattleStacksController.h"
#include "CreatureAnimation.h"

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

CBattleAnimation::CBattleAnimation(BattleInterface & owner)
	: owner(owner),
	  ID(owner.stacksController->animIDhelper++),
	  initialized(false)
{
	logAnim->trace("Animation #%d created", ID);
}

bool CBattleAnimation::tryInitialize()
{
	assert(!initialized);

	if ( init() )
	{
		initialized = true;
		return true;
	}
	return false;
}

bool CBattleAnimation::isInitialized()
{
	return initialized;
}

CBattleAnimation::~CBattleAnimation()
{
	logAnim->trace("Animation #%d ended, type is %s", ID, typeid(this).name());
	for(auto & elem : pendingAnimations())
	{
		if(elem == this)
			elem = nullptr;
	}
	logAnim->trace("Animation #%d deleted", ID);
}

std::vector<CBattleAnimation *> & CBattleAnimation::pendingAnimations()
{
	return owner.stacksController->currentAnimations;
}

std::shared_ptr<CreatureAnimation> CBattleAnimation::stackAnimation(const CStack * stack) const
{
	return owner.stacksController->stackAnimation[stack->ID];
}

bool CBattleAnimation::stackFacingRight(const CStack * stack)
{
	return owner.stacksController->stackFacingRight[stack->ID];
}

void CBattleAnimation::setStackFacingRight(const CStack * stack, bool facingRight)
{
	owner.stacksController->stackFacingRight[stack->ID] = facingRight;
}

bool CBattleAnimation::checkInitialConditions()
{
	int lowestMoveID = ID;
	auto * thAnim = dynamic_cast<CBattleStackAnimation *>(this);
	auto * thSen = dynamic_cast<CPointEffectAnimation *>(this);

	for(auto & elem : pendingAnimations())
	{
		auto * sen = dynamic_cast<CPointEffectAnimation *>(elem);

		// all effect animations can play concurrently with each other
		if(sen && thSen && sen != thSen)
			continue;

		auto * revAnim = dynamic_cast<CReverseAnimation *>(elem);

		// if there is high-priority reverse animation affecting our stack then this animation will wait
		if(revAnim && thAnim && revAnim && revAnim->stack->ID == thAnim->stack->ID && revAnim->priority)
			return false;

		if(elem)
			vstd::amin(lowestMoveID, elem->ID);
	}
	return ID == lowestMoveID;
}

CBattleStackAnimation::CBattleStackAnimation(BattleInterface & owner, const CStack * stack)
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
		myAnim->onAnimationReset += [&](){ delete this; };
	}

	if(!soundPlayed)
	{
		if(shooting)
			CCS->soundh->playSound(battle_sound(getCreature(), shoot));
		else
			CCS->soundh->playSound(battle_sound(getCreature(), attack));
		soundPlayed = true;
	}
}

CAttackAnimation::~CAttackAnimation()
{
	myAnim->setType(ECreatureAnimType::HOLDING);
}

bool CAttackAnimation::checkInitialConditions()
{
	for(auto & elem : pendingAnimations())
	{
		CBattleStackAnimation * stAnim = dynamic_cast<CBattleStackAnimation *>(elem);
		CReverseAnimation * revAnim = dynamic_cast<CReverseAnimation *>(stAnim);

		if(revAnim && attackedStack) // enemy must be fully reversed
		{
			if (revAnim->stack->ID == attackedStack->ID)
				return false;
		}
	}
	return CBattleAnimation::checkInitialConditions();
}

const CCreature * CAttackAnimation::getCreature() const
{
	if (attackingStack->getCreature()->idNumber == CreatureID::ARROW_TOWERS)
		return owner.siegeController->getTurretCreature();
	else
		return attackingStack->getCreature();
}

CAttackAnimation::CAttackAnimation(BattleInterface & owner, const CStack *attacker, BattleHex _dest, const CStack *defender)
	: CBattleStackAnimation(owner, attacker),
	  shooting(false),
	  group(ECreatureAnimType::SHOOT_FRONT),
	  soundPlayed(false),
	  dest(_dest),
	  attackedStack(defender),
	  attackingStack(attacker)
{
	assert(attackingStack && "attackingStack is nullptr in CBattleAttack::CBattleAttack !\n");
	attackingStackPosBeforeReturn = attackingStack->getPosition();
}

CDefenceAnimation::CDefenceAnimation(StackAttackedInfo _attackedInfo, BattleInterface & owner)
	: CBattleStackAnimation(owner, _attackedInfo.defender),
	  attacker(_attackedInfo.attacker),
	  rangedAttack(_attackedInfo.indirectAttack),
	  killed(_attackedInfo.killed),
	  timeToWait(0)
{
	logAnim->debug("Created defence anim for %s", _attackedInfo.defender->getName());
}

bool CDefenceAnimation::init()
{
	ui32 lowestMoveID = ID;
	for(auto & elem : pendingAnimations())
	{

		auto * defAnim = dynamic_cast<CDefenceAnimation *>(elem);
		if(defAnim && defAnim->stack->ID != stack->ID)
			continue;

		auto * attAnim = dynamic_cast<CAttackAnimation *>(elem);
		if(attAnim && attAnim->stack->ID != stack->ID)
			continue;

		auto * sen = dynamic_cast<CPointEffectAnimation *>(elem);
		if (sen && attacker == nullptr)
			return false;

		if (sen)
			continue;

		CReverseAnimation * animAsRev = dynamic_cast<CReverseAnimation *>(elem);

		if(animAsRev)
			return false;

		if(elem)
			vstd::amin(lowestMoveID, elem->ID);
	}

	if(ID > lowestMoveID)
		return false;


	//reverse unit if necessary
	if(attacker && owner.getCurrentPlayerInterface()->cb->isToReverse(stack->getPosition(), attacker->getPosition(), stackFacingRight(stack), attacker->doubleWide(), stackFacingRight(attacker)))
	{
		owner.stacksController->addNewAnim(new CReverseAnimation(owner, stack, stack->getPosition(), true));
		return false;
	}
	//unit reversed

	if(rangedAttack && attacker != nullptr && owner.projectilesController->hasActiveProjectile(attacker)) //delay hit animation
	{
		return false;
	}

	// synchronize animation with attacker, unless defending or attacked by shooter:
	// wait for 1/2 of attack animation
	if (!rangedAttack && getMyAnimType() != ECreatureAnimType::DEFENCE)
	{
		float frameLength = AnimationControls::getCreatureAnimationSpeed(
								  stack->getCreature(), stackAnimation(stack).get(), getMyAnimType());

		timeToWait = myAnim->framesInGroup(getMyAnimType()) * frameLength / 2;

		myAnim->setType(ECreatureAnimType::HOLDING);
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

ECreatureAnimType::Type CDefenceAnimation::getMyAnimType()
{
	if(killed)
	{
		if(rangedAttack && myAnim->framesInGroup(ECreatureAnimType::DEATH_RANGED) > 0)
			return ECreatureAnimType::DEATH_RANGED;
		else
			return ECreatureAnimType::DEATH;
	}

	if(stack->defendingAnim)
		return ECreatureAnimType::DEFENCE;
	else
		return ECreatureAnimType::HITTED;
}

void CDefenceAnimation::startAnimation()
{
	CCS->soundh->playSound(getMySound());
	myAnim->setType(getMyAnimType());
	myAnim->onAnimationReset += [&](){ delete this; };
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

CDefenceAnimation::~CDefenceAnimation()
{
	if(killed)
	{
		if(rangedAttack && myAnim->framesInGroup(ECreatureAnimType::DEAD_RANGED) > 0)
			myAnim->setType(ECreatureAnimType::DEAD_RANGED);
		else
			myAnim->setType(ECreatureAnimType::DEAD);
	}
	else
	{
		myAnim->setType(ECreatureAnimType::HOLDING);
	}
}

CDummyAnimation::CDummyAnimation(BattleInterface & owner, int howManyFrames)
	: CBattleAnimation(owner),
	  counter(0),
	  howMany(howManyFrames)
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
		delete this;
}

bool CMeleeAttackAnimation::init()
{
	if(!CAttackAnimation::checkInitialConditions())
		return false;

	if(!attackingStack || myAnim->isDeadOrDying())
	{
		delete this;
		return false;
	}

	bool toReverse = owner.getCurrentPlayerInterface()->cb->isToReverse(attackingStackPosBeforeReturn, attackedStack->getPosition(), stackFacingRight(stack), attackedStack->doubleWide(), stackFacingRight(attackedStack));

	if(toReverse)
	{
		owner.stacksController->addNewAnim(new CReverseAnimation(owner, stack, attackingStackPosBeforeReturn, true));
		return false;
	}

	// opponent must face attacker ( = different directions) before he can be attacked
	if(attackingStack && attackedStack &&
		stackFacingRight(attackingStack) == stackFacingRight(attackedStack))
		return false;

	//reversed

	shooting = false;

	static const ECreatureAnimType::Type mutPosToGroup[] =
	{
		ECreatureAnimType::ATTACK_UP,
		ECreatureAnimType::ATTACK_UP,
		ECreatureAnimType::ATTACK_FRONT,
		ECreatureAnimType::ATTACK_DOWN,
		ECreatureAnimType::ATTACK_DOWN,
		ECreatureAnimType::ATTACK_FRONT
	};

	static const ECreatureAnimType::Type mutPosToGroup2H[] =
	{
		ECreatureAnimType::VCMI_2HEX_UP,
		ECreatureAnimType::VCMI_2HEX_UP,
		ECreatureAnimType::VCMI_2HEX_FRONT,
		ECreatureAnimType::VCMI_2HEX_DOWN,
		ECreatureAnimType::VCMI_2HEX_DOWN,
		ECreatureAnimType::VCMI_2HEX_FRONT
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
			ECreatureAnimType::Type group2H = mutPosToGroup2H[mutPos];
			if(myAnim->framesInGroup(group2H)>0)
				group = group2H;
		}

		break;
	default:
		logGlobal->error("Critical Error! Wrong dest in stackAttacking! dest: %d; attacking stack pos: %d; mutual pos: %d", dest.hex, attackingStackPosBeforeReturn, mutPos);
		group = ECreatureAnimType::ATTACK_FRONT;
		break;
	}

	return true;
}

CMeleeAttackAnimation::CMeleeAttackAnimation(BattleInterface & owner, const CStack * attacker, BattleHex _dest, const CStack * _attacked)
	: CAttackAnimation(owner, attacker, _dest, _attacked)
{
	logAnim->debug("Created melee attack anim for %s", attacker->getName());
}

CStackMoveAnimation::CStackMoveAnimation(BattleInterface & owner, const CStack * _stack, BattleHex _currentHex):
	CBattleStackAnimation(owner, _stack),
	currentHex(_currentHex)
{

}

bool CMovementAnimation::init()
{
	if( !CBattleAnimation::checkInitialConditions() )
		return false;

	if(!stack || myAnim->isDeadOrDying())
	{
		delete this;
		return false;
	}

	if(stackAnimation(stack)->framesInGroup(ECreatureAnimType::MOVING) == 0 ||
	   stack->hasBonus(Selector::typeSubtype(Bonus::FLYING, 1)))
	{
		//no movement or teleport, end immediately
		delete this;
		return false;
	}

	//reverse unit if necessary
	if(owner.stacksController->shouldRotate(stack, oldPos, currentHex))
	{
		// it seems that H3 does NOT plays full rotation animation here in most situations
		// Logical since it takes quite a lot of time
		if (curentMoveIndex == 0) // full rotation only for moving towards first tile.
		{
			owner.stacksController->addNewAnim(new CReverseAnimation(owner, stack, oldPos, true));
			return false;
		}
		else
		{
			rotateStack(oldPos);
		}
	}

	if(myAnim->getType() != ECreatureAnimType::MOVING)
	{
		myAnim->setType(ECreatureAnimType::MOVING);
	}

	if (owner.moveSoundHander == -1)
	{
		owner.moveSoundHander = CCS->soundh->playSound(battle_sound(stack->getCreature(), move), -1);
	}

	Point begPosition = owner.stacksController->getStackPositionAtHex(oldPos, stack);
	Point endPosition = owner.stacksController->getStackPositionAtHex(currentHex, stack);

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
		Point coords = owner.stacksController->getStackPositionAtHex(currentHex, stack);
		myAnim->pos = coords;

		// true if creature haven't reached the final destination hex
		if ((curentMoveIndex + 1) < destTiles.size())
		{
			// update the next hex field which has to be reached by the stack
			curentMoveIndex++;
			oldPos = currentHex;
			currentHex = destTiles[curentMoveIndex];

			// request re-initialization
			initialized = false;
		}
		else
			delete this;
	}
}

CMovementAnimation::~CMovementAnimation()
{
	assert(stack);

	myAnim->pos = owner.stacksController->getStackPositionAtHex(currentHex, stack);
	owner.stacksController->addNewAnim(new CMovementEndAnimation(owner, stack, currentHex));

	if(owner.moveSoundHander != -1)
	{
		CCS->soundh->stopSound(owner.moveSoundHander);
		owner.moveSoundHander = -1;
	}
}

CMovementAnimation::CMovementAnimation(BattleInterface & owner, const CStack *_stack, std::vector<BattleHex> _destTiles, int _distance)
	: CStackMoveAnimation(owner, _stack, _destTiles.front()),
	  destTiles(_destTiles),
	  curentMoveIndex(0),
	  oldPos(stack->getPosition()),
	  begX(0), begY(0),
	  distanceX(0), distanceY(0),
	  timeToMove(0.0),
	  progress(0.0)
{
	logAnim->debug("Created movement anim for %s", stack->getName());
}

CMovementEndAnimation::CMovementEndAnimation(BattleInterface & owner, const CStack * _stack, BattleHex destTile)
: CStackMoveAnimation(owner, _stack, destTile)
{
	logAnim->debug("Created movement end anim for %s", stack->getName());
}

bool CMovementEndAnimation::init()
{
	//if( !isEarliest(true) )
	//	return false;

	if(!stack || myAnim->framesInGroup(ECreatureAnimType::MOVE_END) == 0 ||
		myAnim->isDeadOrDying())
	{
		delete this;
		return false;
	}

	CCS->soundh->playSound(battle_sound(stack->getCreature(), endMoving));

	myAnim->setType(ECreatureAnimType::MOVE_END);

	myAnim->onAnimationReset += [&](){ delete this; };

	return true;
}

CMovementEndAnimation::~CMovementEndAnimation()
{
	if(myAnim->getType() != ECreatureAnimType::DEAD)
		myAnim->setType(ECreatureAnimType::HOLDING); //resetting to default

	CCS->curh->show();
}

CMovementStartAnimation::CMovementStartAnimation(BattleInterface & owner, const CStack * _stack)
	: CStackMoveAnimation(owner, _stack, _stack->getPosition())
{
	logAnim->debug("Created movement start anim for %s", stack->getName());
}

bool CMovementStartAnimation::init()
{
	if( !CBattleAnimation::checkInitialConditions() )
		return false;


	if(!stack || myAnim->isDeadOrDying())
	{
		delete this;
		return false;
	}

	CCS->soundh->playSound(battle_sound(stack->getCreature(), startMoving));
	myAnim->setType(ECreatureAnimType::MOVE_START);
	myAnim->onAnimationReset += [&](){ delete this; };

	return true;
}

CReverseAnimation::CReverseAnimation(BattleInterface & owner, const CStack * stack, BattleHex dest, bool _priority)
	: CStackMoveAnimation(owner, stack, dest),
	  priority(_priority)
{
	logAnim->debug("Created reverse anim for %s", stack->getName());
}

bool CReverseAnimation::init()
{
	if(myAnim == nullptr || myAnim->isDeadOrDying())
	{
		delete this;
		return false; //there is no such creature
	}

	if(!priority && !CBattleAnimation::checkInitialConditions())
		return false;

	if(myAnim->framesInGroup(ECreatureAnimType::TURN_L))
	{
		myAnim->setType(ECreatureAnimType::TURN_L);
		myAnim->onAnimationReset += std::bind(&CReverseAnimation::setupSecondPart, this);
	}
	else
	{
		setupSecondPart();
	}
	return true;
}

CReverseAnimation::~CReverseAnimation()
{
	if( stack && stack->alive() )//don't do that if stack is dead
		myAnim->setType(ECreatureAnimType::HOLDING);
}

void CBattleStackAnimation::rotateStack(BattleHex hex)
{
	setStackFacingRight(stack, !stackFacingRight(stack));

	stackAnimation(stack)->pos = owner.stacksController->getStackPositionAtHex(hex, stack);
}

void CReverseAnimation::setupSecondPart()
{
	if(!stack)
	{
		delete this;
		return;
	}

	rotateStack(currentHex);

	if(myAnim->framesInGroup(ECreatureAnimType::TURN_R))
	{
		myAnim->setType(ECreatureAnimType::TURN_R);
		myAnim->onAnimationReset += [&](){ delete this; };
	}
	else
		delete this;
}

CRangedAttackAnimation::CRangedAttackAnimation(BattleInterface & owner, const CStack * attacker, BattleHex dest_, const CStack * defender)
	: CAttackAnimation(owner, attacker, dest_, defender),
	  projectileEmitted(false)
{
	logAnim->info("Ranged attack animation created");
}

bool CRangedAttackAnimation::init()
{
	if( !CAttackAnimation::checkInitialConditions() )
		return false;

	assert(attackingStack);
	assert(!myAnim->isDeadOrDying());

	if(!attackingStack || myAnim->isDeadOrDying())
	{
		//FIXME: how is this possible?
		logAnim->warn("Shooting animation has not started yet but attacker is dead! Aborting...");
		delete this;
		return false;
	}

	//reverse unit if necessary
	if (attackingStack && attackedStack && owner.getCurrentPlayerInterface()->cb->isToReverse(attackingStack->getPosition(), attackedStack->getPosition(), stackFacingRight(attackingStack), attackingStack->doubleWide(), stackFacingRight(attackedStack)))
	{
		owner.stacksController->addNewAnim(new CReverseAnimation(owner, attackingStack, attackingStack->getPosition(), true));
		return false;
	}

	logAnim->info("Ranged attack animation initialized");
	setAnimationGroup();
	initializeProjectile();
	shooting = true;
	return true;
}

void CRangedAttackAnimation::setAnimationGroup()
{
	Point shooterPos = stackAnimation(attackingStack)->pos.topLeft();
	Point shotTarget = owner.stacksController->getStackPositionAtHex(dest, attackedStack);

	//maximal angle in radians between straight horizontal line and shooting line for which shot is considered to be straight (absoulte value)
	static const double straightAngle = 0.2;

	double projectileAngle = -atan2(shotTarget.y - shooterPos.y, std::abs(shotTarget.x - shooterPos.x));

	// Calculate projectile start position. Offsets are read out of the CRANIM.TXT.
	if (projectileAngle > straightAngle)
		group = getUpwardsGroup();
	else if (projectileAngle < -straightAngle)
		group = getDownwardsGroup();
	else
		group = getForwardGroup();
}

void CRangedAttackAnimation::initializeProjectile()
{
	const CCreature *shooterInfo = getCreature();
	Point shotTarget = owner.stacksController->getStackPositionAtHex(dest, attackedStack) + Point(225, 225);
	Point shotOrigin = stackAnimation(attackingStack)->pos.topLeft() + Point(222, 265);
	int multiplier = stackFacingRight(attackingStack) ? 1 : -1;

	if (group == getUpwardsGroup())
	{
		shotOrigin.x += ( -25 + shooterInfo->animation.upperRightMissleOffsetX ) * multiplier;
		shotOrigin.y += shooterInfo->animation.upperRightMissleOffsetY;
	}
	else if (group == getDownwardsGroup())
	{
		shotOrigin.x += ( -25 + shooterInfo->animation.lowerRightMissleOffsetX ) * multiplier;
		shotOrigin.y += shooterInfo->animation.lowerRightMissleOffsetY;
	}
	else if (group == getForwardGroup())
	{
		shotOrigin.x += ( -25 + shooterInfo->animation.rightMissleOffsetX ) * multiplier;
		shotOrigin.y += shooterInfo->animation.rightMissleOffsetY;
	}
	else
	{
		assert(0);
	}

	createProjectile(shotOrigin, shotTarget);
}

void CRangedAttackAnimation::emitProjectile()
{
	logAnim->info("Ranged attack projectile emitted");
	owner.projectilesController->emitStackProjectile(attackingStack);
	projectileEmitted = true;
}

void CRangedAttackAnimation::nextFrame()
{
	for(auto & it : pendingAnimations())
	{
		CMovementStartAnimation * anim = dynamic_cast<CMovementStartAnimation *>(it);
		CReverseAnimation * anim2 = dynamic_cast<CReverseAnimation *>(it);
		if( (anim && anim->stack->ID == stack->ID) || (anim2 && anim2->stack->ID == stack->ID && anim2->priority ) )
		{
			assert(0); // FIXME: our stack started to move even though we are playing shooting animation? How?
			return;
		}
	}

	// animation should be paused if there is an active projectile
	if (projectileEmitted)
	{
		if (owner.projectilesController->hasActiveProjectile(attackingStack))
			stackAnimation(attackingStack)->pause();
		else
			stackAnimation(attackingStack)->play();
	}

	CAttackAnimation::nextFrame();

	if (!projectileEmitted)
	{
		logAnim->info("Ranged attack executing, %d / %d / %d",
					  stackAnimation(attackingStack)->getCurrentFrame(),
					  getAttackClimaxFrame(),
					  stackAnimation(attackingStack)->framesInGroup(group));

		// emit projectile once animation playback reached "climax" frame
		if ( stackAnimation(attackingStack)->getCurrentFrame() >= getAttackClimaxFrame() )
		{
			emitProjectile();
			stackAnimation(attackingStack)->pause();
			return;
		}
	}
}

CRangedAttackAnimation::~CRangedAttackAnimation()
{
	logAnim->info("Ranged attack animation is over");
	//FIXME: this assert triggers under some unclear, rare conditions. Possibly - if game window is inactive and/or in foreground/minimized?
	assert(!owner.projectilesController->hasActiveProjectile(attackingStack));
	assert(projectileEmitted);

	// FIXME: is this possible? Animation is over but we're yet to fire projectile?
	if (!projectileEmitted)
	{
		logAnim->warn("Shooting animation has finished but projectile was not emitted! Emitting it now...");
		emitProjectile();
	}
}

CShootingAnimation::CShootingAnimation(BattleInterface & owner, const CStack * attacker, BattleHex _dest, const CStack * _attacked)
	: CRangedAttackAnimation(owner, attacker, _dest, _attacked)
{
	logAnim->debug("Created shooting anim for %s", stack->getName());
}

void CShootingAnimation::createProjectile(const Point & from, const Point & dest) const
{
	owner.projectilesController->createProjectile(attackingStack, from, dest);
}

uint32_t CShootingAnimation::getAttackClimaxFrame() const
{
	const CCreature *shooterInfo = getCreature();
	return shooterInfo->animation.attackClimaxFrame;
}

ECreatureAnimType::Type CShootingAnimation::getUpwardsGroup() const
{
	return ECreatureAnimType::SHOOT_UP;
}

ECreatureAnimType::Type CShootingAnimation::getForwardGroup() const
{
	return ECreatureAnimType::SHOOT_FRONT;
}

ECreatureAnimType::Type CShootingAnimation::getDownwardsGroup() const
{
	return ECreatureAnimType::SHOOT_DOWN;
}

CCatapultAnimation::CCatapultAnimation(BattleInterface & owner, const CStack * attacker, BattleHex _dest, const CStack * _attacked, int _catapultDmg)
	: CShootingAnimation(owner, attacker, _dest, _attacked),
	catapultDamage(_catapultDmg),
	explosionEmitted(false)
{
	logAnim->debug("Created shooting anim for %s", stack->getName());
}

void CCatapultAnimation::nextFrame()
{
	CShootingAnimation::nextFrame();

	if ( explosionEmitted)
		return;

	if ( !projectileEmitted)
		return;

	if (owner.projectilesController->hasActiveProjectile(attackingStack))
		return;

	explosionEmitted = true;
	Point shotTarget = owner.stacksController->getStackPositionAtHex(dest, attackedStack) + Point(225, 225) - Point(126, 105);

	if(catapultDamage > 0)
		owner.stacksController->addNewAnim( new CPointEffectAnimation(owner, soundBase::WALLHIT, "SGEXPL.DEF", shotTarget));
	else
		owner.stacksController->addNewAnim( new CPointEffectAnimation(owner, soundBase::WALLMISS, "CSGRCK.DEF", shotTarget));
}

void CCatapultAnimation::createProjectile(const Point & from, const Point & dest) const
{
	owner.projectilesController->createCatapultProjectile(attackingStack, from, dest);
}


CCastAnimation::CCastAnimation(BattleInterface & owner, const CStack * attacker, BattleHex dest_, const CStack * defender, const CSpell * spell)
	: CRangedAttackAnimation(owner, attacker, dest_, defender),
	  spell(spell)
{
	assert(dest.isValid());// FIXME: when?

	if(!dest_.isValid() && defender)
		dest = defender->getPosition();
}

ECreatureAnimType::Type CCastAnimation::findValidGroup( const std::vector<ECreatureAnimType::Type> candidates ) const
{
	for ( auto group : candidates)
	{
		if(myAnim->framesInGroup(group) > 0)
			return group;
	}

	assert(0);
	return ECreatureAnimType::HOLDING;
}

ECreatureAnimType::Type CCastAnimation::getUpwardsGroup() const
{
	return findValidGroup({
		ECreatureAnimType::VCMI_CAST_UP,
		ECreatureAnimType::CAST_UP,
		ECreatureAnimType::SHOOT_UP,
		ECreatureAnimType::ATTACK_UP
	});
}

ECreatureAnimType::Type CCastAnimation::getForwardGroup() const
{
	return findValidGroup({
		ECreatureAnimType::VCMI_CAST_FRONT,
		ECreatureAnimType::CAST_FRONT,
		ECreatureAnimType::SHOOT_FRONT,
		ECreatureAnimType::ATTACK_FRONT
	});
}

ECreatureAnimType::Type CCastAnimation::getDownwardsGroup() const
{
	return findValidGroup({
		ECreatureAnimType::VCMI_CAST_DOWN,
		ECreatureAnimType::CAST_DOWN,
		ECreatureAnimType::SHOOT_DOWN,
		ECreatureAnimType::ATTACK_DOWN
	});
}

void CCastAnimation::createProjectile(const Point & from, const Point & dest) const
{
	if (!spell->animationInfo.projectile.empty())
		owner.projectilesController->createSpellProjectile(attackingStack, from, dest, spell);
}

uint32_t CCastAnimation::getAttackClimaxFrame() const
{
	//FIXME: allow defining this parameter in config file, separately from attackClimaxFrame of missile attacks
	uint32_t maxFrames = stackAnimation(attackingStack)->framesInGroup(group);

	if (maxFrames > 2)
		return maxFrames - 2;
	return 0;
}

CPointEffectAnimation::CPointEffectAnimation(BattleInterface & owner, soundBase::soundID sound, std::string animationName, int effects):
	CBattleAnimation(owner),
	animation(std::make_shared<CAnimation>(animationName)),
	sound(sound),
	effectFlags(effects),
	soundPlayed(false),
	soundFinished(false),
	effectFinished(false)
{
}

CPointEffectAnimation::CPointEffectAnimation(BattleInterface & owner, soundBase::soundID sound, std::string animationName, std::vector<BattleHex> hex, int effects):
	CPointEffectAnimation(owner, sound, animationName, effects)
{
	battlehexes = hex;
}

CPointEffectAnimation::CPointEffectAnimation(BattleInterface & owner, soundBase::soundID sound, std::string animationName, BattleHex hex, int effects):
	CPointEffectAnimation(owner, sound, animationName, effects)
{
	assert(hex.isValid());
	battlehexes.push_back(hex);
}

CPointEffectAnimation::CPointEffectAnimation(BattleInterface & owner, soundBase::soundID sound, std::string animationName, std::vector<Point> pos, int effects):
	CPointEffectAnimation(owner, sound, animationName, effects)
{
	positions = pos;
}

CPointEffectAnimation::CPointEffectAnimation(BattleInterface & owner, soundBase::soundID sound, std::string animationName, Point pos, int effects):
	CPointEffectAnimation(owner, sound, animationName, effects)
{
	positions.push_back(pos);
}

CPointEffectAnimation::CPointEffectAnimation(BattleInterface & owner, soundBase::soundID sound, std::string animationName, Point pos, BattleHex hex,   int effects):
	CPointEffectAnimation(owner, sound, animationName, effects)
{
	assert(hex.isValid());
	battlehexes.push_back(hex);
	positions.push_back(pos);
}

bool CPointEffectAnimation::init()
{
	if(!CBattleAnimation::checkInitialConditions())
		return false;

	animation->preload();

	auto first = animation->getImage(0, 0, true);
	if(!first)
	{
		delete this;
		return false;
	}

	if (screenFill())
	{
		for(int i=0; i * first->width() < owner.pos.w ; ++i)
			for(int j=0; j * first->height() < owner.pos.h ; ++j)
				positions.push_back(Point( owner.pos.x + i * first->width(), owner.pos.y + j * first->height()));
	}

	BattleEffect be;
	be.effectID = ID;
	be.animation = animation;
	be.currentFrame = 0;

	for (size_t i = 0; i < std::max(battlehexes.size(), positions.size()); ++i)
	{
		bool hasTile = i < battlehexes.size();
		bool hasPosition = i < positions.size();

		if (hasTile && !forceOnTop())
			be.position = battlehexes[i];
		else
			be.position = BattleHex::INVALID;

		if (hasPosition)
		{
			be.x = positions[i].x;
			be.y = positions[i].y;
		}
		else
		{
			const CStack * destStack = owner.getCurrentPlayerInterface()->cb->battleGetStackByPos(battlehexes[i], false);
			Rect tilePos = owner.fieldController->hexPositionAbsolute(battlehexes[i]);

			be.x = tilePos.x + tilePos.w/2 - first->width()/2;

			if(destStack && destStack->doubleWide()) // Correction for 2-hex creatures.
				be.x += (destStack->side == BattleSide::ATTACKER ? -1 : 1)*tilePos.w/2;

			if (alignToBottom())
				be.y = tilePos.y + tilePos.h - first->height();
			else
				be.y = tilePos.y - first->height()/2;
		}
		owner.effectsController->battleEffects.push_back(be);
	}
	return true;
}

void CPointEffectAnimation::nextFrame()
{
	playSound();
	playEffect();

	if (soundFinished && effectFinished)
	{
		//remove visual effect itself only if sound has finished as well - necessary for obstacles like force field
		clearEffect();
		delete this;
	}
}

bool CPointEffectAnimation::alignToBottom() const
{
	return effectFlags & ALIGN_TO_BOTTOM;
}

bool CPointEffectAnimation::waitForSound() const
{
	return effectFlags & WAIT_FOR_SOUND;
}

bool CPointEffectAnimation::forceOnTop() const
{
	return effectFlags & FORCE_ON_TOP;
}

bool CPointEffectAnimation::screenFill() const
{
	return effectFlags & SCREEN_FILL;
}

void CPointEffectAnimation::onEffectFinished()
{
	effectFinished = true;
}

void CPointEffectAnimation::onSoundFinished()
{
	soundFinished = true;
}

void CPointEffectAnimation::playSound()
{
	if (soundPlayed)
		return;

	soundPlayed = true;
	if (sound == soundBase::invalid)
	{
		onSoundFinished();
		return;
	}

	int channel = CCS->soundh->playSound(sound);

	if (!waitForSound() || channel == -1)
		onSoundFinished();
	else
		CCS->soundh->setCallback(channel, [&](){ onSoundFinished(); });
}

void CPointEffectAnimation::playEffect()
{
	if ( effectFinished )
		return;

	for(auto & elem : owner.effectsController->battleEffects)
	{
		if(elem.effectID == ID)
		{
			elem.currentFrame += AnimationControls::getSpellEffectSpeed() * GH.mainFPSmng->getElapsedMilliseconds() / 1000;

			if(elem.currentFrame >= elem.animation->size())
			{
				elem.currentFrame = elem.animation->size() - 1;
				onEffectFinished();
				break;
			}
		}
	}
}

void CPointEffectAnimation::clearEffect()
{
	auto & effects = owner.effectsController->battleEffects;

	vstd::erase_if(effects, [&](const BattleEffect & effect){
		return effect.effectID == ID;
	});
}

CPointEffectAnimation::~CPointEffectAnimation()
{
	assert(effectFinished);
	assert(soundFinished);
}

CWaitingAnimation::CWaitingAnimation(BattleInterface & owner):
	CBattleAnimation(owner)
{}

void CWaitingAnimation::nextFrame()
{
	// initialization conditions fulfilled, delay is over
	delete this;
}

CWaitingProjectileAnimation::CWaitingProjectileAnimation(BattleInterface & owner, const CStack * shooter):
	CWaitingAnimation(owner),
	shooter(shooter)
{}

bool CWaitingProjectileAnimation::init()
{
	for(auto & elem : pendingAnimations())
	{
		auto * attackAnim = dynamic_cast<CRangedAttackAnimation *>(elem);

		if( attackAnim && shooter && attackAnim->stack->ID == shooter->ID && !attackAnim->isInitialized() )
		{
			// there is ongoing ranged attack that involves our stack, but projectile was not created yet
			return false;
		}
	}

	if(owner.projectilesController->hasActiveProjectile(shooter))
		return false;

	return true;
}
