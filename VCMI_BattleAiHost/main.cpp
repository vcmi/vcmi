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
#include "Client.h"
#include "../lib/VCMI_Lib.h"

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
	initDLL(console,logfile);

	logfile = new std::ofstream(("VCMI_Runner_log_" + boost::lexical_cast<std::string>(pid) + ".txt").c_str());

	try
	{

		string host = "127.0.0.1";
		string port = "3030";

		CConnection *serv = NULL;
		int i = 3;
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
				if(!--i)
					exit(0);
			}
		}

		ui8 color;
		StartInfo si;
		string battleAIName;
		*serv >> si >> battleAIName >> color;
		assert(si.mode == StartInfo::DUEL);
		tlog0 << format("Server wants us to run %s in battle %s as side %d") % battleAIName % si.mapname % (int)color;
		

		CGameState *gs = new CGameState();
		gs->init(&si, 0, 0);
		tlog0 << "Gs inited\n";

		CClient cl;
		cl.serv = serv;
		cl.gs = gs;

		tlog0 << "Cl created\n";
		CBattleCallback * cbc = new CBattleCallback(gs, color, &cl);
		tlog0 << "Cbc created\n";
		if(battleAIName.size())
		{
			cl.ai = CDynLibHandler::getNewBattleAI(battleAIName);
			cl.ai->playerID = color;
			tlog0 << "AI created\n";
			cl.ai->init(cbc);
		}
		else
			tlog0 << "Not loading AI, only simulation will be run\n";
		tlog0 << cbc->battleGetAllStacks().size() << std::endl;
		cl.run();
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