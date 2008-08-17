#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include "../global.h"
#include "../lib/Connection.h"
#include "zlib.h"
#ifndef __GNUC__
#include <tchar.h>
#endif
#include "CVCMIServer.h"
#include <boost/crc.hpp>
#include <boost/serialization/split_member.hpp>
#include "../StartInfo.h"
#include "../map.h"
#include "../hch/CLodHandler.h" 
#include "../lib/VCMI_Lib.h"
#include "CGameHandler.h"
std::string NAME = NAME_VER + std::string(" (server)");
using boost::asio::ip::tcp;
using namespace boost;
using namespace boost::asio;
using namespace boost::asio::ip;

bool end2 = false;

CVCMIServer::CVCMIServer()
: io(new io_service()), acceptor(new tcp::acceptor(*io, tcp::endpoint(tcp::v4(), 3030)))
{
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
				std::cout<<"Cannot establish connection - retrying..." << std::endl;
				i--;
				continue;
			}
			cc = new CConnection(s,NAME,std::cout);
		}	
		gh.conns.insert(cc);
	}

	gh.run();
}
void CVCMIServer::start()
{
	boost::system::error_code error;
	std::cout<<"Listening for connections at port " << acceptor->local_endpoint().port() << std::endl;
	tcp::socket * s = new tcp::socket(acceptor->io_service());
	acceptor->accept(*s,error);
	if (error)
	{
		std::cout<<"Got connection but there is an error " << std::endl;
		return;
	}
	CConnection *connection = new CConnection(s,NAME,std::cout);
	std::cout<<"Got connection!" << std::endl;
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
		}
	}
}

#ifndef __GNUC__
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, _TCHAR* argv[])
#endif
{
	CLodHandler h3bmp;
	h3bmp.init("Data/H3bitmap.lod","Data");
	initDLL(&h3bmp);
	srand ( (unsigned int)time(NULL) );
	try
	{
		io_service io_service;
		CVCMIServer server;
		while(!end2)
			server.start();
		io_service.run();
	} HANDLE_EXCEPTION
  return 0;
}
