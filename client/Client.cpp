#include "Client.h"
#include "../lib/Connection.h"
#include "../StartInfo.h"
#include "../map.h"
#include "../CGameState.h"
#include "../CGameInfo.h"
#include "../mapHandler.h"
#include "../CCallback.h"
#include "../CPlayerInterface.h"
#include "../CConsoleHandler.h"
#include "../lib/NetPacks.h"
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/foreach.hpp>
#include "../hch/CObjectHandler.h"
#include "../hch/CGeneralTextHandler.h"
#include "../hch/CArtHandler.h"
#include <boost/thread/shared_mutex.hpp>
CSharedCond<std::set<IPack*> > mess(new std::set<IPack*>);

std::string toString(MetaString &ms)
{
	std::string ret;
	for(int i=0;i<ms.message.size();i++)
	{
		if(ms.message[i]>0)
		{
			ret += ms.strings[ms.message[i]-1];
		}
		else
		{
			std::vector<std::string> *vec;
			int type = ms.texts[-ms.message[i]-1].first,
				ser = ms.texts[-ms.message[i]-1].second;
			if(type == 5)
			{
				ret += CGI->arth->artifacts[ser].name;
				continue;
			}
			else if(type == 7)
			{
				ret += CGI->creh->creatures[ser].namePl;
				continue;
			}
			else if(type == 9)
			{
				ret += CGI->objh->mines[ser].first;
				continue;
			}
			else if(type == 10)
			{
				ret += CGI->objh->mines[ser].second;
				continue;
			}
			else
			{
				switch(type)
				{
				case 1:
					vec = &CGI->generaltexth->allTexts;
					break;
				case 2:
					vec = &CGI->objh->xtrainfo;
					break;
				case 3:
					vec = &CGI->objh->names;
					break;
				case 4:
					vec = &CGI->objh->restypes;
					break;
				case 6:
					vec = &CGI->generaltexth->arraytxt;
					break;
				case 8:
					vec = &CGI->objh->creGens;
					break;
				case 11:
					vec = &CGI->objh->advobtxt;
				}
				ret += (*vec)[ser];
			}
		}
	}
	return ret;
}

CClient::CClient(void)
{
}
CClient::CClient(CConnection *con, StartInfo *si)
	:serv(con)
{		
	timeHandler tmh;
	CGI->state = new CGameState();
	THC std::cout<<"\tGamestate: "<<tmh.getDif()<<std::endl;
	CConnection &c(*con);
////////////////////////////////////////////////////
	ui8 pom8;
	c << ui8(2) << ui8(1); //new game; one client
	c << *si;
	c >> pom8;
	if(pom8) throw "Server cannot open the map!";
	c << ui8(si->playerInfos.size());
	for(int i=0;i<si->playerInfos.size();i++)
		c << ui8(si->playerInfos[i].color);

	ui32 seed, sum;
	std::string mapname;
	c >> mapname >> sum >> seed;
	THC std::cout<<"\tSending/Getting info to/from the server: "<<tmh.getDif()<<std::endl;

	Mapa * mapa = new Mapa(mapname);
	THC std::cout<<"Reading and detecting map file (together): "<<tmh.getDif()<<std::endl;
	std::cout << "\tServer checksum for "<<mapname <<": "<<sum << std::endl;
	std::cout << "\tOur checksum for the map: "<< mapa->checksum << std::endl;

	if(mapa->checksum != sum)
	{
		throw std::exception("Wrong checksum");
		exit(-1);
	}
	std::cout << "\tUsing random seed: "<<seed << std::endl;

	gs = CGI->state;
	gs->scenarioOps = si;
	gs->init(si,mapa,seed);

	CGI->mh = new CMapHandler();
	THC std::cout<<"Initializing GameState (together): "<<tmh.getDif()<<std::endl;
	CGI->mh->map = mapa;
	THC std::cout<<"Creating mapHandler: "<<tmh.getDif()<<std::endl;
	CGI->mh->init();
	THC std::cout<<"Initializing mapHandler (together): "<<tmh.getDif()<<std::endl;

	for (int i=0; i<CGI->state->scenarioOps->playerInfos.size();i++) //initializing interfaces
	{ 
		ui8 color = gs->scenarioOps->playerInfos[i].color;
		CCallback *cb = new CCallback(gs,color,this);
		if(!gs->scenarioOps->playerInfos[i].human)
			playerint[color] = static_cast<CGameInterface*>(CAIHandler::getNewAI(cb,"EmptyAI.dll"));
		else 
		{
			gs->currentPlayer = color;
			playerint[color] = new CPlayerInterface(color,i);
			playerint[color]->init(cb);
		}
	}
	CGI->consoleh->cb = new CCallback(gs,-1,this);
}
CClient::~CClient(void)
{
}
void CClient::process(int what)
{
	switch (what)
	{
	case 100: //one of our interaces has turn
		{
			ui8 player;
			*serv >> player;//who?
			std::cout << "It's turn of "<<(unsigned)player<<" player."<<std::endl;
			boost::thread(boost::bind(&CGameInterface::yourTurn,playerint[player]));
			break;
		}
	case 101:
		{
			NewTurn n;
			*serv >> n;
			std::cout << "New day: "<<(unsigned)n.day<<". Applying changes... ";
			gs->apply(&n);
			std::cout << "done!"<<std::endl;
			break;
		}
	case 102: //set resource amount
		{
			SetResource sr;
			*serv >> sr;
			std::cout << "Set amount of "<<CGI->objh->restypes[sr.resid] 
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
			for(int i=0;i<iw.components.size();i++)
				comps.push_back(&iw.components[i]);
			playerint[iw.player]->showInfoDialog(toString(iw.text),comps);
			break;
		}
	case 104:
		{
			SetResources sr;
			*serv >> sr;
			std::cout << "Set amount of resources of player "<<(unsigned)sr.player<<std::endl;
			gs->apply(&sr);
			playerint[sr.player]->receivedResource(-1,-1);
			break;
		}
	case 500:
		{
			RemoveHero rh;
			*serv >> rh;
			CGHeroInstance *h = static_cast<CGHeroInstance*>(gs->map->objects[rh.id]);
			std::cout << "Removing hero with id = "<<(unsigned)rh.id<<std::endl;
			CGI->mh->removeObject(h);
			gs->apply(&rh);
			playerint[h->tempOwner]->heroKilled(h);
			break;
		}
	case 501: //hero movement response - we have to notify interfaces and callback
		{
			TryMoveHero *th = new TryMoveHero; //will be deleted by callback after processing
			*serv >> *th;
			std::cout << "HeroMove: id="<<th->id<<"\tResult: "<<(unsigned)th->result<<"\tPosition "<<th->end<<std::endl;

			gs->apply(th);
			int player = gs->map->objects[th->id]->getOwner();

			if(playerint[player])
			{
				for(std::set<int3>::iterator i=th->fowRevealed.begin(); i != th->fowRevealed.end(); i++)
					playerint[player]->tileRevealed(*i);
				//boost::function<void(int3)> tr = boost::bind(&CGameInterface::tileRevealed,playerint[player]);
				//std::for_each(th->fowRevealed.begin(),th->fowRevealed.end(),tr);
			}

			//notify interfacesabout move
			int nn=0; //number of interfece of currently browsed player
			for(std::map<ui8, CGameInterface*>::iterator i=playerint.begin();i!=playerint.end();i++)
			{
				if(gs->players[i->first].fogOfWarMap[th->start.x-1][th->start.y][th->start.z] || gs->players[i->first].fogOfWarMap[th->end.x-1][th->end.y][th->end.z])
				{
					HeroMoveDetails hmd(th->start,th->end,static_cast<CGHeroInstance*>(gs->map->objects[th->id]));
					hmd.successful = th->result;
					i->second->heroMoved(hmd);
				}
			}

			//add info for callback
			mess.mx->lock();
			mess.res->insert(th);
			mess.mx->unlock();
			mess.cv->notify_all();
			break;
		}
	case 502:
		{
			SetGarrisons sg;
			*serv >> sg;
			gs->apply(&sg);
			for(std::map<ui32,CCreatureSet>::iterator i = sg.garrs.begin(); i!=sg.garrs.end(); i++)
				playerint[gs->map->objects[i->first]->tempOwner]->garrisonChanged(gs->map->objects[i->first]);
			break;
		}
	case 503:
		{
			SetStrInfo ssi;
			*serv >> ssi;
			gs->apply(&ssi);
			//TODO: notify interfaces
			break;
		}
	case 504:
		{
			NewStructures ns;
			*serv >> ns;
			gs->apply(&ns);
			BOOST_FOREACH(si32 bid, ns.bid)
				playerint[gs->map->objects[ns.tid]->tempOwner]->buildChanged(static_cast<CGTownInstance*>(gs->map->objects[ns.tid]),bid,1);
			break;
		}
	case 1001:
		{
			SetObjectProperty sop;
			*serv >> sop;
			std::cout << "Setting " << (unsigned)sop.what << " property of " << sop.id <<" object to "<<sop.val<<std::endl;
			gs->apply(&sop);
			break;
		}
	case 1002:
		{
			SetHoverName shn;
			*serv >> shn;
			std::cout << "Setting a name of " << shn.id <<" object to "<< toString(shn.name) <<std::endl;
			gs->mx->lock();
			gs->map->objects[shn.id]->hoverName = toString(shn.name);
			gs->mx->unlock();
			break;
		}
	case 9999:
		break;
	default:
		throw std::exception("Not supported server message!");
		break;
	}
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