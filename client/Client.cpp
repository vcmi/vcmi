#include "../CCallback.h"
#include "../CConsoleHandler.h"
#include "../CGameInfo.h"
#include "../CGameState.h"
#include "../CPlayerInterface.h"
#include "../StartInfo.h"
#include "../hch/CArtHandler.h"
#include "../hch/CDefObjInfoHandler.h"
#include "../hch/CGeneralTextHandler.h"
#include "../hch/CHeroHandler.h"
#include "../hch/CTownHandler.h"
#include "../hch/CObjectHandler.h"
#include "../hch/CBuildingHandler.h"
#include "../hch/CSpellHandler.h"
#include "../lib/Connection.h"
#include "../lib/Interprocess.h"
#include "../lib/NetPacks.h"
#include "../lib/VCMI_Lib.h"
#include "../map.h"
#include "../mapHandler.h"
#include "CConfigHandler.h"
#include "Client.h"
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <sstream>
CSharedCond<std::set<IPack*> > mess(new std::set<IPack*>);
extern std::string NAME;
namespace intpr = boost::interprocess;

void CClient::init()
{
	IObjectInterface::cb = this;
	serv = NULL;
	gs = NULL;
	cb = NULL;
	try
	{
		shared = new SharedMem();
	} HANDLE_EXCEPTION
}

CClient::CClient(void)
{
	init();
}
CClient::CClient(CConnection *con, StartInfo *si)
{
	init();
	newGame(con,si);
}
CClient::~CClient(void)
{
	delete shared;
}
void CClient::process(int what)
{
	static BattleAction curbaction;
	switch (what)
	{
	case 95: //system message
		{
			std::string m;
			*serv >> m;
			tlog4 << "System message from server: " << m << std::endl;
			break;
		}
	case 100: //one of our interfaces has turn
		{
			ui8 player;
			*serv >> player;//who?
			tlog5 << "It's turn of "<<(unsigned)player<<" player."<<std::endl;
			gs->currentPlayer = player;
			boost::thread(boost::bind(&CGameInterface::yourTurn,playerint[player]));
			break;
		}
	case 101:
		{
			NewTurn n;
			*serv >> n;
			tlog5 << "New day: "<<(unsigned)n.day<<". Applying changes... ";
			gs->apply(&n);
			tlog5 << "done!"<<std::endl;
			break;
		}
	case 102: //set resource amount
		{
			SetResource sr;
			*serv >> sr;
			tlog5 << "Set amount of "<<CGI->generaltexth->restypes[sr.resid] 
			  << " of player "<<(unsigned)sr.player <<" to "<<sr.val<<std::endl;
			gs->apply(&sr);
			playerint[sr.player]->receivedResource(sr.resid,sr.val);
			break;
		}
	case 103: //show info dialog
		{
			InfoWindow iw;
			*serv >> iw;
			std::vector<Component*> comps;
			for(size_t i=0;i<iw.components.size();i++) {
				comps.push_back(&iw.components[i]);
                        }
			std::string str = toString(iw.text);
			playerint[iw.player]->showInfoDialog(str,comps);
			break;
		}
	case 104:
		{
			SetResources sr;
			*serv >> sr;
			tlog5 << "Set amount of resources of player "<<(unsigned)sr.player<<std::endl;
			gs->apply(&sr);
			playerint[sr.player]->receivedResource(-1,-1);
			break;
		}
	case 105:
		{
			SetPrimSkill sps;
			*serv >> sps;
			tlog5 << "Changing hero primary skill"<<std::endl;
			gs->apply(&sps);
			playerint[gs->getHero(sps.id)->tempOwner]->heroPrimarySkillChanged(gs->getHero(sps.id),sps.which,sps.val);
			break;
		}
	case 106:
		{
			SetSecSkill sr;
			*serv >> sr;
			tlog5 << "Changing hero secondary skill"<<std::endl;
			gs->apply(&sr);
			//TODO? - maybe inform interfaces
			break;
		}
	case 107:
		{
			ShowInInfobox sii;
			*serv >> sii;
			SComponent sc(sii.c);
			sc.description = toString(sii.text);
			if(playerint[sii.player]->human)
				static_cast<CPlayerInterface*>(playerint[sii.player])->showComp(sc);
			break;
		}
	case 108:
		{
			HeroVisitCastle vc;
			*serv >> vc;
			gs->apply(&vc);
			if(vc.start() && !vc.garrison() && vstd::contains(playerint,gs->getHero(vc.hid)->tempOwner))
			{
				playerint[gs->getHero(vc.hid)->tempOwner]->heroVisitsTown(gs->getHero(vc.hid),gs->getTown(vc.tid));
			}
			break;
		}
	case 109:
		{
			ChangeSpells vc;
			*serv >> vc;
			tlog5 << "Changing spells of hero "<<vc.hid<<std::endl;
			gs->apply(&vc);
			break;
		}
	case 110:
		{
			SetMana sm;
			*serv >> sm;
			tlog5 << "Setting mana value of hero "<<sm.hid<<" to "<<sm.val<<std::endl;
			gs->apply(&sm);
			CGHeroInstance *h = gs->getHero(sm.hid);
			if(vstd::contains(playerint,h->tempOwner))
				playerint[h->tempOwner]->heroManaPointsChanged(h);
			break;
		}
	case 111:
		{
			SetMovePoints smp;
			*serv >> smp;
			tlog5 << "Setting movement points of hero "<<smp.hid<<" to "<<smp.val<<std::endl;
			gs->apply(&smp);
			CGHeroInstance *h = gs->getHero(smp.hid);
			if(vstd::contains(playerint,h->tempOwner))
				playerint[h->tempOwner]->heroMovePointsChanged(h);
			break;
		}
	case 112:
		{
			FoWChange fc;
			*serv >> fc;
			tlog5 << "Changing FoW of player "<<(int)fc.player<<std::endl;
			gs->apply(&fc);
			if(!vstd::contains(playerint,fc.player))
				break;
			if(fc.mode)
				playerint[fc.player]->tileRevealed(fc.tiles);
			else
				playerint[fc.player]->tileHidden(fc.tiles);
			break;
		}
	case 113:
		{
			SetAvailableHeroes sav;
			*serv >> sav;
			tlog5 << "Setting available heroes for player "<<(int)sav.player<<std::endl;
			gs->apply(&sav);
			break;
		}
	case 500:
		{
			RemoveObject rh;
			*serv >> rh;
			CGObjectInstance *obj = gs->map->objects[rh.id];
			CGI->mh->removeObject(obj);
			gs->apply(&rh);
			if(obj->ID == 34)
			{
				CGHeroInstance *h = static_cast<CGHeroInstance*>(obj);
				tlog5 << "Removing hero with id = "<<(unsigned)rh.id<<std::endl;
				playerint[h->tempOwner]->heroKilled(h);
			}
			break;
		}
	case 501: //hero movement response - we have to notify interfaces and callback
		{
			TryMoveHero *th = new TryMoveHero; //will be deleted by callback after processing
			*serv >> *th;
			tlog5 << "HeroMove: id="<<th->id<<"\tResult: "<<(unsigned)th->result<<"\tPosition "<<th->end<<std::endl;

			HeroMoveDetails hmd(th->start,th->end,static_cast<CGHeroInstance*>(gs->map->objects[th->id]));
			hmd.style = th->result-1;
			hmd.successful = th->result;
			if(th->result>1)
				CGI->mh->removeObject(hmd.ho);

			gs->apply(th);
			
			if(th->result>1)
				CGI->mh->printObject(hmd.ho);
			int player = gs->map->objects[th->id]->getOwner();

			if(playerint[player])
			{
				playerint[player]->tileRevealed(th->fowRevealed);
				//std::for_each(th->fowRevealed.begin(),th->fowRevealed.end(),boost::bind(&CGameInterface::tileRevealed,playerint[player],_1));
			}

			//notify interfaces about move
			for(std::map<ui8, CGameInterface*>::iterator i=playerint.begin();i!=playerint.end();i++)
			{
				if(i->first >= PLAYER_LIMIT) continue;
				if(gs->players[i->first].fogOfWarMap[th->start.x-1][th->start.y][th->start.z] || gs->players[i->first].fogOfWarMap[th->end.x-1][th->end.y][th->end.z])
				{
					i->second->heroMoved(hmd);
				}
			}

			//add info for callback
			if(th->result<2)
			{
				mess.mx->lock();
				mess.res->insert(th);
				mess.mx->unlock();
				mess.cv->notify_all();
			}
			break;
		}
	case 502:
		{
			SetGarrisons sg;
			*serv >> sg;
			tlog5 << "Setting garrisons." << std::endl;
			gs->apply(&sg);
			for(std::map<ui32,CCreatureSet>::iterator i = sg.garrs.begin(); i!=sg.garrs.end(); i++)
				playerint[gs->map->objects[i->first]->tempOwner]->garrisonChanged(gs->map->objects[i->first]);
			break;
		}
	case 503:
		{
			//SetStrInfo ssi;
			//*serv >> ssi;
			//gs->apply(&ssi);
			//TODO: notify interfaces
			break;
		}
	case 504:
		{
			NewStructures ns;
			*serv >> ns;
			CGTownInstance *town = static_cast<CGTownInstance*>(gs->map->objects[ns.tid]);
			tlog5 << "New structure(s) in " << ns.tid <<" " << town->name << " - " << *ns.bid.begin() << std::endl;
			gs->apply(&ns);
			BOOST_FOREACH(si32 bid, ns.bid)
			{
				if(bid==13) //for or capitol
				{
					town->defInfo = gs->capitols[town->subID];
				}
				if(bid ==7)
				{
					town->defInfo = gs->forts[town->subID];
				}
				playerint[town->tempOwner]->buildChanged(town,bid,1);
			}
			break;
		}
	case 506:
		{
			SetAvailableCreatures ns;
			*serv >> ns;
			tlog5 << "Setting available creatures in " << ns.tid << std::endl;
			gs->apply(&ns);
			CGTownInstance *t = gs->getTown(ns.tid);
			if(vstd::contains(playerint,t->tempOwner))
				playerint[t->tempOwner]->availableCreaturesChanged(t);
			break;
		}
	case 508:
		{
			SetHeroesInTown inTown;
			*serv >> inTown;
			tlog5 << "Setting heroes in town " << inTown.tid << std::endl;
			gs->apply(&inTown);
			CGTownInstance *t = gs->getTown(inTown.tid);
			if(vstd::contains(playerint,t->tempOwner))
				playerint[t->tempOwner]->heroInGarrisonChange(t);
			break;
		}
	case 509:
		{
			SetHeroArtifacts sha;
			*serv >> sha;
			tlog5 << "Setting artifacts of hero " << sha.hid << std::endl;
			gs->apply(&sha);
			CGHeroInstance *t = gs->getHero(sha.hid);
			if(vstd::contains(playerint,t->tempOwner))
				playerint[t->tempOwner]->heroArtifactSetChanged(t);
			break;
		}
	case 513:
		{
			ui8 color;
			std::string message;
			*serv >> color >> message;
			tlog4 << "Player "<<(int)color<<" sends a message: " << message << std::endl;
			break;
		}
	case 514:
		{
			SetSelection ss;
			*serv >> ss;
			tlog5 << "Selection of player " << (int)ss.player << " set to " << ss.id << std::endl;
			gs->apply(&ss);
			break;
		}
	case 515:
		{
			HeroRecruited hr;
			*serv >> hr;
			tlog5 << "New hero bought\n";
			CGHeroInstance *h = gs->hpool.heroesPool[hr.hid];
			gs->apply(&hr);
			CGI->mh->initHeroDef(h);
			//CGI->mh->printObject(h);
			playerint[h->tempOwner]->heroCreated(h);
			playerint[h->tempOwner]->heroInGarrisonChange(gs->getTown(hr.tid));
			break;
		}
	case 1001:
		{
			SetObjectProperty sop;
			*serv >> sop;
			tlog5 << "Setting " << (unsigned)sop.what << " property of " << sop.id <<" object to "<<sop.val<<std::endl;
			gs->apply(&sop);
			break;
		}
	case 1002:
		{
			SetHoverName shn;
			*serv >> shn;
			tlog5 << "Setting a name of " << shn.id <<" object to "<< toString(shn.name) <<std::endl;
			gs->apply(&shn);
			break;
		}
	case 2000:
		{
			HeroLevelUp  bs;
			*serv >> bs;
			tlog5 << "Hero levels up!" <<std::endl;
			gs->apply(&bs);
			CGHeroInstance *h = gs->getHero(bs.heroid);
			if(vstd::contains(playerint,h->tempOwner))
			{
				boost::function<void(ui32)> callback = boost::function<void(ui32)>(boost::bind(&CCallback::selectionMade,LOCPLINT->cb,_1,bs.id));
				playerint[h->tempOwner]->heroGotLevel((const CGHeroInstance *)h,(int)bs.primskill,bs.skills, callback);
			}
			break;
		}
	case 2001:
		{
			SelectionDialog sd;
			*serv >> sd;
			tlog5 << "Showing selection dialog " <<std::endl;
			std::vector<Component*> comps;
			for(size_t i=0; i < sd.components.size(); ++i) {
				comps.push_back(&sd.components[i]);
                        }
			std::string str = toString(sd.text);
			playerint[sd.player]->showSelDialog(str,comps,sd.id);
			break;
		}
	case 2002:
		{
			YesNoDialog ynd;
			*serv >> ynd;
			tlog5 << "Showing yes/no dialog " <<std::endl;
			std::vector<Component*> comps;
			for(size_t i=0; i < ynd.components.size(); ++i) {
				comps.push_back(&ynd.components[i]);
                        }
			std::string str = toString(ynd.text);
			playerint[ynd.player]->showYesNoDialog(str,comps,ynd.id);
			break;
		}
	case 3000:
		{
			BattleStart bs;
			*serv >> bs; //uses new to allocate memory for battleInfo - must be deleted when battle is over
			tlog5 << "Starting battle!" <<std::endl;
			gs->apply(&bs);

			if(playerint.find(gs->curB->side1) != playerint.end())
				playerint[gs->curB->side1]->battleStart(&gs->curB->army1, &gs->curB->army2, gs->curB->tile, gs->getHero(gs->curB->hero1), gs->getHero(gs->curB->hero2), 0);
			if(playerint.find(gs->curB->side2) != playerint.end())
				playerint[gs->curB->side2]->battleStart(&gs->curB->army1, &gs->curB->army2, gs->curB->tile, gs->getHero(gs->curB->hero1), gs->getHero(gs->curB->hero2), 1);
			
			break;
		}
	case 3001:
		{
			BattleNextRound bnr;
			*serv >> bnr;
			tlog5 << "Round nr " << bnr.round <<std::endl;
			gs->apply(&bnr);

			//tell players about next round
			if(playerint.find(gs->curB->side1) != playerint.end())
				playerint[gs->curB->side1]->battleNewRound(bnr.round);
			if(playerint.find(gs->curB->side2) != playerint.end())
				playerint[gs->curB->side2]->battleNewRound(bnr.round);
			break;
		}
	case 3002:
		{
			BattleSetActiveStack sas;
			*serv >> sas;
			tlog5 << "Active stack: " << sas.stack <<std::endl;
			gs->apply(&sas);
			int owner = gs->curB->getStack(sas.stack)->owner;
			boost::thread(boost::bind(&CClient::waitForMoveAndSend,this,owner));
			break;
		}
	case 3003:
		{
			BattleResult br;
			*serv >> br;
			tlog5 << "Battle ends. Winner: " << (unsigned)br.winner<< ". Type of end: "<< (unsigned)br.result <<std::endl;

			if(playerint.find(gs->curB->side1) != playerint.end())
				playerint[gs->curB->side1]->battleEnd(&br);
			if(playerint.find(gs->curB->side2) != playerint.end())
				playerint[gs->curB->side2]->battleEnd(&br);

			gs->apply(&br);
			break;
		}
	case 3004:
		{
			BattleStackMoved br;
			*serv >> br;
			tlog5 << "Stack "<<br.stack <<" moves to the tile "<<br.tile<<std::endl;
			if(playerint.find(gs->curB->side1) != playerint.end())
				playerint[gs->curB->side1]->battleStackMoved(br.stack,br.tile);
			if(playerint.find(gs->curB->side2) != playerint.end())
				playerint[gs->curB->side2]->battleStackMoved(br.stack,br.tile);
			gs->apply(&br);
			break;
		}
	case 3005:
		{
			BattleStackAttacked bsa;
			*serv >> bsa;
			gs->apply(&bsa);
			if(playerint.find(gs->curB->side1) != playerint.end())
				playerint[gs->curB->side1]->battleStackAttacked(&bsa);
			if(playerint.find(gs->curB->side2) != playerint.end())
				playerint[gs->curB->side2]->battleStackAttacked(&bsa);
			break;
		}
	case 3006:
		{
			BattleAttack ba;
			*serv >> ba;
			tlog5 << "Stack: " << ba.stackAttacking << " is attacking stack "<< ba.bsa.stackAttacked <<std::endl;
			if(playerint.find(gs->curB->side1) != playerint.end())
				playerint[gs->curB->side1]->battleAttack(&ba);
			if(playerint.find(gs->curB->side2) != playerint.end())
				playerint[gs->curB->side2]->battleAttack(&ba);
			gs->apply(&ba);
			if(playerint.find(gs->curB->side1) != playerint.end())
				playerint[gs->curB->side1]->battleStackAttacked(&ba.bsa);
			if(playerint.find(gs->curB->side2) != playerint.end())
				playerint[gs->curB->side2]->battleStackAttacked(&ba.bsa);
			break;
		}
	case 3007:
		{
			*serv >> curbaction;
			tlog5 << "Action started. ID: " << (int)curbaction.actionType << ". Destination: "<< curbaction.destinationTile <<std::endl;
			if(playerint.find(gs->curB->side1) != playerint.end())
				playerint[gs->curB->side1]->actionStarted(&curbaction);
			if(playerint.find(gs->curB->side2) != playerint.end())
				playerint[gs->curB->side2]->actionStarted(&curbaction);
			gs->apply(&StartAction(curbaction));
			break;
		}
	case 3008:
		{
			tlog5 << "Action ended!\n";
			if(!gs->curB)
			{
				tlog2 << "There is no battle state!\n";
				break;
			}
			if(playerint.find(gs->curB->side1) != playerint.end())
				playerint[gs->curB->side1]->actionFinished(&curbaction);
			if(playerint.find(gs->curB->side2) != playerint.end())
				playerint[gs->curB->side2]->actionFinished(&curbaction);
			break;
		}
	case 3009:
		{
			tlog5 << "Spell casted!\n";
			SpellCasted sc;
			*serv >> sc;
			gs->apply(&sc);
			if(playerint.find(gs->curB->side1) != playerint.end())
				playerint[gs->curB->side1]->battleSpellCasted(&sc);
			if(playerint.find(gs->curB->side2) != playerint.end())
				playerint[gs->curB->side2]->battleSpellCasted(&sc);
			break;
		}
	case 3010:
		{
			tlog5 << "Effect set!\n";
			SetStackEffect sse;
			*serv >> sse;
			gs->apply(&sse);
			SpellCasted sc;
			sc.id = sse.effect.id;
			sc.side = 3; //doesn't matter
			sc.skill = sse.effect.level;
			sc.tile = gs->curB->getStack(sse.stack)->position;
			if(playerint.find(gs->curB->side1) != playerint.end())
				playerint[gs->curB->side1]->battleSpellCasted(&sc);
			if(playerint.find(gs->curB->side2) != playerint.end())
				playerint[gs->curB->side2]->battleSpellCasted(&sc);
			break;
		}
	case 9999:
		break;
	default:
		{
			std::ostringstream ex;
			ex << "Not supported server message (type=" << what <<")";
			throw ex.str();
		}
	}
}
void CClient::waitForMoveAndSend(int color)
{
	try
	{
		BattleAction ba = playerint[color]->activeStack(gs->curB->activeStack);
		*serv << ui16(3002) << ba;
		return;
	}HANDLE_EXCEPTION
	tlog1 << "We should not be here!" << std::endl;
}
void CClient::run()
{
	try
	{
		ui16 typ;
		while(1)
		{
			*serv >> typ;
			process(typ);
		}
	} HANDLE_EXCEPTION
}

void CClient::close()
{
	if(!serv)
		return;

	tlog3 << "Connection has been requested to be closed.\n";
	boost::unique_lock<boost::mutex>(*serv->wmx);
	*serv << ui16(99);
	tlog3 << "Sent closing signal to the server\n";
	serv->close();
	tlog3 << "Our socket has been closed.\n";
}

void CClient::save(const std::string & fname)
{
	*serv << ui16(98) << fname;
}

void CClient::load( const std::string & fname )
{
	tlog0 <<"\n\nLoading procedure started!\n\n";

	timeHandler tmh;
	close(); //kill server
	tlog0 <<"Sent kill signal to the server: "<<tmh.getDif()<<std::endl;

	VLC->clear(); //delete old handlers
	delete CGI->mh;
	delete CGI->state;
	//TODO: del callbacks

	for(std::map<ui8,CGameInterface *>::iterator i = playerint.begin(); i!=playerint.end(); i++)
	{
		delete i->second; //delete player interfaces
		}
	tlog0 <<"Deleting old data: "<<tmh.getDif()<<std::endl;

	char portc[10];
	SDL_itoa(conf.cc.port,portc,10);
	runServer(portc); //create new server
	tlog0 <<"Restarting server: "<<tmh.getDif()<<std::endl;

	{
		ui32 ver;
		char sig[8];
		CMapHeader dum;
		CGI->mh = new CMapHandler();

		CLoadFile lf(fname + ".vlgm1");
		lf >> sig >> ver >> dum >> *sig;
		tlog0 <<"Reading save signature: "<<tmh.getDif()<<std::endl;
			
		lf >> *VLC;
		CGI->setFromLib();
		tlog0 <<"Reading handlers: "<<tmh.getDif()<<std::endl;

		lf >> gs;
		tlog0 <<"Reading gamestate: "<<tmh.getDif()<<std::endl;

		CGI->state = gs;
		CGI->mh->map = gs->map;
		CGI->mh->init();
		tlog0 <<"Initing maphandler: "<<tmh.getDif()<<std::endl;
	}

	waitForServer();
	tlog0 <<"Waiting for server: "<<tmh.getDif()<<std::endl;

	serv = new CConnection(conf.cc.server,portc,NAME);
	tlog0 <<"Setting up connection: "<<tmh.getDif()<<std::endl;

	ui8 pom8;
	*serv << ui8(3) << ui8(1); //load game; one client
	*serv << fname;
	*serv >> pom8;
	if(pom8) 
		throw "Server cannot open the savegame!";
	else
		tlog0 << "Server opened savegame properly.\n";

	*serv << ui8(gs->scenarioOps->playerInfos.size()+1); //number of players + neutral
	for(size_t i=0;i<gs->scenarioOps->playerInfos.size();i++) 
	{
		*serv << ui8(gs->scenarioOps->playerInfos[i].color); //players
	}
	*serv << ui8(255); // neutrals
	tlog0 <<"Sent info to server: "<<tmh.getDif()<<std::endl;
	
	for (size_t i=0; i<gs->scenarioOps->playerInfos.size();++i) //initializing interfaces for players
	{ 
		ui8 color = gs->scenarioOps->playerInfos[i].color;
		CCallback *cb = new CCallback(gs,color,this);
		if(!gs->scenarioOps->playerInfos[i].human) {
			playerint[color] = static_cast<CGameInterface*>(CAIHandler::getNewAI(cb,conf.cc.defaultAI));
		}
		else {
			playerint[color] = new CPlayerInterface(color,i);
		}
		gs->currentPlayer = color;
		playerint[color]->init(cb);
		tlog0 <<"Setting up interface for player "<< (int)color <<": "<<tmh.getDif()<<std::endl;
	}
	playerint[255] =  CAIHandler::getNewAI(cb,conf.cc.defaultAI);
	playerint[255]->init(new CCallback(gs,255,this));
	tlog0 <<"Setting up interface for neutral \"player\"" << tmh.getDif() << std::endl;


}

int CClient::getCurrentPlayer()
{
	return gs->currentPlayer;
}

int CClient::getSelectedHero()
{
	return IGameCallback::getSelectedHero(getCurrentPlayer())->id;
}

void CClient::newGame( CConnection *con, StartInfo *si )
{
	timeHandler tmh;
	CGI->state = new CGameState();
	tlog0 <<"\tGamestate: "<<tmh.getDif()<<std::endl;
	serv = con;
	CConnection &c(*con);
	////////////////////////////////////////////////////
	ui8 pom8;
	c << ui8(2) << ui8(1); //new game; one client
	c << *si;
	c >> pom8;
	if(pom8) 
		throw "Server cannot open the map!";
	else
		tlog0 << "Server opened map properly.\n";
	c << ui8(si->playerInfos.size()+1); //number of players + neutral
	for(size_t i=0;i<si->playerInfos.size();i++) 
	{
		c << ui8(si->playerInfos[i].color); //players
	}
	c << ui8(255); // neutrals


	ui32 seed, sum;
	std::string mapname;
	c >> mapname >> sum >> seed;
	tlog0 <<"\tSending/Getting info to/from the server: "<<tmh.getDif()<<std::endl;

	Mapa * mapa = new Mapa(mapname);
	tlog0 <<"Reading and detecting map file (together): "<<tmh.getDif()<<std::endl;
	tlog0 << "\tServer checksum for "<<mapname <<": "<<sum << std::endl;
	tlog0 << "\tOur checksum for the map: "<< mapa->checksum << std::endl;

	if(mapa->checksum != sum)
	{
		tlog1 << "Wrong map checksum!!!" << std::endl;
		throw std::string("Wrong checksum");
	}
	tlog0 << "\tUsing random seed: "<<seed << std::endl;

	gs = CGI->state;
	gs->scenarioOps = si;
	gs->init(si,mapa,seed);

	CGI->mh = new CMapHandler();
	tlog0 <<"Initializing GameState (together): "<<tmh.getDif()<<std::endl;
	CGI->mh->map = mapa;
	tlog0 <<"Creating mapHandler: "<<tmh.getDif()<<std::endl;
	CGI->mh->init();
	tlog0 <<"Initializing mapHandler (together): "<<tmh.getDif()<<std::endl;

	for (size_t i=0; i<CGI->state->scenarioOps->playerInfos.size();++i) //initializing interfaces for players
	{ 
		ui8 color = gs->scenarioOps->playerInfos[i].color;
		CCallback *cb = new CCallback(gs,color,this);
		if(!gs->scenarioOps->playerInfos[i].human) {
			playerint[color] = static_cast<CGameInterface*>(CAIHandler::getNewAI(cb,conf.cc.defaultAI));
		}
		else {
			playerint[color] = new CPlayerInterface(color,i);
		}
		gs->currentPlayer = color;
		playerint[color]->init(cb);
	}
	playerint[255] =  CAIHandler::getNewAI(cb,conf.cc.defaultAI);
	playerint[255]->init(new CCallback(gs,255,this));
}

void CClient::runServer(const char * portc)
{
	static std::string comm = std::string(SERVER_NAME) + " " + portc + " > server_log.txt"; //needs to be static, if not - will be probably destroyed before new thread reads it
	boost::thread servthr(boost::bind(system,comm.c_str())); //runs server executable; 	//TODO: will it work on non-windows platforms?
}

void CClient::waitForServer()
{
	intpr::scoped_lock<intpr::interprocess_mutex> slock(shared->sr->mutex);
	while(!shared->sr->ready)
	{
		shared->sr->cond.wait(slock);
	}
}
