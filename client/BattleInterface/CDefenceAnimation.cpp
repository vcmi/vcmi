#include "StdInc.h"
#include "CDefenceAnimation.h"

#include "CBattleInterface.h"
#include "../CGameInfo.h"
#include "CCreatureAnimation.h"
#include "../CPlayerInterface.h"
#include "../CMusicHandler.h"
#include "../../lib/BattleState.h"
#include "CReverseAnimation.h"
#include "CAttackAnimation.h"
#include "CShootingAnimation.h"

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
			if( attackerAnimType == 11 && attackerAnimType == 12 && attackerAnimType == 13 && owner->creAnims[attacker->ID]->getFrame() < attacker->getCreature()->attackClimaxFrame )
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
	if(attacker && isToReverse(stack->position, attacker->position, owner->creDir[stack->ID], attacker->doubleWide(), owner->creDir[attacker->ID]))
	{
		owner->addNewAnim(new CReverseAnimation(owner, stack, stack->position, true));
		return false;
	}
	//unit reversed

	if(byShooting) //delay hit animation
	{		
		for(std::list<SProjectileInfo>::const_iterator it = owner->projectiles.begin(); it != owner->projectiles.end(); ++it)
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
		if( myAnim()->getType() == CCreatureAnim::DEATH && (owner->animCount+1)%(4/owner->curInt->sysOpts.animSpeed)==0
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

CDefenceAnimation::CDefenceAnimation(SStackAttackedInfo _attackedInfo, CBattleInterface * _owner)
: CBattleStackAnimation(_owner, _attackedInfo.defender), dmg(_attackedInfo.dmg),
amountKilled(_attackedInfo.amountKilled), attacker(_attackedInfo.attacker), byShooting(_attackedInfo.byShooting),
killed(_attackedInfo.killed)
{
}