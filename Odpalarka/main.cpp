//#include "../global.h"
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/program_options.hpp>
namespace po = boost::program_options;

void prog_help() 
{
	std::cout << "If run without args, then StupidAI will be run on b1.json.\n";
}

void runCommand(const std::string &command, const std::string &name, const std::string &logsDir = "")
{
	static std::string commands[100];
	static int i = 0;
	std::string &cmd = commands[i++];
	if(logsDir.size() && name.size())
		cmd = command + " > " + logsDir + "/" + name + ".txt";
	else
		cmd = command;

	boost::thread tt(boost::bind(std::system, cmd.c_str()));
}

int main(int argc, char **argv)
{
	std::cout << "VCMI Odpalarka\nMy path: " << argv[0] << std::endl;

	po::options_description opts("Allowed options");
	opts.add_options()
		("help,h", "Display help and exit")
		("aiLeft,l", po::value<std::string>()->default_value("StupidAI"), "Left AI path")
		("aiRight,r", po::value<std::string>()->default_value("StupidAI"), "Right AI path")
		("battle,b", po::value<std::string>()->default_value("b1.json"), "Duel file path")
		("resultsOut,o", po::value<std::string>()->default_value("./results.json"), "Output file when results will be appended")
		("logsDir,d", po::value<std::string>()->default_value("."), "Directory where log files will be created")
		("visualization,v", "Runs a client to display a visualization of battle");

	std::string leftAI, rightAI, battle, results, logsDir;
	bool withVisualization = false;

	try
	{
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, opts), vm);
		po::notify(vm);

		if(vm.count("help"))
		{
			opts.print(std::cout);
			prog_help();
			return 0;
		}

		leftAI = vm["aiLeft"].as<std::string>();
		rightAI = vm["aiRight"].as<std::string>();
		battle = vm["battle"].as<std::string>();
		results = vm["resultsOut"].as<std::string>();
		logsDir = vm["logsDir"].as<std::string>();
		withVisualization = vm.count("visualization");
	}
	catch(std::exception &e) 
	{
		std::cerr << "Failure during parsing command-line options:\n" << e.what() << std::endl;
		exit(1);
	}

	std::cout << "Config:\n" << leftAI << " vs " << rightAI << " on " << battle << std::endl;

	if(leftAI.empty() || rightAI.empty() || battle.empty())
	{
		std::cerr << "I wasn't able to retreive names of AI or battles. Ending.\n";
		return 1;
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

	std::string serverCommand = servername + " " + battle + " " + leftAI + " " + rightAI + " " + results + " " + logsDir + " " + (withVisualization ? " v" : "");
	std::string runnerCommand = runnername + " " + logsDir;
	std::cout <<"Server command: " << serverCommand << std::endl << "Runner command: " << runnername << std::endl;

	boost::thread t(boost::bind(std::system, serverCommand.c_str()));
	runCommand(runnerCommand, "first_runner", logsDir);
	runCommand(runnerCommand, "second_runner", logsDir);
	runCommand(runnerCommand, "third_runner", logsDir);
	if(withVisualization)
	{
		//boost::this_thread::sleep(boost::posix_time::millisec(500)); //FIXME
		boost::thread tttt(boost::bind(std::system, "VCMI_Client.exe -battle"));
	}

	//boost::this_thread::sleep(boost::posix_time::seconds(5));

	t.join();
	return EXIT_SUCCESS;
}