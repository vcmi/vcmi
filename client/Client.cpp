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

CClient::CClient(void)
{
}
CClient::CClient(CConnection *con, StartInfo *si)
	:serv(con)
{
	timeHandler tmh;
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

		if(!CGI->state->scenarioOps->playerInfos[i].human)
			CGI->playerint.push_back(static_cast<CGameInterface*>(CAIHandler::getNewAI(new CCallback(CGI->state,CGI->state->scenarioOps->playerInfos[i].color),"EmptyAI.dll")));
		else 
		{
			CGI->state->currentPlayer=CGI->state->scenarioOps->playerInfos[i].color;
			CGI->playerint.push_back(new CPlayerInterface(CGI->state->scenarioOps->playerInfos[i].color,i));
			((CPlayerInterface*)(CGI->playerint[i]))->init(new CCallback(CGI->state,CGI->state->scenarioOps->playerInfos[i].color));
		}
	}
	CGI->consoleh->cb = new CCallback(CGI->state,-1);
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
			CGI->playerint[gs->players[player].serial]->yourTurn();
			*serv << ui16(100); //report that we ended turn
			std::cout << "Player "<<(unsigned)player<<" end his turn."<<std::endl;
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
	//while(1) //main game loop, one execution per turn
	//{
	//	CGI->consoleh->cb->newTurn();
	//	for (int i=0;i<CGI->playerint.size();i++)
	//	{
	//		CGI->state->currentPlayer=CGI->playerint[i]->playerID;
	//		try
	//		{
	//			CGI->playerint[i]->yourTurn();
	//		}HANDLE_EXCEPTION
	//	}
	//}
}