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

#include "BattleInterface.h"
#include "BattleInterfaceClasses.h"
#include "BattleProjectileController.h"
#include "BattleSiegeController.h"
#include "BattleFieldController.h"
#include "BattleEffectsController.h"
#include "BattleStacksController.h"
#include "CreatureAnimation.h"

#include "../CGameInfo.h"
#include "../CMusicHandler.h"
#include "../CPlayerInterface.h"
#include "../gui/CCursorHandler.h"
#include "../gui/CGuiHandler.h"

#include "../../CCallback.h"
#include "../../lib/CStack.h"

BattleAnimation::BattleAnimation(BattleInterface & owner)
	: owner(owner),
	  ID(owner.stacksController->animIDhelper++),
	  initialized(false)
{
	logAnim->trace("Animation #%d created", ID);
}

bool BattleAnimation::tryInitialize()
{
	assert(!initialized);

	if ( init() )
	{
		initialized = true;
		return true;
	}
	return false;
}

bool BattleAnimation::isInitialized()
{
	return initialized;
}

BattleAnimation::~BattleAnimation()
{
	logAnim->trace("Animation #%d ended, type is %s", ID, typeid(this).name());
	for(auto & elem : pendingAnimations())
	{
		if(elem == this)
			elem = nullptr;
	}
	logAnim->trace("Animation #%d deleted", ID);
}

std::vector<BattleAnimation *> & BattleAnimation::pendingAnimations()
{
	return owner.stacksController->currentAnimations;
}

std::shared_ptr<CreatureAnimation> BattleAnimation::stackAnimation(const CStack * stack) const
{
	return owner.stacksController->stackAnimation[stack->ID];
}

bool BattleAnimation::stackFacingRight(const CStack * stack)
{
	return owner.stacksController->stackFacingRight[stack->ID];
}

void BattleAnimation::setStackFacingRight(const CStack * stack, bool facingRight)
{
	owner.stacksController->stackFacingRight[stack->ID] = facingRight;
}

BattleStackAnimation::BattleStackAnimation(BattleInterface & owner, const CStack * stack)
	: BattleAnimation(owner),
	  myAnim(stackAnimation(stack)),
	  stack(stack)
{
	assert(myAnim);
}

void AttackAnimation::nextFrame()
{
	if(myAnim->getType() != group)
	{
		myAnim->setType(group);
		myAnim->onAnimationReset += [&](){ delete this; };
	}

	if(!soundPlayed)
	{
		playSound();
		soundPlayed = true;
	}
}

AttackAnimation::~AttackAnimation()
{
	myAnim->setType(ECreatureAnimType::HOLDING);
}

const CCreature * AttackAnimation::getCreature() const
{
	if (attackingStack->getCreature()->idNumber == CreatureID::ARROW_TOWERS)
		return owner.siegeController->getTurretCreature();
	else
		return attackingStack->getCreature();
}


AttackAnimation::AttackAnimation(BattleInterface & owner, const CStack *attacker, BattleHex _dest, const CStack *defender)
	: BattleStackAnimation(owner, attacker),
	  group(ECreatureAnimType::SHOOT_FRONT),
	  soundPlayed(false),
	  dest(_dest),
	  defendingStack(defender),
	  attackingStack(attacker)
{
	assert(attackingStack && "attackingStack is nullptr in CBattleAttack::CBattleAttack !\n");
	attackingStackPosBeforeReturn = attackingStack->getPosition();
}

bool HittedAnimation::init()
{
	CCS->soundh->playSound(battle_sound(stack->getCreature(), wince));
	myAnim->playOnce(ECreatureAnimType::HITTED);
	myAnim->onAnimationReset += [&](){ delete this; };
	return true;
}

HittedAnimation::HittedAnimation(BattleInterface & owner, const CStack * stack)
	: BattleStackAnimation(owner, stack)
{
}

DefenceAnimation::DefenceAnimation(BattleInterface & owner, const CStack * stack)
	: BattleStackAnimation(owner, stack)
{
}

bool DefenceAnimation::init()
{
	CCS->soundh->playSound(battle_sound(stack->getCreature(), defend));
	myAnim->playOnce(ECreatureAnimType::DEFENCE);
	myAnim->onAnimationReset += [&](){ delete this; };

	return true; //initialized successfuly
}

ECreatureAnimType::Type DeathAnimation::getMyAnimType()
{
	if(rangedAttack && myAnim->framesInGroup(ECreatureAnimType::DEATH_RANGED) > 0)
		return ECreatureAnimType::DEATH_RANGED;
	else
		return ECreatureAnimType::DEATH;
}

bool DeathAnimation::init()
{
	CCS->soundh->playSound(battle_sound(stack->getCreature(), killed));
	myAnim->playOnce(getMyAnimType());
	myAnim->onAnimationReset += [&](){ delete this; };
	return true;
}

DeathAnimation::DeathAnimation(BattleInterface & owner, const CStack * stack, bool ranged):
	BattleStackAnimation(owner, stack),
	rangedAttack(ranged)
{
}

DeathAnimation::~DeathAnimation()
{
	if(rangedAttack && myAnim->framesInGroup(ECreatureAnimType::DEAD_RANGED) > 0)
		myAnim->setType(ECreatureAnimType::DEAD_RANGED);
	else
		myAnim->setType(ECreatureAnimType::DEAD);
}

DummyAnimation::DummyAnimation(BattleInterface & owner, int howManyFrames)
	: BattleAnimation(owner),
	  counter(0),
	  howMany(howManyFrames)
{
	logAnim->debug("Created dummy animation for %d frames", howManyFrames);
}

bool DummyAnimation::init()
{
	return true;
}

void DummyAnimation::nextFrame()
{
	counter++;
	if(counter > howMany)
		delete this;
}

bool MeleeAttackAnimation::init()
{
	assert(attackingStack);
	assert(!myAnim->isDeadOrDying());

	if(!attackingStack || myAnim->isDeadOrDying())
	{
		delete this;
		return false;
	}

	logAnim->info("CMeleeAttackAnimation::init: stack %s -> stack %s", stack->getName(), defendingStack->getName());

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
		mutPos = BattleHex::mutualPosition(attackingStackPosBeforeReturn + revShiftattacker, defendingStack->getPosition());
	}
	if (mutPos == -1 && defendingStack->doubleWide())
	{
		mutPos = BattleHex::mutualPosition(attackingStackPosBeforeReturn, defendingStack->occupiedHex());
	}
	if (mutPos == -1 && defendingStack->doubleWide() && attackingStack->doubleWide())
	{
		mutPos = BattleHex::mutualPosition(attackingStackPosBeforeReturn + revShiftattacker, defendingStack->occupiedHex());
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

void MeleeAttackAnimation::nextFrame()
{
	size_t currentFrame = stackAnimation(attackingStack)->getCurrentFrame();
	size_t totalFrames = stackAnimation(attackingStack)->framesInGroup(group);

	if ( currentFrame * 2 >= totalFrames )
	{
		if(owner.getAnimationCondition(EAnimationEvents::HIT) == false)
			owner.setAnimationCondition(EAnimationEvents::HIT, true);
	}
	AttackAnimation::nextFrame();
}

void MeleeAttackAnimation::playSound()
{
	CCS->soundh->playSound(battle_sound(getCreature(), attack));
}

MeleeAttackAnimation::MeleeAttackAnimation(BattleInterface & owner, const CStack * attacker, BattleHex _dest, const CStack * _attacked)
	: AttackAnimation(owner, attacker, _dest, _attacked)
{
	logAnim->debug("Created melee attack anim for %s", attacker->getName());
}

StackMoveAnimation::StackMoveAnimation(BattleInterface & owner, const CStack * _stack, BattleHex _currentHex):
	BattleStackAnimation(owner, _stack),
	currentHex(_currentHex)
{

}

bool MovementAnimation::init()
{
	assert(stack);
	assert(!myAnim->isDeadOrDying());

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

	logAnim->info("CMovementAnimation::init: stack %s moves %d -> %d", stack->getName(), oldPos, currentHex);

	//reverse unit if necessary
	if(owner.stacksController->shouldRotate(stack, oldPos, currentHex))
	{
		// it seems that H3 does NOT plays full rotation animation during movement
		// Logical since it takes quite a lot of time
		rotateStack(oldPos);
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

void MovementAnimation::nextFrame()
{
	progress += float(GH.mainFPSmng->getElapsedMilliseconds()) / 1000 * timeToMove;

	//moving instructions
	myAnim->pos.x = static_cast<Sint16>(begX + distanceX * progress );
	myAnim->pos.y = static_cast<Sint16>(begY + distanceY * progress );

	BattleAnimation::nextFrame();

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

MovementAnimation::~MovementAnimation()
{
	assert(stack);

	myAnim->pos = owner.stacksController->getStackPositionAtHex(currentHex, stack);

	if(owner.moveSoundHander != -1)
	{
		CCS->soundh->stopSound(owner.moveSoundHander);
		owner.moveSoundHander = -1;
	}
}

MovementAnimation::MovementAnimation(BattleInterface & owner, const CStack *_stack, std::vector<BattleHex> _destTiles, int _distance)
	: StackMoveAnimation(owner, _stack, _destTiles.front()),
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

MovementEndAnimation::MovementEndAnimation(BattleInterface & owner, const CStack * _stack, BattleHex destTile)
: StackMoveAnimation(owner, _stack, destTile)
{
	logAnim->debug("Created movement end anim for %s", stack->getName());
}

bool MovementEndAnimation::init()
{
	assert(stack);
	assert(!myAnim->isDeadOrDying());

	if(!stack || myAnim->isDeadOrDying())
	{
		delete this;
		return false;
	}

	logAnim->info("CMovementEndAnimation::init: stack %s", stack->getName());

	CCS->soundh->playSound(battle_sound(stack->getCreature(), endMoving));

	if(!myAnim->framesInGroup(ECreatureAnimType::MOVE_END))
	{
		delete this;
		return false;
	}


	myAnim->setType(ECreatureAnimType::MOVE_END);
	myAnim->onAnimationReset += [&](){ delete this; };

	return true;
}

MovementEndAnimation::~MovementEndAnimation()
{
	if(myAnim->getType() != ECreatureAnimType::DEAD)
		myAnim->setType(ECreatureAnimType::HOLDING); //resetting to default

	CCS->curh->show();
}

MovementStartAnimation::MovementStartAnimation(BattleInterface & owner, const CStack * _stack)
	: StackMoveAnimation(owner, _stack, _stack->getPosition())
{
	logAnim->debug("Created movement start anim for %s", stack->getName());
}

bool MovementStartAnimation::init()
{
	assert(stack);
	assert(!myAnim->isDeadOrDying());

	if(!stack || myAnim->isDeadOrDying())
	{
		delete this;
		return false;
	}

	logAnim->info("CMovementStartAnimation::init: stack %s", stack->getName());
	CCS->soundh->playSound(battle_sound(stack->getCreature(), startMoving));

	if(!myAnim->framesInGroup(ECreatureAnimType::MOVE_START))
	{
		delete this;
		return false;
	}

	myAnim->setType(ECreatureAnimType::MOVE_START);
	myAnim->onAnimationReset += [&](){ delete this; };
	return true;
}

ReverseAnimation::ReverseAnimation(BattleInterface & owner, const CStack * stack, BattleHex dest)
	: StackMoveAnimation(owner, stack, dest)
{
	logAnim->debug("Created reverse anim for %s", stack->getName());
}

bool ReverseAnimation::init()
{
	assert(myAnim);
	assert(!myAnim->isDeadOrDying());

	if(myAnim == nullptr || myAnim->isDeadOrDying())
	{
		delete this;
		return false; //there is no such creature
	}

	logAnim->info("CReverseAnimation::init: stack %s", stack->getName());
	if(myAnim->framesInGroup(ECreatureAnimType::TURN_L))
	{
		myAnim->playOnce(ECreatureAnimType::TURN_L);
		myAnim->onAnimationReset += std::bind(&ReverseAnimation::setupSecondPart, this);
	}
	else
	{
		setupSecondPart();
	}
	return true;
}

void BattleStackAnimation::rotateStack(BattleHex hex)
{
	setStackFacingRight(stack, !stackFacingRight(stack));

	stackAnimation(stack)->pos = owner.stacksController->getStackPositionAtHex(hex, stack);
}

void ReverseAnimation::setupSecondPart()
{
	assert(stack);

	if(!stack)
	{
		delete this;
		return;
	}

	rotateStack(currentHex);

	if(myAnim->framesInGroup(ECreatureAnimType::TURN_R))
	{
		myAnim->playOnce(ECreatureAnimType::TURN_R);
		myAnim->onAnimationReset += [&](){ delete this; };
	}
	else
		delete this;
}

bool ResurrectionAnimation::init()
{
	assert(stack);

	if(!stack)
	{
		delete this;
		return false;
	}

	logAnim->info("CResurrectionAnimation::init: stack %s", stack->getName());
	myAnim->playOnce(ECreatureAnimType::RESURRECTION);
	myAnim->onAnimationReset += [&](){ delete this; };

	return true;
}

ResurrectionAnimation::ResurrectionAnimation(BattleInterface & owner, const CStack * _stack):
	BattleStackAnimation(owner, _stack)
{

}

bool FadingAnimation::init()
{
	logAnim->info("FadingAnimation::init: stack %s", stack->getName());
	//TODO: pause animation?
	return true;
}

void FadingAnimation::nextFrame()
{
	float elapsed  = GH.mainFPSmng->getElapsedMilliseconds() / 1000.f;
	float fullTime = AnimationControls::getFadeInDuration();
	float delta    = elapsed / fullTime;
	progress += delta;

	if (progress > 1.0f)
		progress = 1.0f;

	uint8_t blueFactor = stack->cloned ? 0 : 255;
	uint8_t blueAdded  = stack->cloned ? 255 : 0;
	uint8_t alpha = CSDL_Ext::lerp(from, dest, progress);

	ColorShifterMultiplyAndAdd shifterFade ({255, 255, blueFactor, alpha}, {0, 0, blueAdded, 0});
	stackAnimation(stack)->shiftColor(&shifterFade);

	if (progress == 1.0f)
		delete this;
}

FadingAnimation::FadingAnimation(BattleInterface & owner, const CStack * _stack, uint8_t alphaFrom, uint8_t alphaDest):
	BattleStackAnimation(owner, _stack),
	from(alphaFrom),
	dest(alphaDest)
{
}


RangedAttackAnimation::RangedAttackAnimation(BattleInterface & owner_, const CStack * attacker, BattleHex dest_, const CStack * defender)
	: AttackAnimation(owner_, attacker, dest_, defender),
	  projectileEmitted(false)
{
}

void RangedAttackAnimation::playSound()
{
	CCS->soundh->playSound(battle_sound(getCreature(), shoot));
}

bool RangedAttackAnimation::init()
{
	assert(attackingStack);
	assert(!myAnim->isDeadOrDying());

	if(!attackingStack || myAnim->isDeadOrDying())
	{
		//FIXME: how is this possible?
		logAnim->warn("Shooting animation has not started yet but attacker is dead! Aborting...");
		delete this;
		return false;
	}

	logAnim->info("CRangedAttackAnimation::init: stack %s", stack->getName());

	setAnimationGroup();
	initializeProjectile();
	return true;
}

void RangedAttackAnimation::setAnimationGroup()
{
	Point shooterPos = stackAnimation(attackingStack)->pos.topLeft();
	Point shotTarget = owner.stacksController->getStackPositionAtHex(dest, defendingStack);

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

void RangedAttackAnimation::initializeProjectile()
{
	const CCreature *shooterInfo = getCreature();
	Point shotTarget = owner.stacksController->getStackPositionAtHex(dest, defendingStack) + Point(225, 225);
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

void RangedAttackAnimation::emitProjectile()
{
	logAnim->info("Ranged attack projectile emitted");
	owner.projectilesController->emitStackProjectile(attackingStack);
	projectileEmitted = true;
}

void RangedAttackAnimation::nextFrame()
{
	// animation should be paused if there is an active projectile
	if (projectileEmitted)
	{
		if (owner.projectilesController->hasActiveProjectile(attackingStack))
			stackAnimation(attackingStack)->pause();
		else
		{
			stackAnimation(attackingStack)->play();
			if(owner.getAnimationCondition(EAnimationEvents::HIT) == false)
				owner.setAnimationCondition(EAnimationEvents::HIT, true);
		}
	}

	AttackAnimation::nextFrame();

	if (!projectileEmitted)
	{
		// emit projectile once animation playback reached "climax" frame
		if ( stackAnimation(attackingStack)->getCurrentFrame() >= getAttackClimaxFrame() )
		{
			emitProjectile();
			stackAnimation(attackingStack)->pause();
			return;
		}
	}
}

RangedAttackAnimation::~RangedAttackAnimation()
{
	assert(!owner.projectilesController->hasActiveProjectile(attackingStack));
	assert(projectileEmitted);

	// FIXME: is this possible? Animation is over but we're yet to fire projectile?
	if (!projectileEmitted)
	{
		logAnim->warn("Shooting animation has finished but projectile was not emitted! Emitting it now...");
		emitProjectile();
	}
}

ShootingAnimation::ShootingAnimation(BattleInterface & owner, const CStack * attacker, BattleHex _dest, const CStack * _attacked)
	: RangedAttackAnimation(owner, attacker, _dest, _attacked)
{
	logAnim->debug("Created shooting anim for %s", stack->getName());
}

void ShootingAnimation::createProjectile(const Point & from, const Point & dest) const
{
	owner.projectilesController->createProjectile(attackingStack, from, dest);
}

uint32_t ShootingAnimation::getAttackClimaxFrame() const
{
	const CCreature *shooterInfo = getCreature();
	return shooterInfo->animation.attackClimaxFrame;
}

ECreatureAnimType::Type ShootingAnimation::getUpwardsGroup() const
{
	return ECreatureAnimType::SHOOT_UP;
}

ECreatureAnimType::Type ShootingAnimation::getForwardGroup() const
{
	return ECreatureAnimType::SHOOT_FRONT;
}

ECreatureAnimType::Type ShootingAnimation::getDownwardsGroup() const
{
	return ECreatureAnimType::SHOOT_DOWN;
}

CatapultAnimation::CatapultAnimation(BattleInterface & owner, const CStack * attacker, BattleHex _dest, const CStack * _attacked, int _catapultDmg)
	: ShootingAnimation(owner, attacker, _dest, _attacked),
	catapultDamage(_catapultDmg),
	explosionEmitted(false)
{
	logAnim->debug("Created shooting anim for %s", stack->getName());
}

void CatapultAnimation::nextFrame()
{
	ShootingAnimation::nextFrame();

	if ( explosionEmitted)
		return;

	if ( !projectileEmitted)
		return;

	if (owner.projectilesController->hasActiveProjectile(attackingStack))
		return;

	explosionEmitted = true;
	Point shotTarget = owner.stacksController->getStackPositionAtHex(dest, defendingStack) + Point(225, 225) - Point(126, 105);

	if(catapultDamage > 0)
		owner.stacksController->addNewAnim( new PointEffectAnimation(owner, "WALLHIT", "SGEXPL.DEF", shotTarget));
	else
		owner.stacksController->addNewAnim( new PointEffectAnimation(owner, "WALLMISS", "CSGRCK.DEF", shotTarget));
}

void CatapultAnimation::createProjectile(const Point & from, const Point & dest) const
{
	owner.projectilesController->createCatapultProjectile(attackingStack, from, dest);
}

CastAnimation::CastAnimation(BattleInterface & owner_, const CStack * attacker, BattleHex dest_, const CStack * defender, const CSpell * spell)
	: RangedAttackAnimation(owner_, attacker, dest_, defender),
	  spell(spell)
{
	assert(dest.isValid());// FIXME: when?

	if(!dest_.isValid() && defender)
		dest = defender->getPosition();
}

ECreatureAnimType::Type CastAnimation::findValidGroup( const std::vector<ECreatureAnimType::Type> candidates ) const
{
	for ( auto group : candidates)
	{
		if(myAnim->framesInGroup(group) > 0)
			return group;
	}

	assert(0);
	return ECreatureAnimType::HOLDING;
}

ECreatureAnimType::Type CastAnimation::getUpwardsGroup() const
{
	return findValidGroup({
		ECreatureAnimType::VCMI_CAST_UP,
		ECreatureAnimType::CAST_UP,
		ECreatureAnimType::SHOOT_UP,
		ECreatureAnimType::ATTACK_UP
	});
}

ECreatureAnimType::Type CastAnimation::getForwardGroup() const
{
	return findValidGroup({
		ECreatureAnimType::VCMI_CAST_FRONT,
		ECreatureAnimType::CAST_FRONT,
		ECreatureAnimType::SHOOT_FRONT,
		ECreatureAnimType::ATTACK_FRONT
	});
}

ECreatureAnimType::Type CastAnimation::getDownwardsGroup() const
{
	return findValidGroup({
		ECreatureAnimType::VCMI_CAST_DOWN,
		ECreatureAnimType::CAST_DOWN,
		ECreatureAnimType::SHOOT_DOWN,
		ECreatureAnimType::ATTACK_DOWN
	});
}

void CastAnimation::createProjectile(const Point & from, const Point & dest) const
{
	if (!spell->animationInfo.projectile.empty())
		owner.projectilesController->createSpellProjectile(attackingStack, from, dest, spell);
}

uint32_t CastAnimation::getAttackClimaxFrame() const
{
	//TODO: allow defining this parameter in config file, separately from attackClimaxFrame of missile attacks
	uint32_t maxFrames = stackAnimation(attackingStack)->framesInGroup(group);

	if (maxFrames > 2)
		return maxFrames - 2;
	return 0;
}

PointEffectAnimation::PointEffectAnimation(BattleInterface & owner, std::string soundName, std::string animationName, int effects):
	BattleAnimation(owner),
	animation(std::make_shared<CAnimation>(animationName)),
	soundName(soundName),
	effectFlags(effects),
	soundPlayed(false),
	soundFinished(false),
	effectFinished(false)
{
	logAnim->info("CPointEffectAnimation::init: effect %s", animationName);
}

PointEffectAnimation::PointEffectAnimation(BattleInterface & owner, std::string soundName, std::string animationName, std::vector<BattleHex> hex, int effects):
	PointEffectAnimation(owner, soundName, animationName, effects)
{
	battlehexes = hex;
}

PointEffectAnimation::PointEffectAnimation(BattleInterface & owner, std::string soundName, std::string animationName, BattleHex hex, int effects):
	PointEffectAnimation(owner, soundName, animationName, effects)
{
	assert(hex.isValid());
	battlehexes.push_back(hex);
}

PointEffectAnimation::PointEffectAnimation(BattleInterface & owner, std::string soundName, std::string animationName, std::vector<Point> pos, int effects):
	PointEffectAnimation(owner, soundName, animationName, effects)
{
	positions = pos;
}

PointEffectAnimation::PointEffectAnimation(BattleInterface & owner, std::string soundName, std::string animationName, Point pos, int effects):
	PointEffectAnimation(owner, soundName, animationName, effects)
{
	positions.push_back(pos);
}

PointEffectAnimation::PointEffectAnimation(BattleInterface & owner, std::string soundName, std::string animationName, Point pos, BattleHex hex,   int effects):
	PointEffectAnimation(owner, soundName, animationName, effects)
{
	assert(hex.isValid());
	battlehexes.push_back(hex);
	positions.push_back(pos);
}

bool PointEffectAnimation::init()
{
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

void PointEffectAnimation::nextFrame()
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

bool PointEffectAnimation::alignToBottom() const
{
	return effectFlags & ALIGN_TO_BOTTOM;
}

bool PointEffectAnimation::waitForSound() const
{
	return effectFlags & WAIT_FOR_SOUND;
}

bool PointEffectAnimation::forceOnTop() const
{
	return effectFlags & FORCE_ON_TOP;
}

bool PointEffectAnimation::screenFill() const
{
	return effectFlags & SCREEN_FILL;
}

void PointEffectAnimation::onEffectFinished()
{
	effectFinished = true;
}

void PointEffectAnimation::onSoundFinished()
{
	soundFinished = true;
}

void PointEffectAnimation::playSound()
{
	if (soundPlayed)
		return;

	soundPlayed = true;
	if (soundName.empty())
	{
		onSoundFinished();
		return;
	}

	int channel = CCS->soundh->playSound(soundName);

	if (!waitForSound() || channel == -1)
		onSoundFinished();
	else
		CCS->soundh->setCallback(channel, [&](){ onSoundFinished(); });
}

void PointEffectAnimation::playEffect()
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

void PointEffectAnimation::clearEffect()
{
	auto & effects = owner.effectsController->battleEffects;

	vstd::erase_if(effects, [&](const BattleEffect & effect){
		return effect.effectID == ID;
	});
}

PointEffectAnimation::~PointEffectAnimation()
{
	assert(effectFinished);
	assert(soundFinished);
}

HeroCastAnimation::HeroCastAnimation(BattleInterface & owner, std::shared_ptr<BattleHero> hero, BattleHex dest, const CStack * defender, const CSpell * spell):
	BattleAnimation(owner),
	projectileEmitted(false),
	hero(hero),
	target(defender),
	tile(dest),
	spell(spell)
{
}

bool HeroCastAnimation::init()
{
	hero->setPhase(EHeroAnimType::CAST_SPELL);

	hero->onPhaseFinished([&](){
		assert(owner.getAnimationCondition(EAnimationEvents::HIT) == true);
		delete this;
	});

	initializeProjectile();

	return true;
}

void HeroCastAnimation::initializeProjectile()
{
	//spell has no projectile to play, ignore this step
	if (spell->animationInfo.projectile.empty())
		return;

	Point srccoord = hero->pos.center();
	Point destcoord = owner.stacksController->getStackPositionAtHex(tile, target); //position attacked by projectile

	destcoord += Point(222, 265); // FIXME: what are these constants?
	owner.projectilesController->createSpellProjectile( nullptr, srccoord, destcoord, spell);
}

void HeroCastAnimation::emitProjectile()
{
	if (projectileEmitted)
		return;

	//spell has no projectile to play, skip this step and immediately play hit animations
	if (spell->animationInfo.projectile.empty())
		emitAnimationEvent();
	else
		owner.projectilesController->emitStackProjectile( nullptr );

	projectileEmitted = true;
}

void HeroCastAnimation::emitAnimationEvent()
{
	if(owner.getAnimationCondition(EAnimationEvents::HIT) == false)
		owner.setAnimationCondition(EAnimationEvents::HIT, true);
}

void HeroCastAnimation::nextFrame()
{
	float frame = hero->getFrame();

	if (frame < 4.0f) // middle point of animation //TODO: un-hardcode
		return;

	if (!projectileEmitted)
	{
		emitProjectile();
		hero->pause();
		return;
	}

	if (!owner.projectilesController->hasActiveProjectile(nullptr))
	{
		emitAnimationEvent();
		//TODO: check H3 - it is possible that hero animation should be paused until hit effect is over, not just projectile
		hero->play();
	}
}
