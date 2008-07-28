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
#include "../hch/CObjectHandler.h"
CSharedCond<std::set<IPack*> > mess(new std::set<IPack*>);

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
		exit(-1);
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
	case 501: //hero movement response - we have to notify interfaces and callback
		{
			TryMoveHero *th = new TryMoveHero;
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