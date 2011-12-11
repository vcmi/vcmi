#include "../lib/NetPacks.h"
#include "Client.h"
#include "../lib/CGameState.h"
#include "../lib/Connection.h"
#include "../lib/CGameInterface.h"

#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "../lib/BattleState.h"
#include "CheckTime.h"


void postInfoCall(int timeUsed, int limit)
{
	tlog0 << "AI was processing info for " << timeUsed << " ms.\n";
	if(timeUsed > limit + MEASURE_MARGIN)
	{
		tlog1 << "That's too long (limit=" << limit << "+" << MEASURE_MARGIN << ")! AI is disqualified!\n";
		exit(1);
	}
}

void postDecisionCall(int timeUsed, const std::string &text/* = "AI was thinking over an action"*/, int timeLimit /*= MAKE_DECIDION_TIME*/)
{
	tlog0 << text << " for " << timeUsed << " ms.\n";
	if(timeUsed > timeLimit + MEASURE_MARGIN)
	{
		tlog1 << "That's too long! AI is disqualified!\n";
		exit(1);
	}
}

//macros to avoid code duplication - calls given method with given arguments if interface for specific player is present
//awaiting variadic templates...
// 

#define BATTLE_INTERFACE_CALL_IF_PRESENT_WITH_TIME_LIMIT(TIME_LIMIT, txt, function, ...) 		\
	do														\
	{														\
		int timeUsed = 0;									\
		if(cl->ai)											\
		{													\
			Bomb *b = new Bomb(TIME_LIMIT + HANGUP_TIME,txt);\
			CheckTime pr;									\
			cl->ai->function(__VA_ARGS__);					\
			postInfoCall(pr.timeSinceStart(), TIME_LIMIT);	\
			b->disarm();									\
		}													\
	} while(0)

#define BATTLE_INTERFACE_CALL_IF_PRESENT(function,...) 		\
	BATTLE_INTERFACE_CALL_IF_PRESENT_WITH_TIME_LIMIT(PROCESS_INFO_TIME, "process info timer", function, __VA_ARGS__)

#define UNEXPECTED_PACK assert(0)

void SetResources::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void SetResource::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void SetPrimSkill::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void SetSecSkill::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void HeroVisitCastle::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void ChangeSpells::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void SetMana::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void SetMovePoints::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void FoWChange::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void SetAvailableHeroes::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void ChangeStackCount::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void SetStackType::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void EraseStack::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void SwapStacks::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void InsertNewStack::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void RebalanceStacks::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void PutArtifact::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void EraseArtifact::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void MoveArtifact::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void AssembledArtifact::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void DisassembledArtifact::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void HeroVisit::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void NewTurn::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void GiveBonus::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void ChangeObjPos::applyFirstCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void ChangeObjPos::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void PlayerEndsGame::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void RemoveBonus::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void UpdateCampaignState::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void RemoveObject::applyFirstCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void RemoveObject::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void TryMoveHero::applyFirstCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void TryMoveHero::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void NewStructures::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void RazeStructures::applyCl (CClient *cl)
{
	UNEXPECTED_PACK;
}

void SetAvailableCreatures::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void SetHeroesInTown::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void HeroRecruited::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void GiveHero::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void GiveHero::applyFirstCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void InfoWindow::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void SetObjectProperty::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void HeroLevelUp::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void BlockingDialog::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void GarrisonDialog::applyCl(CClient *cl)
{
	UNEXPECTED_PACK;
}

void BattleStart::applyCl( CClient *cl )
{
	BATTLE_INTERFACE_CALL_IF_PRESENT_WITH_TIME_LIMIT(STARTUP_TIME, "battleStart timer", battleStart, info->belligerents[0], info->belligerents[1], info->tile, info->heroes[0], info->heroes[1], cl->color);
	if(info->tacticDistance && cl->color == info->tacticsSide)
	{
		boost::thread(&CClient::commenceTacticPhaseForInt, cl, cl->ai);
	}
}

void BattleNextRound::applyFirstCl(CClient *cl)
{
	BATTLE_INTERFACE_CALL_IF_PRESENT(battleNewRoundFirst,round);
}

void BattleNextRound::applyCl( CClient *cl )
{
	BATTLE_INTERFACE_CALL_IF_PRESENT(battleNewRound,round);
}

void BattleSetActiveStack::applyCl( CClient *cl )
{
	CStack * activated = GS(cl)->curB->getStack(stack);
	int playerToCall = -1; //player that will move activated stack
	if( activated->hasBonusOfType(Bonus::HYPNOTIZED))
		playerToCall = ( GS(cl)->curB->sides[0] == activated->owner ? GS(cl)->curB->sides[1] : GS(cl)->curB->sides[0] );
	else
		playerToCall = activated->owner;

	if(cl->ai && cl->color == playerToCall)
		cl->requestMoveFromAI(activated);
}

void BattleResult::applyFirstCl( CClient *cl )
{
	BATTLE_INTERFACE_CALL_IF_PRESENT(battleEnd,this);
}

void BattleStackMoved::applyFirstCl( CClient *cl )
{
	const CStack * movedStack = GS(cl)->curB->getStack(stack);
	BATTLE_INTERFACE_CALL_IF_PRESENT(battleStackMoved,movedStack,tilesToMove,distance);
}

void BattleStackAttacked::applyCl( CClient *cl )
{
	std::vector<BattleStackAttacked> bsa;
	bsa.push_back(*this);

	BATTLE_INTERFACE_CALL_IF_PRESENT(battleStacksAttacked,bsa);
}

void BattleAttack::applyFirstCl( CClient *cl )
{
	BATTLE_INTERFACE_CALL_IF_PRESENT(battleAttack,this);
	for (int g=0; g<bsa.size(); ++g)
	{
		for (int z=0; z<bsa[g].healedStacks.size(); ++z)
		{
			bsa[g].healedStacks[z].applyCl(cl);
		}
	}
}

void BattleAttack::applyCl( CClient *cl )
{
	BATTLE_INTERFACE_CALL_IF_PRESENT(battleStacksAttacked,bsa);
}

void StartAction::applyFirstCl( CClient *cl )
{
	cl->curbaction = new BattleAction(ba);
	BATTLE_INTERFACE_CALL_IF_PRESENT(actionStarted, &ba);
}

void BattleSpellCast::applyCl( CClient *cl )
{
	BATTLE_INTERFACE_CALL_IF_PRESENT(battleSpellCast,this);
	if(id >= 66 && id <= 69) //elemental summoning
	{
		BATTLE_INTERFACE_CALL_IF_PRESENT(battleNewStackAppeared,GS(cl)->curB->stacks.back());
	}
}

void SetStackEffect::applyCl( CClient *cl )
{
	//informing about effects
	BATTLE_INTERFACE_CALL_IF_PRESENT(battleStacksEffectsSet,*this);
}

void StacksInjured::applyCl( CClient *cl )
{
	BATTLE_INTERFACE_CALL_IF_PRESENT(battleStacksAttacked,stacks);
}

void BattleResultsApplied::applyCl( CClient *cl )
{
	cl->terminate = true;
	CloseServer cs;
	*cl->serv << &cs;
}

void StacksHealedOrResurrected::applyCl( CClient *cl )
{
	std::vector<std::pair<ui32, ui32> > shiftedHealed;
	for(int v=0; v<healedStacks.size(); ++v)
	{
		shiftedHealed.push_back(std::make_pair(healedStacks[v].stackID, healedStacks[v].healedHP));
	}
	BATTLE_INTERFACE_CALL_IF_PRESENT(battleStacksHealedRes, shiftedHealed, lifeDrain, tentHealing, drainedFrom);
}

void ObstaclesRemoved::applyCl( CClient *cl )
{
	//inform interfaces about removed obstacles
	BATTLE_INTERFACE_CALL_IF_PRESENT(battleObstaclesRemoved, obstacles);
}

void CatapultAttack::applyCl( CClient *cl )
{
	//inform interfaces about catapult attack
	BATTLE_INTERFACE_CALL_IF_PRESENT(battleCatapultAttacked, *this);
}

void BattleStacksRemoved::applyCl( CClient *cl )
{
	//inform interfaces about removed stacks
	BATTLE_INTERFACE_CALL_IF_PRESENT(battleStacksRemoved, *this);
}

CGameState* CPackForClient::GS( CClient *cl )
{
	return cl->gs;
}

void EndAction::applyCl( CClient *cl )
{
	BATTLE_INTERFACE_CALL_IF_PRESENT(actionFinished, cl->curbaction);

	delNull(cl->curbaction);
}

void PackageApplied::applyCl( CClient *cl )
{
 	ui8 player = GS(cl)->currentPlayer;
 	//INTERFACE_CALL_IF_PRESENT(player, requestRealized, this);
 	if(cl->waitingRequest.get() == packType)
 		cl->waitingRequest.setn(false);
 	else if(cl->waitingRequest.get())
 		tlog3 << "Surprising server message!\n";
}

void SystemMessage::applyCl( CClient *cl )
{
	std::ostringstream str;
	str << "System message: " << text;

	tlog4 << str.str() << std::endl;
}

void PlayerBlocked::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void YourTurn::applyCl( CClient *cl )
{
	UNEXPECTED_PACK;
}

void SaveGame::applyCl(CClient *cl)
{
	UNEXPECTED_PACK;
}

void PlayerMessage::applyCl(CClient *cl)
{
	std::ostringstream str;
	str << "Player "<<(int)player<<" sends a message: " << text;

	tlog4 << str.str() << std::endl;
}

void SetSelection::applyCl(CClient *cl)
{
	UNEXPECTED_PACK;
}

void ShowInInfobox::applyCl(CClient *cl)
{
	UNEXPECTED_PACK;
}

void AdvmapSpellCast::applyCl(CClient *cl)
{
	UNEXPECTED_PACK;
}

void OpenWindow::applyCl(CClient *cl)
{
	UNEXPECTED_PACK;
}

void CenterView::applyCl(CClient *cl)
{
	UNEXPECTED_PACK;
}

void NewObject::applyCl(CClient *cl)
{
	UNEXPECTED_PACK;
}

void SetAvailableArtifacts::applyCl(CClient *cl)
{
	UNEXPECTED_PACK;
}

void TradeComponents::applyCl(CClient *cl)
{
	UNEXPECTED_PACK;
}