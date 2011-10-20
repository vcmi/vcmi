#include "../global.h"
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/program_options.hpp>
namespace po = boost::program_options;

void prog_help() 
{
	throw std::exception("The method or operation is not implemented.");
}

int main(int argc, const char **argv)
{
	std::cout << "VCMI Odpalarka\nMy path: " << argv[0] << std::endl;

	po::options_description opts("Allowed options");
	opts.add_options()
		("help,h", "display help and exit")
		("aiLeft,l", po::value<std::string>(), "Left AI path")
		("aiRight,r", po::value<std::string>(), "Right AI path")
		("battle,b", po::value<std::string>(), "Duel file path")
		("visualization,v", "Runs a client to display a visualization of battle");

	std::string leftAI = "StupidAI", 
				rightAI = "StupidAI",
				battle = "b1.json";

	po::variables_map vm;
	if(argc > 1)
	{
		try
		{
			po::store(po::parse_command_line(argc, argv, opts), vm);
			po::notify(vm);
			leftAI = vm["aiLeft"].as<std::string>();
			rightAI = vm["aiRight"].as<std::string>();
			battle = vm["battle"].as<std::string>();
		}
		catch(std::exception &e) 
		{
			std::cerr << "Failure during parsing command-line options:\n" << e.what() << std::endl;
			exit(1);
		}
	}
	else
	{
		std::cout << "Default AIs will be used." << std::endl;
	}

	if(vm.count("help"))
	{
		prog_help();
		return 0;
	}



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

	bool withVisualization = vm.count("visualization");
	std::string serverCommand = servername + " " + battle + " " + leftAI + " " + rightAI + (withVisualization ? " v" : "");
	std::string runnerCommand = runnername;
	boost::thread t(boost::bind(std::system, serverCommand.c_str()));
	boost::thread tt(boost::bind(std::system, runnerCommand.c_str()));
	boost::thread ttt(boost::bind(std::system, runnerCommand.c_str()));
	boost::thread tttt(boost::bind(std::system, runnername.c_str()));
	if(withVisualization)
	{
		//boost::this_thread::sleep(boost::posix_time::millisec(500)); //FIXME
		boost::thread tttt(boost::bind(std::system, "VCMI_Client.exe -battle"));
	}

	//boost::this_thread::sleep(boost::posix_time::seconds(5));

	t.join();
	return EXIT_SUCCESS;
}