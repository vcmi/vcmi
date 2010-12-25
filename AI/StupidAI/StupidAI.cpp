#include "stdafx.h"
#include "StupidAI.h"
#include "../../lib/CGameState.h"

CStupidAI::CStupidAI(void)
	: side(-1), cb(NULL)
{
	print("created");
}


CStupidAI::~CStupidAI(void)
{
	print("destroyed");
}

void CStupidAI::init( IBattleCallback * CB )
{
	print("init called, saving ptr to IBattleCallback");
	cb = CB;
}

void CStupidAI::actionFinished( const BattleAction *action )
{
	print("actionFinished called");
}

void CStupidAI::actionStarted( const BattleAction *action )
{
	print("actionStarted called");
}

BattleAction CStupidAI::activeStack( const CStack * stack )
{
	print("activeStack called");
	if(stack->position % 17  <  5) //move army little towards enemy
	{
		THex dest = stack->position + !side*2 - 1;
		print(stack->nodeName() + "will be moved to " + boost::lexical_cast<std::string>(dest));
		return BattleAction::makeMove(stack, dest); 
	}
	return BattleAction::makeDefend(stack);
}

void CStupidAI::battleAttack(const BattleAttack *ba)
{
	print("battleAttack called");
}

void CStupidAI::battleStacksAttacked(const std::vector<BattleStackAttacked> & bsa)
{
	print("battleStacksAttacked called");
}

void CStupidAI::battleEnd(const BattleResult *br) 
{
	print("battleEnd called");
}

void CStupidAI::battleResultsApplied() 
{
	print("battleResultsApplied called");
}

void CStupidAI::battleNewRoundFirst(int round) 
{
	print("battleNewRoundFirst called");
}

void CStupidAI::battleNewRound(int round) 
{
	print("battleNewRound called");
}

void CStupidAI::battleStackMoved(const CStack * stack, THex dest, int distance, bool end) 
{
	print("battleStackMoved called");;
}

void CStupidAI::battleSpellCast(const BattleSpellCast *sc) 
{
	print("battleSpellCast called");
}

void CStupidAI::battleStacksEffectsSet(const SetStackEffect & sse) 
{
	print("battleStacksEffectsSet called");
}

void CStupidAI::battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool Side) 
{
	print("battleStart called");
	side = Side;
}

void CStupidAI::battleStacksHealedRes(const std::vector<std::pair<ui32, ui32> > & healedStacks, bool lifeDrain, si32 lifeDrainFrom) 
{
	print("battleStacksHealedRes called");
}

void CStupidAI::battleNewStackAppeared(const CStack * stack) 
{
	print("battleNewStackAppeared called");
}

void CStupidAI::battleObstaclesRemoved(const std::set<si32> & removedObstacles) 
{
	print("battleObstaclesRemoved called");
}

void CStupidAI::battleCatapultAttacked(const CatapultAttack & ca) 
{
	print("battleCatapultAttacked called");
}

void CStupidAI::battleStacksRemoved(const BattleStacksRemoved & bsr) 
{
	print("battleStacksRemoved called");
}

void CStupidAI::print(const std::string &text) const
{
	tlog0 << "CStupidAI [" << this <<"]: " << text << std::endl;
}