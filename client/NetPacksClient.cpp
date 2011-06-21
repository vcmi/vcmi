#include "../lib/NetPacks.h"
#include "../CCallback.h"
#include "Client.h"
#include "CPlayerInterface.h"
#include "CGameInfo.h"
#include "../lib/Connection.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CDefObjInfoHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CObjectHandler.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/map.h"
#include "../lib/VCMIDirs.h"
#include "../lib/CSpellHandler.h"
#include "CSoundBase.h"
#include "mapHandler.h"
#include "GUIClasses.h"
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "CConfigHandler.h"
#include "SDL_Extensions.h"
#include "CBattleInterface.h"
#include "../lib/CCampaignHandler.h"
#include "../lib/CGameState.h"
#include "../lib/BattleState.h"


//macros to avoid code duplication - calls given method with given arguments if interface for specific player is present
//awaiting variadic templates...
#define CALL_IN_PRIVILAGED_INTS(function, ...)										\
	do																				\
	{																				\
		BOOST_FOREACH(IGameEventsReceiver *ger, cl->privilagedGameEventReceivers)	\
			ger->function(__VA_ARGS__);												\
	} while(0)

#define CALL_ONLY_THAT_INTERFACE(player, function, ...)		\
		do													\
		{													\
		if(vstd::contains(cl->playerint,player))			\
			cl->playerint[player]->function(__VA_ARGS__);	\
		}while(0)											

#define INTERFACE_CALL_IF_PRESENT(player,function,...) 				\
		do															\
		{															\
			CALL_ONLY_THAT_INTERFACE(player, function, __VA_ARGS__);\
			CALL_IN_PRIVILAGED_INTS(function, __VA_ARGS__);			\
		} while(0)										

#define CALL_ONLY_THT_BATTLE_INTERFACE(player,function, ...) 	\
	do															\
	{															\
		if(vstd::contains(cl->battleints,player))				\
			cl->battleints[player]->function(__VA_ARGS__);		\
	} while (0);

#define BATTLE_INTERFACE_CALL_RECEIVERS(function,...) 	\
	do															\
	{															\
		BOOST_FOREACH(IBattleEventsReceiver *ber, cl->privilagedBattleEventReceivers)\
			ber->function(__VA_ARGS__);							\
	} while(0)

#define BATTLE_INTERFACE_CALL_IF_PRESENT(player,function,...) 	\
	do															\
	{															\
		CALL_ONLY_THAT_INTERFACE(player, function, __VA_ARGS__);\
		BATTLE_INTERFACE_CALL_RECEIVERS(function, __VA_ARGS__);	\
	} while(0)

//calls all normal interfaces and privilaged ones, playerints may be updated when iterating over it, so we need a copy
#define CALL_IN_ALL_INTERFACES(function, ...)							\
	do																	\
	{																	\
		std::map<ui8, CGameInterface*> ints = cl->playerint;			\
		for(std::map<ui8, CGameInterface*>::iterator i = ints.begin(); i != ints.end(); i++)\
			CALL_ONLY_THAT_INTERFACE(i->first, function, __VA_ARGS__);	\
	} while(0)


#define BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(function,...) 				\
	CALL_ONLY_THT_BATTLE_INTERFACE(GS(cl)->curB->sides[0], function, __VA_ARGS__)	\
	CALL_ONLY_THT_BATTLE_INTERFACE(GS(cl)->curB->sides[1], function, __VA_ARGS__)	\
	BATTLE_INTERFACE_CALL_RECEIVERS(function, __VA_ARGS__)
/*
 * NetPacksClient.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

void SetResources::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(player,receivedResource,-1,-1);
}

void SetResource::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(player,receivedResource,resid,val);
}

void SetPrimSkill::applyCl( CClient *cl )
{
	const CGHeroInstance *h = cl->getHero(id);
	if(!h)
	{
		tlog1 << "Cannot find hero with ID " << id << std::endl;
		return;
	}
	INTERFACE_CALL_IF_PRESENT(h->tempOwner,heroPrimarySkillChanged,h,which,val);
}

void SetSecSkill::applyCl( CClient *cl )
{
	const CGHeroInstance *h = cl->getHero(id);
	if(!h)
	{
		tlog1 << "Cannot find hero with ID " << id << std::endl;
		return;
	}
	INTERFACE_CALL_IF_PRESENT(h->tempOwner,heroSecondarySkillChanged,h,which,val);
}

void HeroVisitCastle::applyCl( CClient *cl )
{
	const CGHeroInstance *h = cl->getHero(hid);

	if(start())
	{
		INTERFACE_CALL_IF_PRESENT(h->tempOwner, heroVisitsTown, h, GS(cl)->getTown(tid));
	}
}

void ChangeSpells::applyCl( CClient *cl )
{
	//TODO: inform interface?
}

void SetMana::applyCl( CClient *cl )
{
	const CGHeroInstance *h = cl->getHero(hid);
	INTERFACE_CALL_IF_PRESENT(h->tempOwner, heroManaPointsChanged, h);
}

void SetMovePoints::applyCl( CClient *cl )
{
	const CGHeroInstance *h = cl->getHero(hid);

	if (cl->IGameCallback::getSelectedHero(LOCPLINT->playerID) == h)//if we have selected that hero
	{
		GS(cl)->calculatePaths(h, *cl->pathInfo);
	}

	INTERFACE_CALL_IF_PRESENT(h->tempOwner, heroMovePointsChanged, h);
}

void FoWChange::applyCl( CClient *cl )
{
	if(mode)
		INTERFACE_CALL_IF_PRESENT(player, tileRevealed, tiles);
	else
		INTERFACE_CALL_IF_PRESENT(player, tileHidden, tiles);

	cl->updatePaths();
}

void SetAvailableHeroes::applyCl( CClient *cl )
{
	//TODO: inform interface?
}

void ChangeStackCount::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(sl.army->tempOwner, stackChagedCount, sl, count, absoluteValue);
}

void SetStackType::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(sl.army->tempOwner, stackChangedType, sl, *type);
}

void EraseStack::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(sl.army->tempOwner, stacksErased, sl);
}

void SwapStacks::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(sl1.army->tempOwner, stacksSwapped, sl1, sl2);
	if(sl1.army->tempOwner != sl2.army->tempOwner)
		INTERFACE_CALL_IF_PRESENT(sl2.army->tempOwner, stacksSwapped, sl1, sl2);
}

void InsertNewStack::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(sl.army->tempOwner,newStackInserted,sl, *sl.getStack());
}

void RebalanceStacks::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(src.army->tempOwner, stacksRebalanced, src, dst, count);
	if(src.army->tempOwner != dst.army->tempOwner)
		INTERFACE_CALL_IF_PRESENT(dst.army->tempOwner,stacksRebalanced, src, dst, count);
}

void PutArtifact::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(al.hero->tempOwner, artifactPut, al);
}

void EraseArtifact::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(al.hero->tempOwner, artifactRemoved, al);
}

void MoveArtifact::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(src.hero->tempOwner, artifactMoved, src, dst);
	if(src.hero->tempOwner != dst.hero->tempOwner)
		INTERFACE_CALL_IF_PRESENT(src.hero->tempOwner, artifactMoved, src, dst);
}

void AssembledArtifact::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(al.hero->tempOwner, artifactAssembled, al);
}

void DisassembledArtifact::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(al.hero->tempOwner, artifactDisassembled, al);
}

void HeroVisit::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(hero->tempOwner, heroVisit, hero, obj, starting);
}


void GiveBonus::applyCl( CClient *cl )
{
	switch(who)
	{
	case HERO:
		{
			const CGHeroInstance *h = GS(cl)->getHero(id);
			INTERFACE_CALL_IF_PRESENT(h->tempOwner, heroBonusChanged, h, *h->bonuses.back(),true);
		}
		break;
	case PLAYER:
		{
			const PlayerState *p = GS(cl)->getPlayer(id);
			INTERFACE_CALL_IF_PRESENT(id, playerBonusChanged, *p->bonuses.back(), true);
		}
		break;
	}
}

void ChangeObjPos::applyFirstCl( CClient *cl )
{
	CGObjectInstance *obj = GS(cl)->map->objects[objid];
	if(flags & 1)
		CGI->mh->hideObject(obj);
}
void ChangeObjPos::applyCl( CClient *cl )
{
	CGObjectInstance *obj = GS(cl)->map->objects[objid];
	if(flags & 1)
		CGI->mh->printObject(obj);

	cl->updatePaths();
}

void PlayerEndsGame::applyCl( CClient *cl )
{
	CALL_IN_ALL_INTERFACES(gameOver, player, victory);
}

void RemoveBonus::applyCl( CClient *cl )
{
	switch(who)
	{
	case HERO:
		{
			const CGHeroInstance *h = GS(cl)->getHero(id);
			INTERFACE_CALL_IF_PRESENT(h->tempOwner, heroBonusChanged, h, bonus,false);
		}
		break;
	case PLAYER:
		{
			const PlayerState *p = GS(cl)->getPlayer(id);
			INTERFACE_CALL_IF_PRESENT(id, playerBonusChanged, bonus, false);
		}
		break;
	}
}

void UpdateCampaignState::applyCl( CClient *cl )
{
	cl->stopConnection();
	if(camp->mapsRemaining.size())
		cl->proposeNextMission(camp);
	else
		cl->finishCampaign(camp);
}

void RemoveObject::applyFirstCl( CClient *cl )
{
	const CGObjectInstance *o = cl->getObj(id);
	CGI->mh->hideObject(o);

	int3 pos = o->visitablePos();
	//notify interfaces about removal
	for(std::map<ui8, CGameInterface*>::iterator i=cl->playerint.begin();i!=cl->playerint.end();i++)
	{
		if(i->first >= PLAYER_LIMIT) continue;
		if(GS(cl)->getPlayerTeam(i->first)->fogOfWarMap[pos.x][pos.y][pos.z])
		{
			i->second->objectRemoved(o);
		}
	}
}

void RemoveObject::applyCl( CClient *cl )
{
	if(cl->pathInfo->hero && cl->pathInfo->hero->id != id)
		GS(cl)->calculatePaths(cl->pathInfo->hero, *cl->pathInfo);
}

void TryMoveHero::applyFirstCl( CClient *cl )
{
	CGHeroInstance *h = GS(cl)->getHero(id);

	//check if playerint will have the knowledge about movement - if not, directly update maphandler
	for(std::map<ui8, CGameInterface*>::iterator i=cl->playerint.begin();i!=cl->playerint.end();i++)
	{
		if(i->first >= PLAYER_LIMIT)
			continue;
		TeamState *t = GS(cl)->getPlayerTeam(i->first);
		if((t->fogOfWarMap[start.x-1][start.y][start.z] || t->fogOfWarMap[end.x-1][end.y][end.z])
				&& GS(cl)->getPlayer(i->first)->human)
			humanKnows = true;
	}

	if(result == TELEPORTATION  ||  result == EMBARK  ||  result == DISEMBARK  ||  !humanKnows)
		CGI->mh->removeObject(h);


	if(result == DISEMBARK)
		CGI->mh->printObject(h->boat);
}

void TryMoveHero::applyCl( CClient *cl )
{
	const CGHeroInstance *h = cl->getHero(id);

	if(result == TELEPORTATION  ||  result == EMBARK  ||  result == DISEMBARK)
		CGI->mh->printObject(h);

	if(result == EMBARK)
		CGI->mh->hideObject(h->boat);

	int player = h->tempOwner;

	if(vstd::contains(cl->playerint,player))
	{
		cl->playerint[player]->tileRevealed(fowRevealed);
	}

	//notify interfaces about move
	for(std::map<ui8, CGameInterface*>::iterator i=cl->playerint.begin();i!=cl->playerint.end();i++)
	{
		if(i->first >= PLAYER_LIMIT) continue;
		TeamState *t = GS(cl)->getPlayerTeam(i->first);
		if(t->fogOfWarMap[start.x-1][start.y][start.z] || t->fogOfWarMap[end.x-1][end.y][end.z])
		{
			i->second->heroMoved(*this);
		}
	}

	if(!humanKnows) //maphandler didn't get update from playerint, do it now
	{				//TODO: restructure nicely
		CGI->mh->printObject(h);
	}
}

void NewStructures::applyCl( CClient *cl )
{
	CGTownInstance *town = GS(cl)->getTown(tid);
	BOOST_FOREACH(si32 id, bid)
	{
		if(id==13) //fort or capitol
		{
			town->defInfo = GS(cl)->capitols[town->subID];
		}
		if(id ==7)
		{
			town->defInfo = GS(cl)->forts[town->subID];
		}
		if(vstd::contains(cl->playerint,town->tempOwner))
			cl->playerint[town->tempOwner]->buildChanged(town,id,1);
	}
}
void RazeStructures::applyCl (CClient *cl)
{
	CGTownInstance *town = GS(cl)->getTown(tid);
	BOOST_FOREACH(si32 id, bid)
	{
		if (id == 13) //fort or capitol
		{
			town->defInfo = GS(cl)->forts[town->subID];
		}
		if(vstd::contains (cl->playerint,town->tempOwner))
			cl->playerint[town->tempOwner]->buildChanged (town,id,2);
	}
}

void SetAvailableCreatures::applyCl( CClient *cl )
{
	const CGDwelling *dw = static_cast<const CGDwelling*>(cl->getObj(tid));

	//inform order about the change

	int p = -1;
	if(dw->ID == 106) //War Machines Factory is not flaggable, it's "owned" by visitor
		p = cl->getTile(dw->visitablePos())->visitableObjects.back()->tempOwner;
	else
		p = dw->tempOwner;

	INTERFACE_CALL_IF_PRESENT(p, availableCreaturesChanged, dw);
}

void SetHeroesInTown::applyCl( CClient *cl )
{
	CGTownInstance *t = GS(cl)->getTown(tid);
	if(vstd::contains(cl->playerint,t->tempOwner))
		cl->playerint[t->tempOwner]->heroInGarrisonChange(t);
}

// void SetHeroArtifacts::applyCl( CClient *cl )
// {
// 	tlog1 << "SetHeroArtifacts :(\n";
// // 
// // 	CGHeroInstance *h = GS(cl)->getHero(hid);
// // 	CGameInterface *player = (vstd::contains(cl->playerint,h->tempOwner) ? cl->playerint[h->tempOwner] : NULL);
// // 	if(!player)
// // 		return;
// 
// 	//h->recreateArtBonuses();
// 	//player->heroArtifactSetChanged(h);
// 
// // 	BOOST_FOREACH(Bonus bonus, gained)
// // 	{
// // 		player->heroBonusChanged(h,bonus,true);
// // 	}
// // 	BOOST_FOREACH(Bonus bonus, lost)
// // 	{
// // 		player->heroBonusChanged(h,bonus,false);
// // 	}
// }

void HeroRecruited::applyCl( CClient *cl )
{
	CGHeroInstance *h = GS(cl)->map->heroes.back();
	if(h->subID != hid)
	{
		tlog1 << "Something wrong with hero recruited!\n";
	}

	CGI->mh->initHeroDef(h);
	CGI->mh->printObject(h);
		
	if(vstd::contains(cl->playerint,h->tempOwner))
	{
		cl->playerint[h->tempOwner]->heroCreated(h);
		if(const CGTownInstance *t = GS(cl)->getTown(tid))
			cl->playerint[h->tempOwner]->heroInGarrisonChange(t);
	}
}

void GiveHero::applyCl( CClient *cl )
{
	CGHeroInstance *h = GS(cl)->getHero(id);
	CGI->mh->initHeroDef(h);
	CGI->mh->printObject(h);
	cl->playerint[h->tempOwner]->heroCreated(h);
}

void GiveHero::applyFirstCl( CClient *cl )
{
	CGI->mh->hideObject(GS(cl)->getHero(id));
}

void InfoWindow::applyCl( CClient *cl )
{
	std::vector<Component*> comps;
	for(size_t i=0;i<components.size();i++) 
	{
		comps.push_back(&components[i]);
	}
	std::string str;
	text.toString(str);

	if(vstd::contains(cl->playerint,player))
		cl->playerint[player]->showInfoDialog(str,comps,(soundBase::soundID)soundID);
	else
		tlog2 << "We received InfoWindow for not our player...\n";
}

void SetObjectProperty::applyCl( CClient *cl )
{
	//inform all players that see this object
	for(std::map<ui8,CGameInterface *>::const_iterator it = cl->playerint.begin(); it != cl->playerint.end(); ++it)
	{
		if(GS(cl)->isVisible(GS(cl)->map->objects[id], it->first))
			INTERFACE_CALL_IF_PRESENT(it->first, objectPropertyChanged, this);
	}
}

void HeroLevelUp::applyCl( CClient *cl )
{
	CGHeroInstance *h = GS(cl)->getHero(heroid);
	if(vstd::contains(cl->playerint,h->tempOwner))
	{
		boost::function<void(ui32)> callback = boost::function<void(ui32)>(boost::bind(&CCallback::selectionMade,LOCPLINT->cb,_1,id));
		cl->playerint[h->tempOwner]->heroGotLevel(const_cast<const CGHeroInstance*>(h),static_cast<int>(primskill),skills, callback);
	}
}

void BlockingDialog::applyCl( CClient *cl )
{
	std::string str;
	text.toString(str);

	if(vstd::contains(cl->playerint,player))
		cl->playerint[player]->showBlockingDialog(str,components,id,(soundBase::soundID)soundID,selection(),cancel());
	else
		tlog2 << "We received YesNoDialog for not our player...\n";
}

void GarrisonDialog::applyCl(CClient *cl)
{
	const CGHeroInstance *h = cl->getHero(hid);
	const CArmedInstance *obj = static_cast<const CArmedInstance*>(cl->getObj(objid));

	if(!vstd::contains(cl->playerint,h->getOwner()))
		return;

	boost::function<void()> callback = boost::bind(&CCallback::selectionMade,LOCPLINT->cb,0,id);
	cl->playerint[h->getOwner()]->showGarrisonDialog(obj,h,removableUnits,callback);
}

void BattleStart::applyCl( CClient *cl )
{
	cl->battleStarted(info);
}

void BattleNextRound::applyFirstCl(CClient *cl)
{
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleNewRoundFirst,round);
}

void BattleNextRound::applyCl( CClient *cl )
{
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleNewRound,round);
}

void BattleSetActiveStack::applyCl( CClient *cl )
{
	CStack * activated = GS(cl)->curB->getStack(stack);
	int playerToCall = -1; //player that will move activated stack
	if( activated->hasBonusOfType(Bonus::HYPNOTIZED) )
	{
		playerToCall = ( GS(cl)->curB->sides[0] == activated->owner ? GS(cl)->curB->sides[1] : GS(cl)->curB->sides[0] );
	}
	else
	{
		playerToCall = activated->owner;
	}
	if( vstd::contains(cl->battleints, playerToCall) )
		boost::thread( boost::bind(&CClient::waitForMoveAndSend, cl, playerToCall) );
}

void BattleResult::applyFirstCl( CClient *cl )
{
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleEnd,this);
}

void BattleStackMoved::applyFirstCl( CClient *cl )
{
	const CStack * movedStack = GS(cl)->curB->getStack(stack);
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleStackMoved,movedStack,tile,distance,ending);
}

void BattleStackAttacked::applyCl( CClient *cl )
{
	std::vector<BattleStackAttacked> bsa;
	bsa.push_back(*this);

	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleStacksAttacked,bsa);
}

void BattleAttack::applyFirstCl( CClient *cl )
{
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleAttack,this);
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
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleStacksAttacked,bsa);
}

void StartAction::applyFirstCl( CClient *cl )
{
	cl->curbaction = new BattleAction(ba);
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(actionStarted, &ba);
}

void BattleSpellCast::applyCl( CClient *cl )
{
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleSpellCast,this);

	if(id >= 66 && id <= 69) //elemental summoning
	{
		BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleNewStackAppeared,GS(cl)->curB->stacks.back());
	}
}

void SetStackEffect::applyCl( CClient *cl )
{
	//informing about effects
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleStacksEffectsSet,*this);
}

void StacksInjured::applyCl( CClient *cl )
{
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleStacksAttacked,stacks);
}

void BattleResultsApplied::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(player1, battleResultsApplied);
	INTERFACE_CALL_IF_PRESENT(player2, battleResultsApplied);
	INTERFACE_CALL_IF_PRESENT(254, battleResultsApplied);
	if(GS(cl)->initialOpts->mode == StartInfo::DUEL)
	{
		cl->terminate = true;
		CloseServer cs;
		*cl->serv << &cs;
	}
}

void StacksHealedOrResurrected::applyCl( CClient *cl )
{
	std::vector<std::pair<ui32, ui32> > shiftedHealed;
	for(int v=0; v<healedStacks.size(); ++v)
	{
		shiftedHealed.push_back(std::make_pair(healedStacks[v].stackID, healedStacks[v].healedHP));
	}
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleStacksHealedRes, shiftedHealed, lifeDrain, tentHealing, drainedFrom);
}

void ObstaclesRemoved::applyCl( CClient *cl )
{
	//inform interfaces about removed obstacles
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleObstaclesRemoved, obstacles);
}

void CatapultAttack::applyCl( CClient *cl )
{
	//inform interfaces about catapult attack
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleCatapultAttacked, *this);
}

void BattleStacksRemoved::applyCl( CClient *cl )
{
	//inform interfaces about removed stacks
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(battleStacksRemoved, *this);
}

CGameState* CPackForClient::GS( CClient *cl )
{
	return cl->gs;
}

void EndAction::applyCl( CClient *cl )
{
	BATTLE_INTERFACE_CALL_IF_PRESENT_FOR_BOTH_SIDES(actionFinished, cl->curbaction);

	delete cl->curbaction;
	cl->curbaction = NULL;
}

void PackageApplied::applyCl( CClient *cl )
{
	ui8 player = GS(cl)->currentPlayer;
	INTERFACE_CALL_IF_PRESENT(player, requestRealized, this);
	if(cl->waitingRequest.get())
		cl->waitingRequest.setn(false);
}

void SystemMessage::applyCl( CClient *cl )
{
	std::ostringstream str;
	str << "System message: " << text;

	tlog4 << str.str() << std::endl;
	if(LOCPLINT)
		LOCPLINT->cingconsole->print(str.str());
}

void PlayerBlocked::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(player,playerBlocked,reason);
}

void YourTurn::applyCl( CClient *cl )
{
	CALL_ONLY_THAT_INTERFACE(player,yourTurn);
}

void SaveGame::applyCl(CClient *cl)
{
	CSaveFile save(GVCMIDirs.UserPath + "/Games/" + fname + ".vcgm1");
	save << *cl;
}

void PlayerMessage::applyCl(CClient *cl)
{
	std::ostringstream str;
	str << "Player "<<(int)player<<" sends a message: " << text;

	tlog4 << str.str() << std::endl;
	if(LOCPLINT)
		LOCPLINT->cingconsole->print(str.str());
}

void SetSelection::applyCl(CClient *cl)
{
	const CGHeroInstance *h = cl->getHero(id);
	if(!h)
		return;

	//CPackForClient::GS(cl)->calculatePaths(h, *cl->pathInfo);
}

void ShowInInfobox::applyCl(CClient *cl)
{
	SComponent sc(c);
	text.toString(sc.description);
	if(vstd::contains(cl->playerint, player) && cl->playerint[player]->human)
	{
		static_cast<CPlayerInterface*>(cl->playerint[player])->showComp(sc);
	}
}


void AdvmapSpellCast::applyCl(CClient *cl)
{
	cl->playerint[caster->getOwner()]->advmapSpellCast(caster, spellID);
}

void OpenWindow::applyCl(CClient *cl)
{
	switch(window)
	{
	case EXCHANGE_WINDOW:
		{
			const CGHeroInstance *h = cl->getHero(id1);
			const CGObjectInstance *h2 = cl->getHero(id2);
			assert(h && h2);
			INTERFACE_CALL_IF_PRESENT(h->tempOwner,heroExchangeStarted, id1, id2);
		}
		break;
	case RECRUITMENT_FIRST:
	case RECRUITMENT_ALL:
		{
			const CGDwelling *dw = dynamic_cast<const CGDwelling*>(cl->getObj(id1));
			const CArmedInstance *dst = dynamic_cast<const CArmedInstance*>(cl->getObj(id2));
			INTERFACE_CALL_IF_PRESENT(dst->tempOwner,showRecruitmentDialog, dw, dst, window == RECRUITMENT_FIRST ? 0 : -1);
		}
		break;
	case SHIPYARD_WINDOW:
		{
			const IShipyard *sy = IShipyard::castFrom(cl->getObj(id1));
			INTERFACE_CALL_IF_PRESENT(sy->o->tempOwner, showShipyardDialog, sy);
		}
		break;
	case THIEVES_GUILD:
		{
			//displays Thieves' Guild window (when hero enters Den of Thieves)
			const CGObjectInstance *obj = cl->getObj(id1);
			GH.pushInt( new CThievesGuildWindow(obj) );
		}
		break;
	case UNIVERSITY_WINDOW:
		{
			//displays University window (when hero enters University on adventure map)
			const IMarket *market = IMarket::castFrom(cl->getObj(id1));
			const CGHeroInstance *hero = cl->getHero(id2);
			INTERFACE_CALL_IF_PRESENT(hero->tempOwner,showUniversityWindow, market, hero);
		}
		break;
	case MARKET_WINDOW:
		{
			//displays Thieves' Guild window (when hero enters Den of Thieves)
			const CGObjectInstance *obj = cl->getObj(id1);
			const CGHeroInstance *hero = cl->getHero(id2);
			const IMarket *market = IMarket::castFrom(obj);
			INTERFACE_CALL_IF_PRESENT(cl->getTile(obj->visitablePos())->visitableObjects.back()->tempOwner, showMarketWindow, market, hero);
		}
		break;
	case HILL_FORT_WINDOW:
		{
			//displays Hill fort window
			const CGObjectInstance *obj = cl->getObj(id1);
			const CGHeroInstance *hero = cl->getHero(id2);
			INTERFACE_CALL_IF_PRESENT(cl->getTile(obj->visitablePos())->visitableObjects.back()->tempOwner, showHillFortWindow, obj, hero);
		}
		break;
	case PUZZLE_MAP:
		{
			INTERFACE_CALL_IF_PRESENT(id1, showPuzzleMap);
		}
		break;
	case TAVERN_WINDOW:
		const CGObjectInstance *obj1 = cl->getObj(id1),
								*obj2 = cl->getObj(id2);
		INTERFACE_CALL_IF_PRESENT(obj1->tempOwner, showTavernWindow, obj2);
		break;
	}

}

void CenterView::applyCl(CClient *cl)
{
	INTERFACE_CALL_IF_PRESENT (player, centerView, pos, focusTime);
}

void NewObject::applyCl(CClient *cl)
{
	cl->updatePaths();

	const CGObjectInstance *obj = cl->getObj(id);
	//notify interfaces about move
	for(std::map<ui8, CGameInterface*>::iterator i=cl->playerint.begin();i!=cl->playerint.end();i++)
	{
		//TODO: check if any covered tile is visible
		if(i->first >= PLAYER_LIMIT) continue;
		if(GS(cl)->getPlayerTeam(i->first)->fogOfWarMap[obj->pos.x][obj->pos.y][obj->pos.z])
		{
			i->second->newObject(obj);
		}
	}
}

void SetAvailableArtifacts::applyCl(CClient *cl)
{
	if(id < 0) //artifact merchants globally
	{
		for(std::map<ui8, CGameInterface*>::iterator i=cl->playerint.begin();i!=cl->playerint.end();i++)
			i->second->availableArtifactsChanged(NULL);
	}
	else
	{
		const CGBlackMarket *bm = dynamic_cast<const CGBlackMarket *>(cl->getObj(id));
		assert(bm);
		INTERFACE_CALL_IF_PRESENT(cl->getTile(bm->visitablePos())->visitableObjects.back()->tempOwner, availableArtifactsChanged, bm);
	}
}

void TradeComponents::applyCl(CClient *cl)
{///Shop handler
	switch (CGI->mh->map->objects[objectid]->ID)
	{
		case 7: //Black Market
			break;
		case 95: //Tavern
			break;
		case 97: //Den of Thieves
			break;
		case 221: //Trading Post
			break;
		default: 
			tlog2 << "Shop type not supported! \n";
	}
}
