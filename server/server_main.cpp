#include <iostream>
#include <string>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include "../global.h"
#include "../lib/Connection.h"
#include "../CGameState.h"
#include "zlib.h"
#include <boost/thread.hpp>
#include <boost/crc.hpp>
#include <boost/serialization/split_member.hpp>
#include "../StartInfo.h"
std::string NAME = NAME_VER + std::string(" (server)");
using boost::asio::ip::tcp;
using namespace boost;
using namespace boost::asio;
using namespace boost::asio::ip;
mutex smx1;
class CVCMIServer
{
	CGameState *gs;
	tcp::acceptor acceptor;
	std::map<int,CConnection*> connections;
	std::set<CConnection*> conns;
	ui32 seed;
public:
	CVCMIServer(io_service& io_service)
    : acceptor(io_service, tcp::endpoint(tcp::v4(), 3030))
	{
		start();
	}
	void setUpConnection(CConnection *c, std::string mapname, si32 checksum)
	{
		ui8 quantity, pom;
		(*c) << mapname << checksum << seed;
		(*c) >> quantity;
		for(int i=0;i<quantity;i++)
		{
			(*c) >> pom;
			smx1.lock();
			connections[pom] = c;
			conns.insert(c);
			smx1.unlock();
		}
	}
	void newGame(CConnection &c)
	{
		boost::system::error_code error;
		StartInfo *si = new StartInfo;
		ui8 clients;
		std::string mapname;
		c >> clients;
		c >> mapname;
	  //getting map
		gzFile map = gzopen(mapname.c_str(),"rb");
		if(!map){ c << int8_t(1); return; }
		std::vector<unsigned char> mapstr; int pom;
		while((pom=gzgetc(map))>=0)
		{
			mapstr.push_back(pom);
		}
		gzclose(map);
	  //map is decompressed
		c << int8_t(0); //OK!
		gs = new CGameState();
		gs->scenarioOps = si;
		c > *si; //get start options
		boost::crc_32_type  result;
		result.process_bytes(&(*mapstr.begin()),mapstr.size());
		int checksum = result.checksum();
		std::cout << "Checksum:" << checksum << std::endl; 
		CConnection* cc; tcp::socket * ss;
		for(int i=0; i<clients; i++)
		{
			if(!i) 
			{
				cc=&c;
			}
			else
			{
				tcp::socket * s = new tcp::socket(acceptor.io_service());
				acceptor.accept(*s,error);
				if(error) //retry
				{
					std::cout<<"Cannot establish connection - retrying..." << std::endl;
					i--;
					continue;
				}
				cc = new CConnection(s,NAME,std::cout);
			}
			setUpConnection(cc,mapname,checksum);
		}
		//TODO: wait for other connections
	}
	void start()
	{
		srand ( time(NULL) );
		seed = rand();
		boost::system::error_code error;
		std::cout<<"Listening for connections at port " << acceptor.local_endpoint().port() << std::endl;
		tcp::socket * s = new tcp::socket(acceptor.io_service());
		acceptor.accept(*s,error);
		if (error)
		{
			std::cout<<"Got connection but there is an error " << std::endl;
			return;
		}
		CConnection connection(s,NAME,std::cout);
		std::cout<<"Got connection!" << std::endl;
		while(1)
		{
			uint8_t mode;
			connection >> mode;
			switch (mode)
			{
			case 0:
				connection.socket->close();
				exit(0);
				break;
			case 1:
				connection.socket->close();
				return;
				break;
			case 2:
				newGame(connection);
				break;
			}
		}
	}
};

int _tmain(int argc, _TCHAR* argv[])
{
  try
  {
    io_service io_service;
    CVCMIServer server(io_service);
	while(1)
		server.start();
    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
