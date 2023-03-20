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
#include "../gui/CursorHandler.h"
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

StackActionAnimation::StackActionAnimation(BattleInterface & owner, const CStack * stack)
	: BattleStackAnimation(owner, stack)
	, nextGroup(ECreatureAnimType::HOLDING)
	, currGroup(ECreatureAnimType::HOLDING)
{
}

ECreatureAnimType StackActionAnimation::getGroup() const
{
	return currGroup;
}

void StackActionAnimation::setNextGroup( ECreatureAnimType group )
{
	nextGroup = group;
}

void StackActionAnimation::setGroup( ECreatureAnimType group )
{
	currGroup = group;
}

void StackActionAnimation::setSound( std::string sound )
{
	this->sound = sound;
}

bool StackActionAnimation::init()
{
	if (!sound.empty())
		CCS->soundh->playSound(sound);

	if (myAnim->framesInGroup(currGroup) > 0)
	{
		myAnim->playOnce(currGroup);
		myAnim->onAnimationReset += [&](){ delete this; };
	}
	else
		delete this;

	return true;
}

StackActionAnimation::~StackActionAnimation()
{
	if (stack->isFrozen())
		myAnim->setType(ECreatureAnimType::HOLDING);
	else
		myAnim->setType(nextGroup);

}

ECreatureAnimType AttackAnimation::findValidGroup( const std::vector<ECreatureAnimType> candidates ) const
{
	for ( auto group : candidates)
	{
		if(myAnim->framesInGroup(group) > 0)
			return group;
	}

	assert(0);
	return ECreatureAnimType::HOLDING;
}

const CCreature * AttackAnimation::getCreature() const
{
	if (attackingStack->getCreature()->idNumber == CreatureID::ARROW_TOWERS)
		return owner.siegeController->getTurretCreature();
	else
		return attackingStack->getCreature();
}


AttackAnimation::AttackAnimation(BattleInterface & owner, const CStack *attacker, BattleHex _dest, const CStack *defender)
	: StackActionAnimation(owner, attacker),
	  dest(_dest),
	  defendingStack(defender),
	  attackingStack(attacker)
{
	assert(attackingStack && "attackingStack is nullptr in CBattleAttack::CBattleAttack !\n");
	attackingStackPosBeforeReturn = attackingStack->getPosition();
}

HittedAnimation::HittedAnimation(BattleInterface & owner, const CStack * stack)
	: StackActionAnimation(owner, stack)
{
	setGroup(ECreatureAnimType::HITTED);
	setSound(battle_sound(stack->getCreature(), wince));
	logAnim->debug("Created HittedAnimation for %s", stack->getName());
}

DefenceAnimation::DefenceAnimation(BattleInterface & owner, const CStack * stack)
	: StackActionAnimation(owner, stack)
{
	setGroup(ECreatureAnimType::DEFENCE);
	setSound(battle_sound(stack->getCreature(), defend));
	logAnim->debug("Created DefenceAnimation for %s", stack->getName());
}

DeathAnimation::DeathAnimation(BattleInterface & owner, const CStack * stack, bool ranged):
	StackActionAnimation(owner, stack)
{
	setSound(battle_sound(stack->getCreature(), killed));

	if(ranged && myAnim->framesInGroup(ECreatureAnimType::DEATH_RANGED) > 0)
		setGroup(ECreatureAnimType::DEATH_RANGED);
	else
		setGroup(ECreatureAnimType::DEATH);

	if(ranged && myAnim->framesInGroup(ECreatureAnimType::DEAD_RANGED) > 0)
		setNextGroup(ECreatureAnimType::DEAD_RANGED);
	else
		setNextGroup(ECreatureAnimType::DEAD);

	logAnim->debug("Created DeathAnimation for %s", stack->getName());
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

ECreatureAnimType MeleeAttackAnimation::getUpwardsGroup(bool multiAttack) const
{
	if (!multiAttack)
		return ECreatureAnimType::ATTACK_UP;

	return findValidGroup({
		ECreatureAnimType::GROUP_ATTACK_UP,
		ECreatureAnimType::SPECIAL_UP,
		ECreatureAnimType::SPECIAL_FRONT, // weird, but required for H3
		ECreatureAnimType::ATTACK_UP
	});
}

ECreatureAnimType MeleeAttackAnimation::getForwardGroup(bool multiAttack) const
{
	if (!multiAttack)
		return ECreatureAnimType::ATTACK_FRONT;

	return findValidGroup({
		ECreatureAnimType::GROUP_ATTACK_FRONT,
		ECreatureAnimType::SPECIAL_FRONT,
		ECreatureAnimType::ATTACK_FRONT
	});
}

ECreatureAnimType MeleeAttackAnimation::getDownwardsGroup(bool multiAttack) const
{
	if (!multiAttack)
		return ECreatureAnimType::ATTACK_DOWN;

	return findValidGroup({
		ECreatureAnimType::GROUP_ATTACK_DOWN,
		ECreatureAnimType::SPECIAL_DOWN,
		ECreatureAnimType::SPECIAL_FRONT, // weird, but required for H3
		ECreatureAnimType::ATTACK_DOWN
	});
}

ECreatureAnimType MeleeAttackAnimation::selectGroup(bool multiAttack)
{
	const ECreatureAnimType mutPosToGroup[] =
	{
		getUpwardsGroup  (multiAttack),
		getUpwardsGroup  (multiAttack),
		getForwardGroup  (multiAttack),
		getDownwardsGroup(multiAttack),
		getDownwardsGroup(multiAttack),
		getForwardGroup  (multiAttack)
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

	assert(mutPos >= 0 && mutPos <=5);

	return mutPosToGroup[mutPos];
}

void MeleeAttackAnimation::nextFrame()
{
	size_t currentFrame = stackAnimation(attackingStack)->getCurrentFrame();
	size_t totalFrames = stackAnimation(attackingStack)->framesInGroup(getGroup());

	if ( currentFrame * 2 >= totalFrames )
		owner.executeAnimationStage(EAnimationEvents::HIT);

	AttackAnimation::nextFrame();
}

MeleeAttackAnimation::MeleeAttackAnimation(BattleInterface & owner, const CStack * attacker, BattleHex _dest, const CStack * _attacked, bool multiAttack)
	: AttackAnimation(owner, attacker, _dest, _attacked)
{
	logAnim->debug("Created MeleeAttackAnimation for %s", attacker->getName());
	setSound(battle_sound(getCreature(), attack));
	setGroup(selectGroup(multiAttack));
}

StackMoveAnimation::StackMoveAnimation(BattleInterface & owner, const CStack * _stack, BattleHex prevHex, BattleHex nextHex):
	BattleStackAnimation(owner, _stack),
	prevHex(prevHex),
	nextHex(nextHex)
{
}

bool MovementAnimation::init()
{
	assert(stack);
	assert(!myAnim->isDeadOrDying());
	assert(stackAnimation(stack)->framesInGroup(ECreatureAnimType::MOVING) > 0);

	if(stackAnimation(stack)->framesInGroup(ECreatureAnimType::MOVING) == 0)
	{
		//no movement, end immediately
		delete this;
		return false;
	}

	logAnim->debug("CMovementAnimation::init: stack %s moves %d -> %d", stack->getName(), prevHex, nextHex);

	//reverse unit if necessary
	if(owner.stacksController->shouldRotate(stack, prevHex, nextHex))
	{
		// it seems that H3 does NOT plays full rotation animation during movement
		// Logical since it takes quite a lot of time
		rotateStack(prevHex);
	}

	if(myAnim->getType() != ECreatureAnimType::MOVING)
	{
		myAnim->setType(ECreatureAnimType::MOVING);
	}

	if (moveSoundHander == -1)
	{
		moveSoundHander = CCS->soundh->playSound(battle_sound(stack->getCreature(), move), -1);
	}

	Point begPosition = owner.stacksController->getStackPositionAtHex(prevHex, stack);
	Point endPosition = owner.stacksController->getStackPositionAtHex(nextHex, stack);

	progressPerSecond = AnimationControls::getMovementDistance(stack->getCreature());

	begX = begPosition.x;
	begY = begPosition.y;
	//progress = 0;
	distanceX = endPosition.x - begPosition.x;
	distanceY = endPosition.y - begPosition.y;

	if (stack->hasBonus(Selector::type()(Bonus::FLYING)))
	{
		float distance = static_cast<float>(sqrt(distanceX * distanceX + distanceY * distanceY));
		progressPerSecond =  AnimationControls::getFlightDistance(stack->getCreature()) / distance;
	}

	return true;
}

void MovementAnimation::nextFrame()
{
	progress += float(GH.mainFPSmng->getElapsedMilliseconds()) / 1000 * progressPerSecond;

	//moving instructions
	myAnim->pos.x = static_cast<Sint16>(begX + distanceX * progress );
	myAnim->pos.y = static_cast<Sint16>(begY + distanceY * progress );

	BattleAnimation::nextFrame();

	if(progress >= 1.0)
	{
		progress -= 1.0;
		// Sets the position of the creature animation sprites
		Point coords = owner.stacksController->getStackPositionAtHex(nextHex, stack);
		myAnim->pos.moveTo(coords);

		// true if creature haven't reached the final destination hex
		if ((curentMoveIndex + 1) < destTiles.size())
		{
			// update the next hex field which has to be reached by the stack
			curentMoveIndex++;
			prevHex = nextHex;
			nextHex = destTiles[curentMoveIndex];

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

	if(moveSoundHander != -1)
		CCS->soundh->stopSound(moveSoundHander);
}

MovementAnimation::MovementAnimation(BattleInterface & owner, const CStack *stack, std::vector<BattleHex> _destTiles, int _distance)
	: StackMoveAnimation(owner, stack, stack->getPosition(), _destTiles.front()),
	  destTiles(_destTiles),
	  curentMoveIndex(0),
	  begX(0), begY(0),
	  distanceX(0), distanceY(0),
	  progressPerSecond(0.0),
	  moveSoundHander(-1),
	  progress(0.0)
{
	logAnim->debug("Created MovementAnimation for %s", stack->getName());
}

MovementEndAnimation::MovementEndAnimation(BattleInterface & owner, const CStack * _stack, BattleHex destTile)
: StackMoveAnimation(owner, _stack, destTile, destTile)
{
	logAnim->debug("Created MovementEndAnimation for %s", stack->getName());
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

	logAnim->debug("CMovementEndAnimation::init: stack %s", stack->getName());
	myAnim->pos.moveTo(owner.stacksController->getStackPositionAtHex(nextHex, stack));

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
	: StackMoveAnimation(owner, _stack, _stack->getPosition(), _stack->getPosition())
{
	logAnim->debug("Created MovementStartAnimation for %s", stack->getName());
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

	logAnim->debug("CMovementStartAnimation::init: stack %s", stack->getName());
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
	: StackMoveAnimation(owner, stack, dest, dest)
{
	logAnim->debug("Created ReverseAnimation for %s", stack->getName());
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

	logAnim->debug("CReverseAnimation::init: stack %s", stack->getName());
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

	stackAnimation(stack)->pos.moveTo(owner.stacksController->getStackPositionAtHex(hex, stack));
}

void ReverseAnimation::setupSecondPart()
{
	assert(stack);

	if(!stack)
	{
		delete this;
		return;
	}

	rotateStack(nextHex);

	if(myAnim->framesInGroup(ECreatureAnimType::TURN_R))
	{
		myAnim->playOnce(ECreatureAnimType::TURN_R);
		myAnim->onAnimationReset += [&](){ delete this; };
	}
	else
		delete this;
}

ResurrectionAnimation::ResurrectionAnimation(BattleInterface & owner, const CStack * _stack):
	StackActionAnimation(owner, _stack)
{
	setGroup(ECreatureAnimType::RESURRECTION);
	logAnim->debug("Created ResurrectionAnimation for %s", stack->getName());
}

bool ColorTransformAnimation::init()
{
	return true;
}

void ColorTransformAnimation::nextFrame()
{
	float elapsed  = GH.mainFPSmng->getElapsedMilliseconds() / 1000.f;
	float fullTime = AnimationControls::getFadeInDuration();
	float delta    = elapsed / fullTime;
	totalProgress += delta;

	size_t index = 0;

	while (index < timePoints.size() && timePoints[index] < totalProgress )
		++index;

	if (index == timePoints.size())
	{
		//end of animation. Apply ColorShifter using final values and die
		const auto & shifter = steps[index - 1];
		owner.stacksController->setStackColorFilter(shifter, stack, spell, false);
		delete this;
		return;
	}

	assert(index != 0);

	const auto & prevShifter = steps[index - 1];
	const auto & nextShifter = steps[index];

	float prevPoint = timePoints[index-1];
	float nextPoint = timePoints[index];
	float localProgress = totalProgress - prevPoint;
	float stepDuration = (nextPoint - prevPoint);
	float factor = localProgress / stepDuration;

	auto shifter = ColorFilter::genInterpolated(prevShifter, nextShifter, factor);

	owner.stacksController->setStackColorFilter(shifter, stack, spell, true);
}

ColorTransformAnimation::ColorTransformAnimation(BattleInterface & owner, const CStack * _stack, const std::string & colorFilterName, const CSpell * spell):
	BattleStackAnimation(owner, _stack),
	spell(spell),
	totalProgress(0.f)
{
	auto effect = owner.effectsController->getMuxerEffect(colorFilterName);
	steps = effect.filters;
	timePoints = effect.timePoints;

	assert(!steps.empty() && steps.size() == timePoints.size());

	logAnim->debug("Created ColorTransformAnimation for %s", stack->getName());
}

RangedAttackAnimation::RangedAttackAnimation(BattleInterface & owner_, const CStack * attacker, BattleHex dest_, const CStack * defender)
	: AttackAnimation(owner_, attacker, dest_, defender),
	  projectileEmitted(false)
{
	setSound(battle_sound(getCreature(), shoot));
}

bool RangedAttackAnimation::init()
{
	setAnimationGroup();
	initializeProjectile();

	return AttackAnimation::init();
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
		setGroup(getUpwardsGroup());
	else if (projectileAngle < -straightAngle)
		setGroup(getDownwardsGroup());
	else
		setGroup(getForwardGroup());
}

void RangedAttackAnimation::initializeProjectile()
{
	const CCreature *shooterInfo = getCreature();
	Point shotTarget = owner.stacksController->getStackPositionAtHex(dest, defendingStack) + Point(225, 225);
	Point shotOrigin = stackAnimation(attackingStack)->pos.topLeft() + Point(222, 265);
	int multiplier = stackFacingRight(attackingStack) ? 1 : -1;

	if (getGroup() == getUpwardsGroup())
	{
		shotOrigin.x += ( -25 + shooterInfo->animation.upperRightMissleOffsetX ) * multiplier;
		shotOrigin.y += shooterInfo->animation.upperRightMissleOffsetY;
	}
	else if (getGroup() == getDownwardsGroup())
	{
		shotOrigin.x += ( -25 + shooterInfo->animation.lowerRightMissleOffsetX ) * multiplier;
		shotOrigin.y += shooterInfo->animation.lowerRightMissleOffsetY;
	}
	else if (getGroup() == getForwardGroup())
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
	logAnim->debug("Ranged attack projectile emitted");
	owner.projectilesController->emitStackProjectile(attackingStack);
	projectileEmitted = true;
}

void RangedAttackAnimation::nextFrame()
{
	// animation should be paused if there is an active projectile
	if (projectileEmitted)
	{
		if (!owner.projectilesController->hasActiveProjectile(attackingStack, false))
			owner.executeAnimationStage(EAnimationEvents::HIT);

	}

	bool stackHasProjectile = owner.projectilesController->hasActiveProjectile(stack, true);

	if (!projectileEmitted || stackHasProjectile)
		stackAnimation(attackingStack)->playUntil(getAttackClimaxFrame());
	else
		stackAnimation(attackingStack)->playUntil(static_cast<size_t>(-1));

	AttackAnimation::nextFrame();

	if (!projectileEmitted)
	{
		// emit projectile once animation playback reached "climax" frame
		if ( stackAnimation(attackingStack)->getCurrentFrame() >= getAttackClimaxFrame() )
		{
			emitProjectile();
			return;
		}
	}
}

RangedAttackAnimation::~RangedAttackAnimation()
{
	assert(!owner.projectilesController->hasActiveProjectile(attackingStack, false));
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
	logAnim->debug("Created ShootingAnimation for %s", stack->getName());
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

ECreatureAnimType ShootingAnimation::getUpwardsGroup() const
{
	return ECreatureAnimType::SHOOT_UP;
}

ECreatureAnimType ShootingAnimation::getForwardGroup() const
{
	return ECreatureAnimType::SHOOT_FRONT;
}

ECreatureAnimType ShootingAnimation::getDownwardsGroup() const
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

	if (owner.projectilesController->hasActiveProjectile(attackingStack, false))
		return;

	explosionEmitted = true;
	Point shotTarget = owner.stacksController->getStackPositionAtHex(dest, defendingStack) + Point(225, 225) - Point(126, 105);

	std::string soundFilename  = (catapultDamage > 0) ? "WALLHIT" : "WALLMISS";
	std::string effectFilename = (catapultDamage > 0) ? "SGEXPL" : "CSGRCK";

	CCS->soundh->playSound( soundFilename );
	owner.stacksController->addNewAnim( new EffectAnimation(owner, effectFilename, shotTarget));
}

void CatapultAnimation::createProjectile(const Point & from, const Point & dest) const
{
	owner.projectilesController->createCatapultProjectile(attackingStack, from, dest);
}

CastAnimation::CastAnimation(BattleInterface & owner_, const CStack * attacker, BattleHex dest, const CStack * defender, const CSpell * spell)
	: RangedAttackAnimation(owner_, attacker, dest, defender),
	  spell(spell)
{
	if(!dest.isValid())
	{
		assert(spell->animationInfo.projectile.empty());

		if (defender)
			dest = defender->getPosition();
		else
			dest = attacker->getPosition();
	}
}

ECreatureAnimType CastAnimation::getUpwardsGroup() const
{
	return findValidGroup({
		ECreatureAnimType::CAST_UP,
		ECreatureAnimType::SPECIAL_UP,
		ECreatureAnimType::SPECIAL_FRONT, // weird, but required for H3
		ECreatureAnimType::SHOOT_UP,
		ECreatureAnimType::ATTACK_UP
	});
}

ECreatureAnimType CastAnimation::getForwardGroup() const
{
	return findValidGroup({
		ECreatureAnimType::CAST_FRONT,
		ECreatureAnimType::SPECIAL_FRONT,
		ECreatureAnimType::SHOOT_FRONT,
		ECreatureAnimType::ATTACK_FRONT
	});
}

ECreatureAnimType CastAnimation::getDownwardsGroup() const
{
	return findValidGroup({
		ECreatureAnimType::CAST_DOWN,
		ECreatureAnimType::SPECIAL_DOWN,
		ECreatureAnimType::SPECIAL_FRONT, // weird, but required for H3
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
	uint32_t maxFrames = stackAnimation(attackingStack)->framesInGroup(getGroup());

	return maxFrames / 2;
}

EffectAnimation::EffectAnimation(BattleInterface & owner, std::string animationName, int effects):
	BattleAnimation(owner),
	animation(std::make_shared<CAnimation>(animationName)),
	effectFlags(effects),
	effectFinished(false)
{
	logAnim->debug("CPointEffectAnimation::init: effect %s", animationName);
}

EffectAnimation::EffectAnimation(BattleInterface & owner, std::string animationName, std::vector<BattleHex> hex, int effects):
	EffectAnimation(owner, animationName, effects)
{
	battlehexes = hex;
}

EffectAnimation::EffectAnimation(BattleInterface & owner, std::string animationName, BattleHex hex, int effects):
	EffectAnimation(owner, animationName, effects)
{
	assert(hex.isValid());
	battlehexes.push_back(hex);
}

EffectAnimation::EffectAnimation(BattleInterface & owner, std::string animationName, std::vector<Point> pos, int effects):
	EffectAnimation(owner, animationName, effects)
{
	positions = pos;
}

EffectAnimation::EffectAnimation(BattleInterface & owner, std::string animationName, Point pos, int effects):
	EffectAnimation(owner, animationName, effects)
{
	positions.push_back(pos);
}

EffectAnimation::EffectAnimation(BattleInterface & owner, std::string animationName, Point pos, BattleHex hex,   int effects):
	EffectAnimation(owner, animationName, effects)
{
	assert(hex.isValid());
	battlehexes.push_back(hex);
	positions.push_back(pos);
}

bool EffectAnimation::init()
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
		for(int i=0; i * first->width() < owner.fieldController->pos.w ; ++i)
			for(int j=0; j * first->height() < owner.fieldController->pos.h ; ++j)
				positions.push_back(Point( i * first->width(), j * first->height()));
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
			be.tile = battlehexes[i];
		else
			be.tile = BattleHex::INVALID;

		if (hasPosition)
		{
			be.pos.x = positions[i].x;
			be.pos.y = positions[i].y;
		}
		else
		{
			const CStack * destStack = owner.getCurrentPlayerInterface()->cb->battleGetStackByPos(battlehexes[i], false);
			Rect tilePos = owner.fieldController->hexPositionLocal(battlehexes[i]);

			be.pos.x = tilePos.x + tilePos.w/2 - first->width()/2;

			if(destStack && destStack->doubleWide()) // Correction for 2-hex creatures.
				be.pos.x += (destStack->side == BattleSide::ATTACKER ? -1 : 1)*tilePos.w/2;

			if (alignToBottom())
				be.pos.y = tilePos.y + tilePos.h - first->height();
			else
				be.pos.y = tilePos.y - first->height()/2;
		}
		owner.effectsController->battleEffects.push_back(be);
	}
	return true;
}

void EffectAnimation::nextFrame()
{
	playEffect();

	if (effectFinished)
	{
		//remove visual effect itself only if sound has finished as well - necessary for obstacles like force field
		clearEffect();
		delete this;
	}
}

bool EffectAnimation::alignToBottom() const
{
	return effectFlags & ALIGN_TO_BOTTOM;
}

bool EffectAnimation::forceOnTop() const
{
	return effectFlags & FORCE_ON_TOP;
}

bool EffectAnimation::screenFill() const
{
	return effectFlags & SCREEN_FILL;
}

void EffectAnimation::onEffectFinished()
{
	effectFinished = true;
}

void EffectAnimation::playEffect()
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

void EffectAnimation::clearEffect()
{
	auto & effects = owner.effectsController->battleEffects;

	vstd::erase_if(effects, [&](const BattleEffect & effect){
		return effect.effectID == ID;
	});
}

EffectAnimation::~EffectAnimation()
{
	assert(effectFinished);
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
		delete this;
	});

	initializeProjectile();

	return true;
}

void HeroCastAnimation::initializeProjectile()
{
	// spell has no projectile to play, ignore this step
	if (spell->animationInfo.projectile.empty())
		return;

	// targeted spells should have well, target
	assert(tile.isValid());

	Point srccoord = hero->pos.center() - hero->parent->pos.topLeft();
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
	owner.executeAnimationStage(EAnimationEvents::HIT);
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

	if (!owner.projectilesController->hasActiveProjectile(nullptr, false))
	{
		emitAnimationEvent();
		//TODO: check H3 - it is possible that hero animation should be paused until hit effect is over, not just projectile
		hero->play();
	}
}
