#include "StdInc.h"
#include "CMeleeAttackAnimation.h"

#include "CBattleInterface.h"
#include "CCreatureAnimation.h"
#include "../../lib/BattleState.h"
#include "CReverseAnimation.h"

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

	bool toReverse = isToReverse(attackingStackPosBeforeReturn, dest, owner->creDir[stack->ID], attackedStack->doubleWide(), owner->creDir[attackedStack->ID]);

	if(toReverse)
	{

		owner->addNewAnim(new CReverseAnimation(owner, stack, attackingStackPosBeforeReturn, true));
		return false;
	}
	//reversed

	shooting = false;

	static const CCreatureAnim::EAnimType mutPosToGroup[] = {CCreatureAnim::ATTACK_UP, CCreatureAnim::ATTACK_UP,
		CCreatureAnim::ATTACK_FRONT, CCreatureAnim::ATTACK_DOWN, CCreatureAnim::ATTACK_DOWN, CCreatureAnim::ATTACK_FRONT};

	int revShiftattacker = (attackingStack->attackerOwned ? -1 : 1);

	int mutPos = SBattleHex::mutualPosition(attackingStackPosBeforeReturn, dest);
	if(mutPos == -1 && attackingStack->doubleWide())
	{
		mutPos = SBattleHex::mutualPosition(attackingStackPosBeforeReturn + revShiftattacker, attackedStack->position);
	}
	if (mutPos == -1 && attackedStack->doubleWide())
	{
		mutPos = SBattleHex::mutualPosition(attackingStackPosBeforeReturn, attackedStack->occupiedHex());
	}
	if (mutPos == -1 && attackedStack->doubleWide() && attackingStack->doubleWide())
	{
		mutPos = SBattleHex::mutualPosition(attackingStackPosBeforeReturn + revShiftattacker, attackedStack->occupiedHex());
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

CMeleeAttackAnimation::CMeleeAttackAnimation(CBattleInterface * _owner, const CStack * attacker, SBattleHex _dest, const CStack * _attacked)
	: CAttackAnimation(_owner, attacker, _dest, _attacked)
{
}