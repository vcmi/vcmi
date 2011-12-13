#include "StdInc.h"
#include "CBattleAnimation.h"
#include "CBattleInterface.h"
#include "../../lib/BattleState.h"
#include "CSpellEffectAnimation.h"
#include "CReverseAnimation.h"

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

CBattleAnimation::CBattleAnimation(CBattleInterface * _owner)
	: owner(_owner), ID(_owner->animIDhelper++)
{}