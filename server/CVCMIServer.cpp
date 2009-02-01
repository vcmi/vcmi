#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include "../global.h"
#include "../lib/Connection.h"
#include "../hch/CArtHandler.h"
#include "../hch/CDefObjInfoHandler.h"
#include "../hch/CGeneralTextHandler.h"
#include "../hch/CHeroHandler.h"
#include "../hch/CTownHandler.h"
#include "../hch/CObjectHandler.h"
#include "../hch/CBuildingHandler.h"
#include "../hch/CSpellHandler.h"
#include "zlib.h"
#ifndef __GNUC__
#include <tchar.h>
#endif
#include "CVCMIServer.h"
#include <boost/crc.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include "../StartInfo.h"
#include "../map.h"
#include "../hch/CLodHandler.h" 
#include "../lib/Interprocess.h"
#include "../lib/VCMI_Lib.h"
#include "CGameHandler.h"
std::string NAME = NAME_VER + std::string(" (server)");
using namespace boost;
using namespace boost::asio;
using namespace boost::asio::ip;
namespace intpr = boost::interprocess;
bool end2 = false;
int port = 3030;

void vaccept(tcp::acceptor *ac, tcp::socket *s, boost::system::error_code *error)
{
	ac->accept(*s,*error);
}

CVCMIServer::CVCMIServer()
: io(new io_service()), acceptor(new tcp::acceptor(*io, tcp::endpoint(tcp::v4(), port)))
{
	tlog4 << "CVCMIServer created!" <<std::endl;
}
CVCMIServer::~CVCMIServer()
{
	//delete io;
	//delete acceptor;
}

void CVCMIServer::newGame(CConnection *c)
{
	CGameHandler gh;
	boost::system::error_code error;
	StartInfo *si = new StartInfo;
	ui8 clients;
	*c >> clients; //how many clients should be connected - TODO: support more than one
	*c >> *si; //get start options
	int problem;
#ifdef _MSC_VER
	FILE *f;
	problem = fopen_s(&f,si->mapname.c_str(),"r");
#else
	FILE * f = fopen(si->mapname.c_str(),"r");
	problem = !f;
#endif
	if(problem)
	{
		*c << ui8(problem); //WRONG!
		return;
	}
	else
	{
		fclose(f);
		*c << ui8(0); //OK!
	}

	gh.init(si,rand());

	CConnection* cc; //tcp::socket * ss;
	for(int i=0; i<clients; i++)
	{
		if(!i) 
		{
			cc=c;
		}
		else
		{
			tcp::socket * s = new tcp::socket(acceptor->io_service());
			acceptor->accept(*s,error);
			if(error) //retry
			{
				tlog3<<"Cannot establish connection - retrying..." << std::endl;
				i--;
				continue;
			}
			cc = new CConnection(s,NAME);
		}	
		gh.conns.insert(cc);
	}

	gh.run(false);
}
void CVCMIServer::start()
{
	ServerReady *sr = NULL;
	intpr::mapped_region *mr;
	try
	{
		intpr::shared_memory_object smo(intpr::open_only,"vcmi_memory",intpr::read_write);
		smo.truncate(sizeof(ServerReady));
		mr = new intpr::mapped_region(smo,intpr::read_write);
		sr = (ServerReady*)mr->get_address();
	}
	catch(...)
	{
		intpr::shared_memory_object smo(intpr::create_only,"vcmi_memory",intpr::read_write);
		smo.truncate(sizeof(ServerReady));
		mr = new intpr::mapped_region(smo,intpr::read_write);
		sr = new(mr->get_address())ServerReady();
	}

	boost::system::error_code error;
	tlog0<<"Listening for connections at port " << acceptor->local_endpoint().port() << std::endl;
	tcp::socket * s = new tcp::socket(acceptor->io_service());
	boost::thread acc(boost::bind(vaccept,acceptor,s,&error));
	sr->setToTrueAndNotify();
	delete mr;
	acc.join();
	if (error)
	{
		tlog2<<"Got connection but there is an error " << std::endl << error;
		return;
	}
	tlog0<<"We've accepted someone... " << std::endl;
	CConnection *connection = new CConnection(s,NAME);
	tlog0<<"Got connection!" << std::endl;
	while(!end2)
	{
		uint8_t mode;
		*connection >> mode;
		switch (mode)
		{
		case 0:
			connection->socket->close();
			exit(0);
			break;
		case 1:
			connection->socket->close();
			return;
			break;
		case 2:
			newGame(connection);
			break;
		case 3:
			loadGame(connection);
			break;
		}
	}
}

void CVCMIServer::loadGame( CConnection *c )
{
	std::string fname;
	CGameHandler gh;
	boost::system::error_code error;
	ui8 clients;
	*c >> clients >> fname; //how many clients should be connected - TODO: support more than one

	{
		ui32 ver;
		char sig[8];
		CMapHeader dum;

		CLoadFile lf(fname + ".vlgm1");
		lf >> sig >> ver >> dum >> *sig;
		tlog0 <<"Reading save signature"<<std::endl;

		lf >> *VLC;
		tlog0 <<"Reading handlers"<<std::endl;

		lf >> (gh.gs);
		tlog0 <<"Reading gamestate"<<std::endl;
	}

	{
		CLoadFile lf(fname + ".vsgm1");
		lf >> gh;
	}

	*c << ui8(0);

	CConnection* cc; //tcp::socket * ss;
	for(int i=0; i<clients; i++)
	{
		if(!i) 
		{
			cc=c;
		}
		else
		{
			tcp::socket * s = new tcp::socket(acceptor->io_service());
			acceptor->accept(*s,error);
			if(error) //retry
			{
				tlog3<<"Cannot establish connection - retrying..." << std::endl;
				i--;
				continue;
			}
			cc = new CConnection(s,NAME);
		}	
		gh.conns.insert(cc);
	}

	gh.run(true);
}

#ifndef __GNUC__
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char** argv)
#endif
{
	logfile = new std::ofstream("VCMI_Server_log.txt");
	console = new CConsoleHandler;
	//boost::thread t(boost::bind(&CConsoleHandler::run,::console));
	if(argc > 1)
	{
#ifdef _MSC_VER
		port = _tstoi(argv[1]);
#else
		port = _ttoi(argv[1]);
#endif
	}
	tlog0 << "Port " << port << " will be used." << std::endl;
	CLodHandler h3bmp;
	h3bmp.init("Data" PATHSEPARATOR "H3bitmap.lod","Data");
	initDLL(&h3bmp,console,logfile);
	srand ( (unsigned int)time(NULL) );
	try
	{
		io_service io_service;
		CVCMIServer server;
		while(!end2)
		{
			server.start();
		}
		io_service.run();
	} HANDLE_EXCEPTION
  return 0;
}
