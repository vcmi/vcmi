#include "StdInc.h"
#include "CAttackAnimation.h"

#include "../CMusicHandler.h"
#include "../CGameInfo.h"
#include "CBattleInterface.h"
#include "CCreatureAnimation.h"
#include "../../lib/BattleState.h"
#include "../CPlayerInterface.h"
#include "../../CCallback.h"

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

CAttackAnimation::CAttackAnimation(CBattleInterface *_owner, const CStack *attacker, SBattleHex _dest, const CStack *defender)
: CBattleStackAnimation(_owner, attacker), dest(_dest), attackedStack(defender), attackingStack(attacker)
{

	assert(attackingStack && "attackingStack is NULL in CBattleAttack::CBattleAttack !\n");
	if(attackingStack->getCreature()->idNumber != 145) //catapult is allowed to attack not-creature
	{
		assert(attackedStack && "attackedStack is NULL in CBattleAttack::CBattleAttack !\n");
	}
	else //catapult can attack walls only
	{
		assert(owner->curInt->cb->battleGetWallUnderHex(_dest) >= 0);
	}
	attackingStackPosBeforeReturn = attackingStack->position;
}