#include "../lib/NetPacks.h"
#include "../CCallback.h"
#include "Client.h"
#include "CPlayerInterface.h"
#include "CGameInfo.h"
#include "../lib/Connection.h"
#include "../hch/CGeneralTextHandler.h"
#include "../hch/CDefObjInfoHandler.h"
#include "../hch/CHeroHandler.h"
#include "../hch/CObjectHandler.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/map.h"
#include "../hch/CSpellHandler.h"
#include "../hch/CSoundBase.h"
#include "../mapHandler.h"
#include "GUIClasses.h"
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>

//macro to avoid code duplication - calls given method with given arguments if interface for specific player is present
#define INTERFACE_CALL_IF_PRESENT(player,function,...) 	\
		if(vstd::contains(cl->playerint,player))		\
			cl->playerint[player]->function(__VA_ARGS__);

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
	cl->playerint[player]->receivedResource(-1,-1);
}

void SetResource::applyCl( CClient *cl )
{
	cl->playerint[player]->receivedResource(resid,val);
}

void SetPrimSkill::applyCl( CClient *cl )
{
	const CGHeroInstance *h = GS(cl)->getHero(id);
	if(!h)
	{
		tlog1 << "Cannot find hero with ID " << id << std::endl;
		return;
	}
	INTERFACE_CALL_IF_PRESENT(h->tempOwner,heroPrimarySkillChanged,h,which,val);
}

void SetSecSkill::applyCl( CClient *cl )
{
	//TODO: inform interface?
}

void HeroVisitCastle::applyCl( CClient *cl )
{
	if(start() && !garrison() && vstd::contains(cl->playerint,GS(cl)->getHero(hid)->tempOwner))
	{
		cl->playerint[GS(cl)->getHero(hid)->tempOwner]->heroVisitsTown(GS(cl)->getHero(hid),GS(cl)->getTown(tid));
	}
}

void ChangeSpells::applyCl( CClient *cl )
{
	//TODO: inform interface?
}

void SetMana::applyCl( CClient *cl )
{
	CGHeroInstance *h = GS(cl)->getHero(hid);
	if(vstd::contains(cl->playerint,h->tempOwner))
		cl->playerint[h->tempOwner]->heroManaPointsChanged(h);
}

void SetMovePoints::applyCl( CClient *cl )
{
	CGHeroInstance *h = GS(cl)->getHero(hid);
	if(vstd::contains(cl->playerint,h->tempOwner))
		cl->playerint[h->tempOwner]->heroMovePointsChanged(h);
}

void FoWChange::applyCl( CClient *cl )
{
	if(!vstd::contains(cl->playerint,player))
		return;

	if(mode)
		cl->playerint[player]->tileRevealed(tiles);
	else
		cl->playerint[player]->tileHidden(tiles);
}

void SetAvailableHeroes::applyCl( CClient *cl )
{
	//TODO: inform interface?
}

void GiveBonus::applyCl( CClient *cl )
{
	CGHeroInstance *h = GS(cl)->getHero(hid);
	if(vstd::contains(cl->playerint,h->tempOwner))
		cl->playerint[h->tempOwner]->heroBonusChanged(h,h->bonuses.back(),true);
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
}

void RemoveObject::applyFirstCl( CClient *cl )
{
	CGI->mh->hideObject(cl->getObj(id));
	CGHeroInstance *h = GS(cl)->getHero(id);
	if(h)
	{
		if(vstd::contains(cl->playerint,h->tempOwner))
			cl->playerint[h->tempOwner]->heroKilled(h);
	}
}

void RemoveObject::applyCl( CClient *cl )
{
}

void TryMoveHero::applyFirstCl( CClient *cl )
{
	if(result>1)
		CGI->mh->removeObject(GS(cl)->getHero(id));
}

void TryMoveHero::applyCl( CClient *cl )
{
	HeroMoveDetails hmd(start,end,GS(cl)->getHero(id));
	hmd.style = result-1;
	hmd.successful = result;

	if(result>1)
		CGI->mh->printObject(hmd.ho);
	int player = hmd.ho->tempOwner;

	if(vstd::contains(cl->playerint,player))
	{
		cl->playerint[player]->tileRevealed(fowRevealed);
	}

	//notify interfaces about move
	for(std::map<ui8, CGameInterface*>::iterator i=cl->playerint.begin();i!=cl->playerint.end();i++)
	{
		if(i->first >= PLAYER_LIMIT) continue;
		if(GS(cl)->players[i->first].fogOfWarMap[start.x-1][start.y][start.z] || GS(cl)->players[i->first].fogOfWarMap[end.x-1][end.y][end.z])
		{
			i->second->heroMoved(hmd);
		}
	}
}

void SetGarrisons::applyCl( CClient *cl )
{
	for(std::map<ui32,CCreatureSet>::iterator i = garrs.begin(); i!=garrs.end(); i++)
		if(vstd::contains(cl->playerint,cl->getOwner(i->first)))
			cl->playerint[cl->getOwner(i->first)]->garrisonChanged(cl->getObj(i->first));
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

void SetAvailableCreatures::applyCl( CClient *cl )
{
	CGTownInstance *t = GS(cl)->getTown(tid);
	if(vstd::contains(cl->playerint,t->tempOwner))
		cl->playerint[t->tempOwner]->availableCreaturesChanged(t);
}

void SetHeroesInTown::applyCl( CClient *cl )
{
	CGTownInstance *t = GS(cl)->getTown(tid);
	if(vstd::contains(cl->playerint,t->tempOwner))
		cl->playerint[t->tempOwner]->heroInGarrisonChange(t);
}

void SetHeroArtifacts::applyCl( CClient *cl )
{
	CGHeroInstance *h = GS(cl)->getHero(hid);
	CGameInterface *player = (vstd::contains(cl->playerint,h->tempOwner) ? cl->playerint[h->tempOwner] : NULL);
	if(!player)
		return;

	player->heroArtifactSetChanged(h);

	BOOST_FOREACH(HeroBonus *bonus, gained)
	{
		player->heroBonusChanged(h,*bonus,true);
	}
	BOOST_FOREACH(HeroBonus *bonus, lost)
	{
		player->heroBonusChanged(h,*bonus,false);
	}
}

void HeroRecruited::applyCl( CClient *cl )
{
	CGHeroInstance *h = GS(cl)->map->heroes.back();
	if(h->subID != hid)
	{
		tlog1 << "Something wrong with hero recruited!\n";
	}

	CGI->mh->initHeroDef(h);
	if(vstd::contains(cl->playerint,h->tempOwner))
	{
		cl->playerint[h->tempOwner]->heroCreated(h);
		cl->playerint[h->tempOwner]->heroInGarrisonChange(GS(cl)->getTown(tid));
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
	std::string str = toString(text);

	if(vstd::contains(cl->playerint,player))
		cl->playerint[player]->showInfoDialog(str,comps,(soundBase::soundID)soundID);
	else
		tlog2 << "We received InfoWindow for not our player...\n";
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
	std::string str = toString(text);
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
	cl->playerint[h->getOwner()]->showGarrisonDialog(obj,h,callback);
}

void BattleStart::applyCl( CClient *cl )
{
	if(vstd::contains(cl->playerint,info->side1))
		cl->playerint[info->side1]->battleStart(&info->army1, &info->army2, info->tile, GS(cl)->getHero(info->hero1), GS(cl)->getHero(info->hero2), 0);

	if(vstd::contains(cl->playerint,info->side2))
		cl->playerint[info->side2]->battleStart(&info->army1, &info->army2, info->tile, GS(cl)->getHero(info->hero1), GS(cl)->getHero(info->hero2), 1);
}

void BattleNextRound::applyCl( CClient *cl )
{
	if(cl->playerint.find(GS(cl)->curB->side1) != cl->playerint.end())
		cl->playerint[GS(cl)->curB->side1]->battleNewRound(round);
	if(cl->playerint.find(GS(cl)->curB->side2) != cl->playerint.end())
		cl->playerint[GS(cl)->curB->side2]->battleNewRound(round);
}

void BattleSetActiveStack::applyCl( CClient *cl )
{
	int owner = GS(cl)->curB->getStack(stack)->owner;
	if(vstd::contains(cl->playerint,owner))
		boost::thread(boost::bind(&CClient::waitForMoveAndSend,cl,owner));
}

void BattleResult::applyFirstCl( CClient *cl )
{
	if(cl->playerint.find(GS(cl)->curB->side1) != cl->playerint.end())
		cl->playerint[GS(cl)->curB->side1]->battleEnd(this);
	if(cl->playerint.find(GS(cl)->curB->side2) != cl->playerint.end())
		cl->playerint[GS(cl)->curB->side2]->battleEnd(this);
}

void BattleStackMoved::applyFirstCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(GS(cl)->curB->side1,battleStackMoved,stack,tile,distance,ending);
	INTERFACE_CALL_IF_PRESENT(GS(cl)->curB->side2,battleStackMoved,stack,tile,distance,ending);
}

void BattleStackAttacked::applyCl( CClient *cl )
{
	std::set<BattleStackAttacked> bsa;
	bsa.insert(*this);

	INTERFACE_CALL_IF_PRESENT(GS(cl)->curB->side1,battleStacksAttacked,bsa);
	INTERFACE_CALL_IF_PRESENT(GS(cl)->curB->side2,battleStacksAttacked,bsa);
}

void BattleAttack::applyFirstCl( CClient *cl )
{
	if(cl->playerint.find(GS(cl)->curB->side1) != cl->playerint.end())
		cl->playerint[GS(cl)->curB->side1]->battleAttack(this);
	if(cl->playerint.find(GS(cl)->curB->side2) != cl->playerint.end())
		cl->playerint[GS(cl)->curB->side2]->battleAttack(this);
}

void BattleAttack::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(GS(cl)->curB->side1,battleStacksAttacked,bsa);
	INTERFACE_CALL_IF_PRESENT(GS(cl)->curB->side2,battleStacksAttacked,bsa);
}

void StartAction::applyFirstCl( CClient *cl )
{
	cl->curbaction = new BattleAction(ba);
	if(cl->playerint.find(GS(cl)->curB->side1) != cl->playerint.end())
		cl->playerint[GS(cl)->curB->side1]->actionStarted(&ba);
	if(cl->playerint.find(GS(cl)->curB->side2) != cl->playerint.end())
		cl->playerint[GS(cl)->curB->side2]->actionStarted(&ba);
}

void SpellCast::applyCl( CClient *cl )
{
	if(cl->playerint.find(GS(cl)->curB->side1) != cl->playerint.end())
		cl->playerint[GS(cl)->curB->side1]->battleSpellCast(this);
	if(cl->playerint.find(GS(cl)->curB->side2) != cl->playerint.end())
		cl->playerint[GS(cl)->curB->side2]->battleSpellCast(this);
}

void SetStackEffect::applyCl( CClient *cl )
{
	SpellCast sc;
	sc.id = effect.id;
	sc.side = 3; //doesn't matter
	sc.skill = effect.level;

	//informing about effects
	if(cl->playerint.find(GS(cl)->curB->side1) != cl->playerint.end())
		cl->playerint[GS(cl)->curB->side1]->battleStacksEffectsSet(*this);
	if(cl->playerint.find(GS(cl)->curB->side2) != cl->playerint.end())
		cl->playerint[GS(cl)->curB->side2]->battleStacksEffectsSet(*this);
}

void StacksInjured::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(GS(cl)->curB->side1,battleStacksAttacked,stacks);
	INTERFACE_CALL_IF_PRESENT(GS(cl)->curB->side2,battleStacksAttacked,stacks);
}

CGameState* CPackForClient::GS( CClient *cl )
{
	return cl->gs;
}

void EndAction::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(GS(cl)->curB->side1,actionFinished,cl->curbaction);
	INTERFACE_CALL_IF_PRESENT(GS(cl)->curB->side2,actionFinished,cl->curbaction);

	delete cl->curbaction;
	cl->curbaction = NULL;
}

void PackageApplied::applyCl( CClient *cl )
{
	INTERFACE_CALL_IF_PRESENT(GS(cl)->currentPlayer,requestRealized,this);
}

void SystemMessage::applyCl( CClient *cl )
{
	std::ostringstream str;
	str << "System message: " << text;

	tlog4 << str.str() << std::endl;
	if(LOCPLINT)
		LOCPLINT->cingconsole->print(str.str());
}

void YourTurn::applyCl( CClient *cl )
{
	boost::thread(boost::bind(&CGameInterface::yourTurn,cl->playerint[player]));
}

void SaveGame::applyCl(CClient *cl)
{
	CSaveFile save("Games" PATHSEPARATOR + fname + ".vcgm1");
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

void ShowInInfobox::applyCl(CClient *cl)
{
	SComponent sc(c);
	sc.description = toString(text);
	if(cl->playerint[player]->human)
	{
		static_cast<CPlayerInterface*>(cl->playerint[player])->showComp(sc);
	}
}
