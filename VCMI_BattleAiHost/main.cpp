 #include "../global.h"
 #include "../lib/Connection.h"
 #include <boost/lexical_cast.hpp>
 #include <boost/thread.hpp>
#include <fstream>
#include "../StartInfo.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif
#include "../lib/CGameState.h"
#include "../CCallback.h"
#include "../lib/CGameInterface.h"
#include <boost/format.hpp>

using namespace std;
using namespace boost;

std::string NAME = NAME_VER + std::string(" DLL runner");

int main(int argc, char** argv)
{
	int pid = -1;

#ifdef _WIN32
	pid = GetCurrentProcessId();
#else
	pid = getpid();
#endif

	logfile = new std::ofstream(("VCMI_Server_log_" + boost::lexical_cast<std::string>(pid) + ".txt").c_str());

	try
	{

		string host = "127.0.0.1";
		string port = "3030";

		CConnection *serv = NULL;
		while(!serv)
		{
			try
			{
				tlog0 << "Establishing connection...\n";
				serv = new CConnection(host, port, "DLL host");
			}
			catch(...)
			{
				tlog1 << "\nCannot establish connection! Retrying within 2 seconds" << std::endl;
				boost::this_thread::sleep(boost::posix_time::seconds(2));
			}
		}

		ui8 color;
		StartInfo si;
		string battleAIName;
		*serv >> si >> battleAIName >> color;
		assert(si.mode == StartInfo::DUEL);
		tlog0 << format("Server wants us to run %s in battle %s as side %d") % battleAIName % si.mapname % color;
		

		CGameState *gs = new CGameState();
		gs->init(&si, 0, 0);


		CBattleCallback * cbc = new CBattleCallback(gs, color, this);
		CBattleGameInterface *ai = CDynLibHandler::getNewBattleAI(battleAIName);
		ai->init(cbc);

	}
	catch(std::exception &e)
	{
		tlog1 << "Encountered exception: " << e.what() << std::endl;
	}
	catch(...)
	{
		tlog1 << "Encountered unknown exception!" << std::endl;
	}
	return EXIT_SUCCESS;
}