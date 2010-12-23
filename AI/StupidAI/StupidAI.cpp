#include "stdafx.h"
#include "StupidAI.h"
#include "../../lib/CGameState.h"

CStupidAI::CStupidAI(void)
{
}


CStupidAI::~CStupidAI(void)
{
}

void CStupidAI::init( IBattleCallback * CB )
{

}

void CStupidAI::actionFinished( const BattleAction *action )
{

}

void CStupidAI::actionStarted( const BattleAction *action )
{

}

BattleAction CStupidAI::activeStack( const CStack * stack )
{
	BattleAction ba;
	ba.actionType = BattleAction::DEFEND;
	ba.stackNumber = stack->ID;
	return ba;
}