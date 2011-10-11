#include "../global.h"
#include <boost/thread.hpp>
#include <boost/bind.hpp>

int main(int argc, const char **argv)
{
	std::string runnername = 
#ifdef _WIN32
		"VCMI_BattleAiHost.exe"
#else
		"./vcmirunner"
#endif
	;
	std::string servername = 
#ifdef _WIN32
		"VCMI_server.exe"
#else
		"./vcmiserver"
#endif
	;

	std::string serverCommand = servername + " b1.json StupidAI MadAI"; // StupidAI MadAI
	boost::thread t(boost::bind(std::system, serverCommand.c_str()));
	boost::thread tt(boost::bind(std::system, runnername.c_str()));
	boost::thread ttt(boost::bind(std::system, runnername.c_str()));
	if(argc == 2)
	{
		boost::this_thread::sleep(boost::posix_time::millisec(500)); //FIXME
		boost::thread tttt(boost::bind(std::system, "VCMI_Client.exe -battle"));
	}
	else if(argc == 1)
		boost::thread tttt(boost::bind(std::system, runnername.c_str()));

	//boost::this_thread::sleep(boost::posix_time::seconds(5));

	t.join();
	return EXIT_SUCCESS;
}