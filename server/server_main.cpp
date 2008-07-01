#include <iostream>
#include <string>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include "../global.h"
std::string NAME = NAME_VER + std::string(" (server)");
//using boost::asio::ip::tcp;
//using namespace boost;
//using namespace boost::asio;
//
//class CConnection
//{
//public:
//	int ID;
//	tcp::socket socket;
//	void witaj()
//	{
//		char message[50]; strcpy_s(message,50,NAME.c_str());message[NAME.size()]='\n';
//		write(socket,buffer("Aiya!\n"));
//		write(socket,buffer(message,NAME.size()+1));
//	}
//	CConnection(io_service& io_service, int id=-1)
//		: socket(io_service), ID(id)
//	{
//	}
//};
//
//class CVCMIServer
//{
//	tcp::acceptor acceptor;
//	std::vector<CConnection*> connections;
//public:
//	CVCMIServer(io_service& io_service)
//    : acceptor(io_service, tcp::endpoint(tcp::v4(), 3030))
//	{
//		start_accept();
//	}
//
//private:
//	void start_accept()
//	{
//		std::cout<<"Listening for connections at port " << acceptor.local_endpoint().port() << std::endl;
//		CConnection * new_connection = new CConnection(acceptor.io_service());
//		acceptor.accept(new_connection->socket);
//		new_connection->witaj();
//		acceptor.async_accept(new_connection->socket,
//			boost::bind(&CVCMIServer::gotConnection, this, new_connection,
//			placeholders::error));
//	}
//
//	void gotConnection(CConnection * connection,const system::error_code& error)
//	{
//		if (!error)
//		{
//			std::cout<<"Got connection!" << std::endl;
//			connection->witaj();
//			start_accept();
//		}
//		else
//		{
//			std::cout<<"Got connection but there is an error " << std::endl;
//		}
//	}
//
//};
//
int main()
{
//  try
//  {
//    io_service io_service;
//    CVCMIServer server(io_service);
//    io_service.run();
//  }
//  catch (std::exception& e)
//  {
//    std::cerr << e.what() << std::endl;
//  }
//
  return 0;
}