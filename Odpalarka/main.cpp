#include "../global.h"
#include <boost/thread.hpp>
#include <boost/bind.hpp>

int main(int argc, const char **)
{
	boost::thread t(boost::bind(std::system, "VCMI_server.exe b1.json StupidAI StupidAI"));
	boost::thread tt(boost::bind(std::system, "VCMI_BattleAiHost.exe"));
	boost::thread ttt(boost::bind(std::system, "VCMI_BattleAiHost.exe"));
	if(argc == 2)
	{
		boost::this_thread::sleep(boost::posix_time::millisec(500)); //FIXME
		boost::thread tttt(boost::bind(std::system, "VCMI_Client.exe -battle"));
	}
	else if(argc == 1)
		boost::thread tttt(boost::bind(std::system, "VCMI_BattleAiHost.exe"));

	//boost::this_thread::sleep(boost::posix_time::seconds(5));

	t.join();
	return EXIT_SUCCESS;
}