#include "../global.h"
#include <boost/thread.hpp>
#include <boost/bind.hpp>

int main()
{
	boost::thread t(boost::bind(std::system, "VCMI_server.exe b1.json StupidAI StupidAI"));
	boost::thread tt(boost::bind(std::system, "VCMI_BattleAiHost.exe"));
	boost::thread ttt(boost::bind(std::system, "VCMI_BattleAiHost.exe"));
	boost::thread tttt(boost::bind(std::system, "VCMI_BattleAiHost.exe"));
	boost::this_thread::sleep(boost::posix_time::seconds(5));

	return EXIT_SUCCESS;
}