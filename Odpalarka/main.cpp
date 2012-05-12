//#include "../global.h"
#include "StdInc.h"
#include "../lib/VCMI_Lib.h"
namespace po = boost::program_options;

std::string leftAI, rightAI, battle, results, logsDir;
bool withVisualization = false;
std::string servername;
std::string runnername;
extern DLL_EXPORT LibClasses * VLC;

std::string addQuotesIfNeeded(const std::string &s)
{
	if(s.find_first_of(' ') != std::string::npos)
		return "\"" + s + "\"";

	return s;
}

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
	{
		std::string directionLogs = logsDir + "/" + name + ".txt";
		cmd = command + " > " + addQuotesIfNeeded(directionLogs);
	}
	else
		cmd = command;

	boost::thread tt(boost::bind(std::system, cmd.c_str()));
}

double playBattle(const DuelParameters &dp)
{
	{
		CSaveFile out("pliczek.ssnb");
		out << dp;
	}


	std::string serverCommand = servername + " " + addQuotesIfNeeded(battle) + " " + addQuotesIfNeeded(leftAI) + " " + addQuotesIfNeeded(rightAI) + " " + addQuotesIfNeeded(results) + " " + addQuotesIfNeeded(logsDir) + " " + (withVisualization ? " v" : "");
	std::string runnerCommand = runnername + " " + addQuotesIfNeeded(logsDir);
	std::cout <<"Server command: " << serverCommand << std::endl << "Runner command: " << runnerCommand << std::endl;

	int code = 0;
	boost::thread t([&]
	{ 
		code = std::system(serverCommand.c_str());
	});

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
	return code / 1000000.0;
}

void SSNRun()
{
	CArtifact *nowy = new CArtifact();
	nowy->description = "Cudowny miecz Towa gwarantuje zwyciestwo";
	nowy->name = "Cudowny miecz";
	nowy->constituentOf = nowy->constituents = NULL;
	nowy->possibleSlots.push_back(Arts::LEFT_HAND);

	CArtifactInstance *artinst = new CArtifactInstance(nowy);
	auto &arts = VLC->arth->artifacts;
	CArtifactInstance *inny = new CArtifactInstance(VLC->arth->artifacts[15]);

	artinst->addNewBonus(new Bonus(Bonus::PERMANENT, Bonus::PRIMARY_SKILL, Bonus::ARTIFACT_INSTANCE, +25, nowy->id, PrimarySkill::ATTACK));
	artinst->addNewBonus(new Bonus(Bonus::PERMANENT, Bonus::PRIMARY_SKILL, Bonus::ARTIFACT_INSTANCE, +25, nowy->id, PrimarySkill::DEFENSE));

	DuelParameters dp;
	dp.bfieldType = 1;
	dp.terType = 1;

	for(int i = 0; i < 2 ; i++)
	{
		auto &side = dp.sides[i];
		side.heroId = i;
		side.heroPrimSkills.resize(4,0);
		side.stacks[0] = DuelParameters::SideSettings::StackSettings(10+i, 40+i);
	}

	auto bonuses = artinst->getBonuses([](const Bonus *){ return true; });
	BOOST_FOREACH(Bonus *b, *bonuses) 
	{
		std::cout << format("%s (%d) value:%d, description: %s\n") % bonusTypeToString(b->type) % b->subtype % b->val % b->Description();
	}

	


	//lewa strona z art 0.9
	//bez artefaktow -0.41
	//prawa strona z art. -0.926

	dp.sides[0].artifacts[Arts::LEFT_HAND] = artinst;

	auto battleOutcome = playBattle(dp);
	int g = 4;
}

int main(int argc, char **argv)
{
	std::cout << "VCMI Odpalarka\nMy path: " << argv[0] << std::endl;

	po::options_description opts("Allowed options");
	opts.add_options()
		("help,h", "Display help and exit")
		("aiLeft,l", po::value<std::string>()->default_value("StupidAI"), "Left AI path")
		("aiRight,r", po::value<std::string>()->default_value("StupidAI"), "Right AI path")
		("battle,b", po::value<std::string>()->default_value("pliczek.ssnb"), "Duel file path")
		("resultsOut,o", po::value<std::string>()->default_value("./results.txt"), "Output file when results will be appended")
		("logsDir,d", po::value<std::string>()->default_value("."), "Directory where log files will be created")
		("visualization,v", "Runs a client to display a visualization of battle");


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



	runnername = 
#ifdef _WIN32
		"VCMI_BattleAiHost.exe"
#else
		"./vcmirunner"
#endif
	;
	servername = 
#ifdef _WIN32
		"VCMI_server.exe"
#else
		"./vcmiserver"
#endif
	;

	
	VLC = new LibClasses();
	VLC->init();

	SSNRun();

	return EXIT_SUCCESS;
}