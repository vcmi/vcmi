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
#include "CCreatureAnimation.h"

#include "../CGameInfo.h"
#include "../CMusicHandler.h"
#include "../CPlayerInterface.h"
#include "../Graphics.h"
#include "../gui/CAnimation.h"
#include "../gui/CCursorHandler.h"
#include "../gui/CGuiHandler.h"
#include "../gui/SDL_Extensions.h"

#include "../../CCallback.h"
#include "../../lib/CStack.h"
#include "../../lib/CTownHandler.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/spells/CSpellHandler.h"

CBattleAnimation::CBattleAnimation(CBattleInterface * _owner)
    : owner(_owner), ID(_owner->animIDhelper++)
{
	logAnim->trace("Animation #%d created", ID);
}

CBattleAnimation::~CBattleAnimation()
{
	logAnim->trace("Animation #%d deleted", ID);
}

void CBattleAnimation::endAnim()
{
	logAnim->trace("Animation #%d ended, type is %s", ID, typeid(this).name());
	for(auto & elem : owner->pendingAnims)
	{
		if(elem.first == this)
		{
			elem.first = nullptr;
		}
	}
}

bool CBattleAnimation::isEarliest(bool perStackConcurrency)
{
	int lowestMoveID = owner->animIDhelper + 5;
	CBattleStackAnimation * thAnim = dynamic_cast<CBattleStackAnimation *>(this);
	CEffectAnimation * thSen = dynamic_cast<CEffectAnimation *>(this);

	for(auto & elem : owner->pendingAnims)
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
	return (ID == lowestMoveID) || (lowestMoveID == (owner->animIDhelper + 5));
}

CBattleStackAnimation::CBattleStackAnimation(CBattleInterface * owner, const CStack * stack)
    : CBattleAnimation(owner),
      myAnim(owner->creAnims[stack->ID]),
      stack(stack)
{
	assert(myAnim);
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
	for(auto & elem : owner->pendingAnims)
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
	if(attacker == nullptr && owner->battleEffects.size() > 0)
		return false;

	ui32 lowestMoveID = owner->animIDhelper + 5;
	for(auto & elem : owner->pendingAnims)
	{

		CDefenceAnimation * defAnim = dynamic_cast<CDefenceAnimation *>(elem.first);
		if(defAnim && defAnim->stack->ID != stack->ID)
			continue;

		CAttackAnimation * attAnim = dynamic_cast<CAttackAnimation *>(elem.first);
		if(attAnim && attAnim->stack->ID != stack->ID)
			continue;

		CEffectAnimation * sen = dynamic_cast<CEffectAnimation *>(elem.first);
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
	if(attacker && owner->getCurrentPlayerInterface()->cb->isToReverse(stack->getPosition(), attacker->getPosition(), owner->creDir[stack->ID], attacker->doubleWide(), owner->creDir[attacker->ID]))
	{
		owner->addNewAnim(new CReverseAnimation(owner, stack, stack->getPosition(), true));
		return false;
	}
	//unit reversed

	if(rangedAttack) //delay hit animation
	{
		for(std::list<ProjectileInfo>::const_iterator it = owner->projectiles.begin(); it != owner->projectiles.end(); ++it)
		{
			if(it->creID == attacker->getCreature()->idNumber)
			{
				return false;
			}
		}
	}

	// synchronize animation with attacker, unless defending or attacked by shooter:
	// wait for 1/2 of attack animation
	if (!rangedAttack && getMyAnimType() != CCreatureAnim::DEFENCE)
	{
		float frameLength = AnimationControls::getCreatureAnimationSpeed(
		                          stack->getCreature(), owner->creAnims[stack->ID], getMyAnimType());

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

	bool toReverse = owner->getCurrentPlayerInterface()->cb->isToReverse(attackingStackPosBeforeReturn, attackedStack->getPosition(), owner->creDir[stack->ID], attackedStack->doubleWide(), owner->creDir[attackedStack->ID]);

	if(toReverse)
	{
		owner->addNewAnim(new CReverseAnimation(owner, stack, attackingStackPosBeforeReturn, true));
		return false;
	}

	// opponent must face attacker ( = different directions) before he can be attacked
	if(attackingStack && attackedStack &&
	    owner->creDir[attackingStack->ID] == owner->creDir[attackedStack->ID])
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

bool CMovementAnimation::shouldRotate()
{
	Point begPosition = CClickableHex::getXYUnitAnim(oldPos, stack, owner);
	Point endPosition = CClickableHex::getXYUnitAnim(nextHex, stack, owner);

	if((begPosition.x > endPosition.x) && owner->creDir[stack->ID] == true)
	{
		return true;
	}
	else if ((begPosition.x < endPosition.x) && owner->creDir[stack->ID] == false)
	{
		return true;
	}
	return false;
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

	if(owner->creAnims[stack->ID]->framesInGroup(CCreatureAnim::MOVING) == 0 ||
	   stack->hasBonus(Selector::typeSubtype(Bonus::FLYING, 1)))
	{
		//no movement or teleport, end immediately
		endAnim();
		return false;
	}

	//reverse unit if necessary
	if(shouldRotate())
	{
		// it seems that H3 does NOT plays full rotation animation here in most situations
		// Logical since it takes quite a lot of time
		if (curentMoveIndex == 0) // full rotation only for moving towards first tile.
		{
			owner->addNewAnim(new CReverseAnimation(owner, stack, oldPos, true));
			return false;
		}
		else
		{
			CReverseAnimation::rotateStack(owner, stack, oldPos);
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

	Point begPosition = CClickableHex::getXYUnitAnim(oldPos, stack, owner);
	Point endPosition = CClickableHex::getXYUnitAnim(nextHex, stack, owner);

	timeToMove = AnimationControls::getMovementDuration(stack->getCreature());

	begX = begPosition.x;
	begY = begPosition.y;
	progress = 0;
	distanceX = endPosition.x - begPosition.x;
	distanceY = endPosition.y - begPosition.y;

	if (stack->hasBonus(Selector::type(Bonus::FLYING)))
	{
		float distance = sqrt(distanceX * distanceX + distanceY * distanceY);

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
		Point coords = CClickableHex::getXYUnitAnim(nextHex, stack, owner);
		myAnim->pos = coords;

		// true if creature haven't reached the final destination hex
		if ((curentMoveIndex + 1) < destTiles.size())
		{
			// update the next hex field which has to be reached by the stack
			curentMoveIndex++;
			oldPos = nextHex;
			nextHex = destTiles[curentMoveIndex];

			// re-init animation
			for(auto & elem : owner->pendingAnims)
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

	myAnim->pos = CClickableHex::getXYUnitAnim(nextHex, stack, owner);
	CBattleAnimation::endAnim();

	owner->addNewAnim(new CMovementEndAnimation(owner, stack, nextHex));

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

void CReverseAnimation::rotateStack(CBattleInterface * owner, const CStack * stack, BattleHex hex)
{
	owner->creDir[stack->ID] = !owner->creDir[stack->ID];

	owner->creAnims[stack->ID]->pos = CClickableHex::getXYUnitAnim(hex, stack, owner);
}

void CReverseAnimation::setupSecondPart()
{
	if(!stack)
	{
		endAnim();
		return;
	}

	rotateStack(owner, stack, hex);

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
	catapultDamage(_catapultDmg)
{
	logAnim->debug("Created shooting anim for %s", stack->getName());
}

bool CShootingAnimation::init()
{
	if( !CAttackAnimation::checkInitialConditions() )
		return false;

	const CStack * shooter = attackingStack;

	if(!shooter || myAnim->isDead())
	{
		endAnim();
		return false;
	}

	//reverse unit if necessary
	if (attackingStack && attackedStack && owner->getCurrentPlayerInterface()->cb->isToReverse(attackingStack->getPosition(), attackedStack->getPosition(), owner->creDir[attackingStack->ID], attackingStack->doubleWide(), owner->creDir[attackedStack->ID]))
	{
		owner->addNewAnim(new CReverseAnimation(owner, attackingStack, attackingStack->getPosition(), true));
		return false;
	}

	// opponent must face attacker ( = different directions) before he can be attacked
	//FIXME: this cause freeze

//	if (attackingStack && attackedStack &&
//	    owner->creDir[attackingStack->ID] == owner->creDir[attackedStack->ID])
//		return false;

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
	spi.shotDone = false;
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

	destPos = CClickableHex::getXYUnitAnim(dest, attackedStack, owner);

	// to properly translate coordinates when shooter is rotated
	int multiplier = spi.reverse ? -1 : 1;

	double projectileAngle = atan2(fabs((double)destPos.y - fromPos.y), fabs((double)destPos.x - fromPos.x));
	if(shooter->getPosition() < dest)
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
		double animSpeed = AnimationControls::getProjectileSpeed(); // flight speed of projectile
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

		double animSpeed = AnimationControls::getProjectileSpeed() / 10;
		spi.lastStep = std::abs((destPos.x - spi.x) / animSpeed);
		spi.dx = animSpeed;
		spi.dy = 0;

		auto img = owner->idToProjectile[spi.creID]->getImage(0);

		// Add explosion anim
		Point animPos(destPos.x - 126 + img->width() / 2,
		              destPos.y - 105 + img->height() / 2);

		owner->addNewAnim( new CEffectAnimation(owner, catapultDamage ? "SGEXPL.DEF" : "CSGRCK.DEF", animPos.x, animPos.y));
	}

	auto & angles = shooterInfo->animation.missleFrameAngles;
	double pi = boost::math::constants::pi<double>();

	if (owner->idToProjectile.count(spi.creID) == 0) //in some cases (known one: hero grants shooter bonus to unit) the shooter stack's projectile may not be properly initialized
		owner->initStackProjectile(shooter);

	// only frames below maxFrame are usable: anything  higher is either no present or we don't know when it should be used
	size_t maxFrame = std::min<size_t>(angles.size(), owner->idToProjectile.at(spi.creID)->size(0));

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

	if(projectileAngle > straightAngle)
		group = CCreatureAnim::SHOOT_UP;
	else if(projectileAngle < -straightAngle)
		group = CCreatureAnim::SHOOT_DOWN;
	else //straight shot
		group = CCreatureAnim::SHOOT_FRONT;

	return true;
}

void CShootingAnimation::nextFrame()
{
	for(auto & it : owner->pendingAnims)
	{
		CMovementStartAnimation * anim = dynamic_cast<CMovementStartAnimation *>(it.first);
		CReverseAnimation * anim2 = dynamic_cast<CReverseAnimation *>(it.first);
		if( (anim && anim->stack->ID == stack->ID) || (anim2 && anim2->stack->ID == stack->ID && anim2->priority ) )
			return;
	}

	CAttackAnimation::nextFrame();
}

void CShootingAnimation::endAnim()
{
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
        if(owner->getCurrentPlayerInterface()->cb->isToReverse(attackingStack->getPosition(), attackedStack->getPosition(), owner->creDir[attackingStack->ID], attackingStack->doubleWide(), owner->creDir[attackedStack->ID]))
		{
			owner->addNewAnim(new CReverseAnimation(owner, attackingStack, attackingStack->getPosition(), true));
			return false;
		}
	}
	else
	{
        if(dest.isValid() && owner->getCurrentPlayerInterface()->cb->isToReverse(attackingStack->getPosition(), dest, owner->creDir[attackingStack->ID], false, false))
		{
			owner->addNewAnim(new CReverseAnimation(owner, attackingStack, attackingStack->getPosition(), true));
			return false;
		}
	}

	//TODO: display spell projectile here

	static const double straightAngle = 0.2;


	Point fromPos;
	Point destPos;

	// NOTE: two lines below return different positions (very notable with 2-hex creatures). Obtaining via creanims seems to be more precise
	fromPos = owner->creAnims[attackingStack->ID]->pos.topLeft();
	//xycoord = CClickableHex::getXYUnitAnim(shooter->getPosition(), true, shooter, owner);

	destPos = CClickableHex::getXYUnitAnim(dest, attackedStack, owner);


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
	for(auto & it : owner->pendingAnims)
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

				owner->battleEffects.push_back(be);
			}
		}
	}
	else // Effects targeted at a specific creature/hex.
	{
		const CStack * destStack = owner->getCurrentPlayerInterface()->cb->battleGetStackByPos(destTile, false);
		Rect & tilePos = owner->bfield[destTile]->pos;
		BattleEffect be;
		be.effectID = ID;
		be.animation = animation;
		be.currentFrame = 0;


		//todo: lightning anim frame count override

//			if(effect == 1)
//				be.maxFrame = 3;

		if(x == -1)
		{
			be.x = tilePos.x + tilePos.w/2 - first->width()/2;
		}
		else
		{
			be.x = x;
		}

		if(y == -1)
		{
			if(alignToBottom)
				be.y = tilePos.y + tilePos.h - first->height();
			else
				be.y = tilePos.y - first->height()/2;
		}
		else
		{
			be.y = y;
		}

		// Correction for 2-hex creatures.
		if(destStack != nullptr && destStack->doubleWide())
			be.x += (destStack->side == BattleSide::ATTACKER ? -1 : 1)*tilePos.w/2;

		//Indicate if effect should be drawn on top of everything or just on top of the hex
		be.position = destTile;

		owner->battleEffects.push_back(be);
	}
	//battleEffects
	return true;
}

void CEffectAnimation::nextFrame()
{
	//notice: there may be more than one effect in owner->battleEffects correcponding to this animation (ie. armageddon)
	for(auto & elem : owner->battleEffects)
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

	std::vector<std::list<BattleEffect>::iterator> toDel;

	for(auto it = owner->battleEffects.begin(); it != owner->battleEffects.end(); ++it)
	{
		if(it->effectID == ID)
		{
			toDel.push_back(it);
		}
	}

	for(auto & elem : toDel)
	{
		owner->battleEffects.erase(elem);
	}

	delete this;
}
