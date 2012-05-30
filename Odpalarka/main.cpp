//#include "../global.h"
#include "StdInc.h"
#include "../lib/VCMI_Lib.h"
namespace po = boost::program_options;
namespace fs = boost::filesystem;
using namespace std;


//FANN
#include <doublefann.h>
#include <fann_cpp.h>

std::string leftAI, rightAI, battle, results, logsDir;
bool withVisualization = false;
std::string servername;
std::string runnername;
extern DLL_EXPORT LibClasses * VLC;

typedef std::map<int, CArtifactInstance*> TArtSet;

namespace Utilities
{
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

	string toString(int i)
	{
		return boost::lexical_cast<string>(i);
	}

	string describeBonus(const Bonus &b)
	{
		return "+" + toString(b.val) + "_to_" + bonusTypeToString(b.type)+"_sub"+toString(b.subtype);
	}
}

using namespace Utilities;

struct Example
{
	//ANN input
	DuelParameters dp;
	CArtifactInstance *art;
	//ANN expected output
	double value;

	//other
	std::string description;

	int i, j, k; //helper values for identification

	Example(){}
	Example(const DuelParameters &Dp, CArtifactInstance *Art, double Value)
		: dp(Dp), art(Art), value(Value)
	{}


	inline bool operator<(const Example & rhs) const
	{
		if (k<rhs.k)
			return true;
		if (k>rhs.k)
			return false;
		if (j<rhs.j)
			return true;
		if (j>rhs.j)
			return false;
		if (i<rhs.i)
			return true;
		if (i>rhs.i)
			return false;
		return false;
	}

	bool operator==(const Example &rhs) const
	{
		return rhs.i == i && rhs.j == j && rhs.k == k;
	}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & dp & art & value & description & i & j & k;
	}
};

class Framework
{
	static CArtifactInstance *generateArtWithBonus(const Bonus &b);
	static DuelParameters generateDuel(const ArmyDescriptor &ad); //generates simple duel where both sides have given army
	static void runCommand(const std::string &command, const std::string &name, const std::string &logsDir = "");
	static double playBattle(const DuelParameters &dp);
	static double cmpArtSets(DuelParameters dp, TArtSet setL, TArtSet setR);
	static double rateArt(const DuelParameters dp, CArtifactInstance * inst); //rates given artifact
	static int theLastN(); 
	static vector<string> getFileNames(const string &dirname = "./examples/", const std::string &ext = "example");
	static vector<ArmyDescriptor> learningArmies();
	static vector<Bonus> learningBonuses();

public:
	Framework();
	~Framework();

	static void buildLearningSet(); 
	static vector<Example> loadExamples(bool printInfo = true);
};

vector<string> Framework::getFileNames(const string &dirname, const std::string &ext)
{
	vector<string> ret;
	if(!fs::exists(dirname))
	{
		tlog1 << "Cannot find " << dirname << " directory! Will attempt creating it.\n";
		fs::create_directory(dirname);
	}

	fs::path tie(dirname);
	fs::directory_iterator end_iter;
	for ( fs::directory_iterator file (tie); file!=end_iter; ++file )
	{
		if(fs::is_regular_file(file->status())
			&& boost::ends_with(file->path().filename(), ext))
		{
			ret.push_back(file->path().string());
		}
	}

	return ret;
}

vector<Example> Framework::loadExamples(bool printInfo)
{
	std::vector<Example> examples;
	BOOST_FOREACH(auto fname, getFileNames("./examples/", "example"))
	{
		CLoadFile loadf(fname);
		Example ex;
		loadf >> ex;
		examples.push_back(ex);
	}

	if(printInfo)
	{
		tlog0 << "Found " << examples.size() << " examples.\n";
		BOOST_FOREACH(auto &ex, examples)
		{
			tlog0 << format("Battle on army %d for bonus %d of value %d has resultdiff %lf\n") % ex.i % ex.j % ex.k % ex.value;
		}
	}

	return examples;
}

int Framework::theLastN()
{
	auto fnames = getFileNames();
	if(!fnames.size())
		return -1;

	range::sort(fnames, [](const std::string &a, const std::string &b)
	{
		return boost::lexical_cast<int>(fs::basename(a)) < boost::lexical_cast<int>(fs::basename(b));
	});

	return boost::lexical_cast<int>(fs::basename(fnames.back()));
}

void Framework::buildLearningSet()
{
	vector<Example> examples = loadExamples();
	range::sort(examples);


	int startExamplesFrom = 0;
	ofstream learningLog("log.txt", std::ios::app);

	int n = theLastN()+1;

	auto armies = learningArmies();
	auto bonuese = learningBonuses();

	for(int i = 0; i < armies.size(); i++)
	{
		string army = "army" + toString(i);
		for(int j = 0; j < bonuese.size(); j++)
		{
			Bonus b = bonuese[j];
			string bonusStr = "bonus" + toString(j) + describeBonus(b);
			for(int k = 0; k < 10; k++)
			{
				int nHere = n++;

// 				if(nHere < startExamplesFrom)
// 					continue;
// 					


				tlog2 << "n="<<nHere<<std::endl;
				b.val = k;

				Example ex;
				ex.i = i;
				ex.j = j;
				ex.k = k;
				ex.art = generateArtWithBonus(b);
				ex.dp = generateDuel(armies[i]);
				ex.description = army + "\t" + describeBonus(b) + "\t";

				if(vstd::contains(examples, ex))
				{
					string msg = str(format("n=%d \tarmy %d \tbonus %d \tresult %lf \t Bonus#%s#") % nHere % i %j % ex.value % describeBonus(b));
					tlog0 << "Already present example, skipping " << msg;
					continue;
				}

				ex.value = rateArt(ex.dp, ex.art);
				
				CSaveFile output("./examples/" + toString(nHere) + ".example");
				output << ex;
				time_t rawtime;
				struct tm * timeinfo;
				time ( &rawtime );
				timeinfo = localtime ( &rawtime );
				string msg = str(format("n=%d \tarmy %d \tbonus %d \tresult %lf \t Bonus#%s# \tdate: %s") % nHere % i %j % ex.value % describeBonus(b) % asctime(timeinfo));
				learningLog << msg << flush;
				tlog0 << msg;
			}
		}
	}

	tlog0 << "Set of learning/testing examples is complete and ready!\n";
}

vector<ArmyDescriptor> Framework::learningArmies()
{
	vector<ArmyDescriptor> ret;

	//armia zlozona ze stworow z malymi HP-kami
	ArmyDescriptor lowHP;
	lowHP[0] = CStackBasicDescriptor(1, 9); //halabardier
	lowHP[1] = CStackBasicDescriptor(14, 20); //centaur
	lowHP[2] = CStackBasicDescriptor(139, 123); //chlop
	lowHP[3] = CStackBasicDescriptor(70, 30); //troglodyta
	lowHP[4] = CStackBasicDescriptor(42, 50); //imp

	//armia zlozona z poteznaych stworow
	ArmyDescriptor highHP;
	highHP[0] = CStackBasicDescriptor(13, 17); //archaniol
	highHP[1] = CStackBasicDescriptor(132, 8); //azure dragon
	highHP[2] = CStackBasicDescriptor(133, 10); //crystal dragon
	highHP[3] = CStackBasicDescriptor(83, 22); //black dragon

	//armia zlozona z tygodniowego przyrostu w zamku
	auto &castleTown = VLC->townh->towns[0];
	ArmyDescriptor castleNormal;
	for(int i = 0; i < 7; i++)
	{
		auto &cre = VLC->creh->creatures[castleTown.basicCreatures[i]];
		castleNormal[i] = CStackBasicDescriptor(cre.get(), cre->growth);
	}
	castleNormal[5].type = VLC->creh->creatures[52]; //replace cavaliers with Efreeti -> stupid ai sometimes blocks with two-hex walkers

	//armia zlozona z tygodniowego ulepszonego przyrostu w ramparcie
	auto &rampartTown = VLC->townh->towns[1];
	ArmyDescriptor rampartUpgraded;
	for(int i = 0; i < 7; i++)
	{
		auto &cre = VLC->creh->creatures[rampartTown.upgradedCreatures[i]];
		rampartUpgraded[i] = CStackBasicDescriptor(cre.get(), cre->growth);
	}
	rampartUpgraded[5].type = VLC->creh->creatures[52]; //replace unicorn with Efreeti -> stupid ai sometimes blocks with two-hex walkers

	//armia zlozona z samych strzelcow
	ArmyDescriptor shooters;
	shooters[0] = CStackBasicDescriptor(35, 17); //arcymag
	shooters[1] = CStackBasicDescriptor(41, 1); //titan
	shooters[2] = CStackBasicDescriptor(3, 70); //kusznik
	shooters[3] = CStackBasicDescriptor(89, 50); //ulepszony ork



	ret.push_back(lowHP);
	ret.push_back(highHP);
	ret.push_back(castleNormal);
	ret.push_back(rampartUpgraded);
	ret.push_back(shooters);
	return ret;
}

vector<Bonus> Framework::learningBonuses()
{
	vector<Bonus> ret;


	Bonus b;
	b.type = Bonus::PRIMARY_SKILL;
	b.subtype = PrimarySkill::ATTACK;
	ret.push_back(b);

	b.subtype = PrimarySkill::DEFENSE;
	ret.push_back(b);

	b.type = Bonus::STACK_HEALTH;
	b.subtype = 0;
	ret.push_back(b);

	b.type = Bonus::STACKS_SPEED;
	ret.push_back(b);

	b.type = Bonus::BLOCKS_RETALIATION;
	ret.push_back(b);

	b.type = Bonus::ADDITIONAL_RETALIATION;
	ret.push_back(b);

	b.type = Bonus::ADDITIONAL_ATTACK;
	ret.push_back(b);

	b.type = Bonus::CREATURE_DAMAGE;
	ret.push_back(b);

	b.type = Bonus::ALWAYS_MAXIMUM_DAMAGE;
	ret.push_back(b);

	b.type = Bonus::NO_DISTANCE_PENALTY;
	ret.push_back(b);

	return ret;
}

double Framework::rateArt(const DuelParameters dp, CArtifactInstance * inst)
{
	TArtSet setL, setR;
	setL[inst->artType->possibleSlots[0]] = inst;

	double resultLR = cmpArtSets(dp, setL, setR),
		resultRL = cmpArtSets(dp, setR, setL),
		resultsBase = cmpArtSets(dp, TArtSet(), TArtSet());

	//lewa strona z art 0.9
	//bez artefaktow -0.41
	//prawa strona z art. -0.926

	double LRgain = resultLR - resultsBase,
		RLgain = resultsBase - resultRL;
	return LRgain+RLgain;
}

double Framework::cmpArtSets(DuelParameters dp, TArtSet setL, TArtSet setR)
{
	dp.sides[0].artifacts = setL;
	dp.sides[1].artifacts = setR;

	auto battleOutcome = playBattle(dp);
	return battleOutcome;
}

double Framework::playBattle(const DuelParameters &dp)
{
	string battleFileName = "pliczek.ssnb";
	{
		CSaveFile out(battleFileName);
		out << dp;
	}


	std::string serverCommand = servername + " " + addQuotesIfNeeded(battleFileName) + " " + addQuotesIfNeeded(leftAI) + " " + addQuotesIfNeeded(rightAI) + " " + addQuotesIfNeeded(results) + " " + addQuotesIfNeeded(logsDir) + " " + (withVisualization ? " v" : "");
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

void Framework::runCommand(const std::string &command, const std::string &name, const std::string &logsDir /*= ""*/)
{
	static std::string commands[100000];
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

DuelParameters Framework::generateDuel(const ArmyDescriptor &ad)
{
	DuelParameters dp;
	dp.bfieldType = 1;
	dp.terType = 1;

	auto &side = dp.sides[0];
	side.heroId = 0;
	side.heroPrimSkills.resize(4,0);
	BOOST_FOREACH(auto &stack, ad)
	{
		side.stacks[stack.first] = DuelParameters::SideSettings::StackSettings(stack.second.type->idNumber, stack.second.count);
	}
	dp.sides[1] = side;
	dp.sides[1].heroId = 1;
	return dp;
}

CArtifactInstance * Framework::generateArtWithBonus(const Bonus &b)
{
	std::vector<CArtifactInstance*> ret;

	static CArtifact *nowy = NULL;

	if(!nowy)
	{
		nowy = new CArtifact();
		nowy->description = "Cudowny miecz Towa gwarantuje zwyciestwo";
		nowy->name = "Cudowny miecz";
		nowy->constituentOf = nowy->constituents = NULL;
		nowy->possibleSlots.push_back(Arts::LEFT_HAND);
	}

	CArtifactInstance *artinst = new CArtifactInstance(nowy);
	artinst->addNewBonus(new Bonus(b));
	return artinst;
}

class SSN
{
	FANN::neural_net net;

	void init();
	static int ANNCallback(FANN::neural_net &net, FANN::training_data &train, unsigned int max_epochs, unsigned int epochs_between_reports, float desired_error, unsigned int epochs, void *user_data);
	static double * genSSNinput(const DuelParameters::SideSettings & dp, CArtifactInstance * art, si32 bfieldType, si32 terType);
	static const unsigned int num_input = 18;
public:
	SSN();
	~SSN();

	void learn(const std::vector<Example> & input);
	double run(const DuelParameters &dp, CArtifactInstance * inst); 

	void save(const std::string &filename);
};

SSN::SSN()
{
	init();
}

void SSN::init()
{
	const float learning_rate = 0.7f;
	const unsigned int num_layers = 3;
	const unsigned int num_hidden = 30;
	const unsigned int num_output = 1;
	const float desired_error = 0.001f;
	const unsigned int max_iterations = 300000;
	const unsigned int iterations_between_reports = 1000;

	net.create_standard(num_layers, num_input, num_hidden, num_output);

	net.set_learning_rate(learning_rate);

	net.set_activation_steepness_hidden(0.9);
	net.set_activation_steepness_output(1.0);

	net.set_activation_function_hidden(FANN::SIGMOID_SYMMETRIC_STEPWISE);
	net.set_activation_function_output(FANN::SIGMOID_SYMMETRIC_STEPWISE);

	net.randomize_weights(0.0, 1.0);
}

double SSN::run(const DuelParameters &dp, CArtifactInstance * inst)
{
	double * input = genSSNinput(dp.sides[0], inst, dp.bfieldType, dp.terType);
	double * out = net.run(input);
	double ret = *out;
	free(out);

	return ret;
}

int SSN::ANNCallback(FANN::neural_net &net, FANN::training_data &train, unsigned int max_epochs, unsigned int epochs_between_reports, float desired_error, unsigned int epochs, void *user_data)
{
	//cout << "Epochs     " << setw(8) << epochs << ". "
	//	<< "Current Error: " << left << net.get_MSE() << right << endl;
	return 0;
}

void SSN::learn(const std::vector<Example> & input)
{
	//FIXME - sypie przy destrukcji
	//FANN::training_data td; 
	FANN::training_data &td = *new FANN::training_data; 

	double ** inputs = new double *[input.size()];
	double ** outputs = new double *[input.size()];
	for(int i=0; i<input.size(); ++i)
	{
		const auto & ci = input[i];
		inputs[i] = genSSNinput(ci.dp.sides[0], ci.art, ci.dp.bfieldType, ci.dp.terType);
		outputs[i] = new double;
		*(outputs[i]) = ci.value;
	}
	td.set_train_data(input.size(), num_input, inputs, 1, outputs);
	net.set_callback(ANNCallback, NULL);
	net.train_on_data(td, 1000, 1000, 0.01);
}

double * SSN::genSSNinput(const DuelParameters::SideSettings & dp, CArtifactInstance * art, si32 bfieldType, si32 terType)
{
	double * ret = new double[num_input];
	double * cur = ret;

	//general description

	*(cur++) = bfieldType/30.0;
	*(cur++) = terType/12.0;

	//creature & hero description


	*(cur++) = dp.heroId/200.0;
	for(int k=0; k<4; ++k)
		*(cur++) = dp.heroPrimSkills[k]/20.0;

	//weighted average of statistics
	auto avg = [&](std::function<int(CCreature *)> getter) -> double
	{
		double ret = 0.0;
		int div = 0;
		for(int i=0; i<7; ++i)
		{
			auto & cstack = dp.stacks[i];
			if(cstack.count > 0)
			{
				ret += getter(VLC->creh->creatures[cstack.type]) * cstack.count;
				div+=cstack.count;
			}
		}
		return ret/div;
	};

	*(cur++) = avg([](CCreature * c){return c->attack;})/50.0;
	*(cur++) = avg([](CCreature * c){return c->defence;})/50.0;
	*(cur++) = avg([](CCreature * c){return c->speed;})/15.0;
	*(cur++) = avg([](CCreature * c){return c->hitPoints;})/1000.0;

	//bonus description
	auto & blist = art->getBonusList();

	*(cur++) = blist[0]->type/100.0;
	*(cur++) = blist[0]->subtype/10.0;
	*(cur++) = blist[0]->val/100.0;;
	*(cur++) = art->Attack()/10.0;
	*(cur++) = art->Defense()/10.0;
	*(cur++) = blist.valOfBonuses(Selector::type(Bonus::STACKS_SPEED))/5.0;
	*(cur++) = blist.valOfBonuses(Selector::type(Bonus::STACK_HEALTH))/10.0;

	return ret;
}

void SSN::save(const std::string &filename)
{
	net.save(filename);
}

SSN::~SSN()
{
}

void SSNRun()
{
	//buildLearningSet();


	auto examples = Framework::loadExamples(false);

	SSN network;
	network.learn(examples);
	network.save("network_config_file.net");
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
