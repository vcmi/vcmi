#include <iostream>
#include <string>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include "../global.h"
#include "../lib/Connection.h"
std::string NAME = NAME_VER + std::string(" (server)");
using boost::asio::ip::tcp;
using namespace boost;
using namespace boost::asio;
using namespace boost::asio::ip;

class CVCMIServer
{
	tcp::acceptor acceptor;
	std::vector<CConnection*> connections;
public:
	CVCMIServer(io_service& io_service)
    : acceptor(io_service, tcp::endpoint(tcp::v4(), 3030))
	{
		start_accept();
	}
private:
	void start_accept()
	{
		boost::system::error_code error;
		std::cout<<"Listening for connections at port " << acceptor.local_endpoint().port() << std::endl;
		tcp::socket s(acceptor.io_service());
		acceptor.accept(s,error);
		if (!error)
		{
			CConnection *connection = new CConnection(&s,&s.io_service(),NAME,std::cout);
			std::cout<<"Got connection!" << std::endl;
		}
		else
		{
			std::cout<<"Got connection but there is an error " << std::endl;
		}

		//asio::write(s,asio::buffer("570"));
		//new_connection->witaj();
		//acceptor.async_accept(s,
		//	boost::bind(&CVCMIServer::gotConnection, this, &s,
		//	placeholders::error));
	}

	void gotConnection(tcp::socket *s,const boost::system::error_code& error)
	{
	}

};

int main()
{
  try
  {
    io_service io_service;
    CVCMIServer server(io_service);
    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
