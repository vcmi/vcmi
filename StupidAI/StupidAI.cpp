#include "stdafx.h"
#include "StupidAI.h"

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

BattleAction CStupidAI::activeStack( int stackID )
{
	BattleAction ba;
	ba.DEFEND;
	ba.stackNumber = stackID;
	return ba;
}