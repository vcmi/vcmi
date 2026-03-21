/*
 * RmgBenchmarkMain.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../lib/GameLibrary.h"
#include "../lib/callback/EditorCallback.h"
#include "../lib/json/JsonNode.h"
#include "../lib/mapping/CMap.h"
#include "../lib/modding/ModScope.h"
#include "../lib/rmg/CMapGenOptions.h"
#include "../lib/rmg/CMapGenerator.h"
#include "../lib/rmg/CRmgTemplate.h"
#include "../lib/rmg/CRmgTemplateStorage.h"
#include "../lib/logging/CLogger.h"
#include "../lib/serializer/JsonDeserializer.h"

#include <boost/algorithm/string.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/program_options.hpp>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <optional>
#include <string>
#include <vector>

#include <tbb/global_control.h>

namespace
{
namespace po = boost::program_options;

enum class SchedulerMode
{
	SINGLE,
	PARALLEL
};

struct BenchmarkOptions
{
	int width = 504;
	int height = 504;
	int levels = 4;
	int players = 2;
	int humanPlayers = 1;
	int compOnlyPlayers = 0;
	EWaterContent::EWaterContent water = EWaterContent::RANDOM;
	EMonsterStrength::EMonsterStrength monsters = EMonsterStrength::RANDOM;
	int seed = 1;
	int seedStep = 0;
	std::time_t creationDateTime = std::time(nullptr);
	std::string templateId;
	std::string templatePath;
	std::string templateKey;
	std::string autoTemplate = "largest";
	int expectedZones = 200;
	int threads = 8;
	SchedulerMode scheduler = SchedulerMode::PARALLEL;
	int warmup = 2;
	int runs = 10;
	std::string outputCsv;
	bool listTemplates = false;
};

struct RunResult
{
	double milliseconds = 0.0;
	int seed = 0;
	size_t zoneCount = 0;
	size_t objectCount = 0;
	std::string templateId;
	std::string templateName;
};

[[nodiscard]] std::string csvEscape(const std::string & value)
{
	std::string result = "\"";
	for(char ch : value)
	{
		if(ch == '"')
			result += "\"\"";
		else
			result += ch;
	}
	result += '"';
	return result;
}

void ensureDevelopmentModeMarker()
{
	const bool hasDataDirectories = boost::filesystem::exists("config") && boost::filesystem::exists("Mods");
	const bool hasAnyBinary = boost::filesystem::exists("vcmiclient")
		|| boost::filesystem::exists("vcmiserver")
		|| boost::filesystem::exists("vcmilobby")
		|| boost::filesystem::exists("vcmieditor")
		|| boost::filesystem::exists("vcmi-rmg-bench");

	if(hasDataDirectories && !hasAnyBinary)
	{
		std::ofstream marker("vcmiserver", std::ios::app);
	}
}

void ensureResourceWorkingDirectory(const char * argv0)
{
	const auto executableDirectory = boost::filesystem::system_complete(argv0).parent_path();
	const auto initialDirectory = boost::filesystem::current_path();
	const auto runtimeDirectory = boost::dll::program_location().parent_path();

	for(const auto & candidate : {executableDirectory, runtimeDirectory, executableDirectory.parent_path(), initialDirectory})
	{
		if(boost::filesystem::exists(candidate / "config/filesystem.json") && boost::filesystem::exists(candidate / "Mods"))
		{
			boost::filesystem::current_path(candidate);
			break;
		}
	}

	ensureDevelopmentModeMarker();
}

EWaterContent::EWaterContent parseWater(std::string value)
{
	boost::algorithm::to_lower(value);
	if(value == "none")
		return EWaterContent::NONE;
	if(value == "normal")
		return EWaterContent::NORMAL;
	if(value == "islands")
		return EWaterContent::ISLANDS;
	if(value == "random")
		return EWaterContent::RANDOM;
	throw std::runtime_error("Invalid --water value: " + value + ". Expected random|none|normal|islands.");
}

EMonsterStrength::EMonsterStrength parseMonsters(std::string value)
{
	boost::algorithm::to_lower(value);
	if(value == "weak")
		return EMonsterStrength::GLOBAL_WEAK;
	if(value == "normal")
		return EMonsterStrength::GLOBAL_NORMAL;
	if(value == "strong")
		return EMonsterStrength::GLOBAL_STRONG;
	if(value == "random")
		return EMonsterStrength::RANDOM;
	throw std::runtime_error("Invalid --monsters value: " + value + ". Expected random|weak|normal|strong.");
}

SchedulerMode parseScheduler(std::string value)
{
	boost::algorithm::to_lower(value);
	if(value == "single")
		return SchedulerMode::SINGLE;
	if(value == "parallel")
		return SchedulerMode::PARALLEL;
	throw std::runtime_error("Invalid --scheduler value: " + value + ". Expected single|parallel.");
}

std::shared_ptr<CRmgTemplate> loadTemplateFromFile(const std::string & templatePath, std::string templateKey)
{
	std::ifstream input(templatePath, std::ios::binary);
	if(!input)
		throw std::runtime_error("Failed to open template file: " + templatePath);

	const std::string data((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
	if(data.empty())
		throw std::runtime_error("Template file is empty: " + templatePath);

	JsonNode root(reinterpret_cast<const std::byte *>(data.data()), data.size(), templatePath);
	root.setModScope(ModScope::scopeBuiltin(), true);

	JsonNode payload;
	std::string templateName;
	if(root.isStruct())
	{
		if(templateKey.empty())
		{
			if(root.Struct().size() != 1)
			{
				throw std::runtime_error("Template file contains multiple templates. Use --template-key to choose one.");
			}
			templateName = root.Struct().begin()->first;
			payload = root.Struct().begin()->second;
		}
		else
		{
			payload = root[templateKey];
			if(payload.isNull())
				throw std::runtime_error("Template key not found in file: " + templateKey);
			templateName = templateKey;
		}
	}
	else
	{
		payload = root;
		templateName = templateKey.empty() ? std::string("template") : templateKey;
	}

	payload.setModScope(ModScope::scopeBuiltin(), true);

	auto result = std::make_shared<CRmgTemplate>();
	result->setId("benchmark:" + templateName);
	result->setName(templateName);

	JsonDeserializer handler(nullptr, payload);
	result->serializeJson(handler);
	result->afterLoad();
	result->validate();

	return result;
}

const CRmgTemplate * pickLargestTemplate(const std::vector<const CRmgTemplate *> & templates)
{
	if(templates.empty())
		return nullptr;

	return *std::max_element(templates.begin(), templates.end(), [](const CRmgTemplate * lhs, const CRmgTemplate * rhs)
	{
		const auto lhsZones = lhs->getZones().size();
		const auto rhsZones = rhs->getZones().size();
		if(lhsZones != rhsZones)
			return lhsZones < rhsZones;

		return lhs->getId() < rhs->getId();
	});
}

void configurePlayers(CMapGenOptions & options, const BenchmarkOptions & benchmark)
{
	options.setHumanOrCpuPlayerCount(benchmark.players);
	options.setCompOnlyPlayerCount(benchmark.compOnlyPlayers);

	for(int i = 0; i < benchmark.players; ++i)
	{
		const EPlayerType playerType = i < benchmark.humanPlayers ? EPlayerType::HUMAN : EPlayerType::AI;
		options.setPlayerTypeForStandardPlayer(PlayerColor(i), playerType);
	}
}

RunResult runSingleGeneration(const BenchmarkOptions & benchmark, int seed, const std::shared_ptr<CRmgTemplate> & fileTemplate)
{
	const int maxParallelism = benchmark.scheduler == SchedulerMode::SINGLE ? 1 : benchmark.threads;
	tbb::global_control limitParallelism(tbb::global_control::max_allowed_parallelism, std::max(1, maxParallelism));

	CMapGenOptions options;
	options.setWidth(benchmark.width);
	options.setHeight(benchmark.height);
	options.setLevels(benchmark.levels);
	options.setWaterContent(benchmark.water);
	options.setMonsterStrength(benchmark.monsters);
	configurePlayers(options, benchmark);

	if(fileTemplate)
	{
		options.setMapTemplate(fileTemplate.get());
	}
	else if(!benchmark.templateId.empty())
	{
		const auto * selected = LIBRARY->tplh->getTemplate(benchmark.templateId);
		if(selected == nullptr)
			throw std::runtime_error("Template not found: " + benchmark.templateId);
		options.setMapTemplate(selected);
	}
	else
	{
		const auto candidates = options.getPossibleTemplates();
		if(candidates.empty())
		{
			throw std::runtime_error(
				"No compatible templates for map "
				+ std::to_string(benchmark.width) + "x"
				+ std::to_string(benchmark.height) + "x"
				+ std::to_string(benchmark.levels)
				+ " and selected player settings. Adjust map options, "
				  "enable mods with larger templates, or pass "
				  "--template-id/--template-path.");
		}
		if(benchmark.autoTemplate == "largest")
		{
			const auto * selected = pickLargestTemplate(candidates);
			options.setMapTemplate(selected);
		}
	}

	// Some object constructors expect a non-null callback while generating.
	CMap callbackMap(nullptr);
	EditorCallback callback(&callbackMap);
	CMapGenerator generator(options, &callback, seed);

	const auto * selectedTemplate = options.getMapTemplate();
	const auto started = std::chrono::steady_clock::now();
	auto generatedMap = generator.generate();
	const auto finished = std::chrono::steady_clock::now();
	generatedMap->creationDateTime = benchmark.creationDateTime;

	RunResult result;
	result.seed = seed;
	result.milliseconds = std::chrono::duration<double, std::milli>(finished - started).count();
	result.objectCount = generatedMap->objects.size();
	if(selectedTemplate)
	{
		result.templateId = selectedTemplate->getId();
		result.templateName = selectedTemplate->getName();
		result.zoneCount = selectedTemplate->getZones().size();
	}
	return result;
}

double percentileValue(std::vector<double> values, double percentile)
{
	if(values.empty())
		return 0.0;

	std::sort(values.begin(), values.end());
	const double position = percentile * static_cast<double>(values.size() - 1);
	const size_t lower = static_cast<size_t>(position);
	const size_t upper = std::min(lower + 1, values.size() - 1);
	const double fraction = position - static_cast<double>(lower);
	return values[lower] * (1.0 - fraction) + values[upper] * fraction;
}

void validateArguments(const BenchmarkOptions & benchmark)
{
	const int maxPlayers = PlayerColor::PLAYER_LIMIT_I;
	if(benchmark.width <= 0 || benchmark.height <= 0 || benchmark.levels <= 0)
		throw std::runtime_error("Map dimensions and levels must be positive.");
	if(benchmark.players <= 0)
		throw std::runtime_error("--players must be at least 1.");
	if(benchmark.humanPlayers < 0 || benchmark.humanPlayers > benchmark.players)
		throw std::runtime_error("--human-players must be in range [0, players].");
	if(benchmark.compOnlyPlayers < 0)
		throw std::runtime_error("--comp-only-players must not be negative.");
	if(benchmark.players + benchmark.compOnlyPlayers > maxPlayers)
		throw std::runtime_error("Total player count exceeds supported limit.");
	if(benchmark.threads <= 0)
		throw std::runtime_error("--threads must be at least 1.");
	if(benchmark.warmup < 0 || benchmark.runs <= 0)
		throw std::runtime_error("--warmup must be >= 0 and --runs must be >= 1.");
	if(!benchmark.templatePath.empty() && !benchmark.templateId.empty())
		throw std::runtime_error("Use either --template-id or --template-path, not both.");
	if(benchmark.autoTemplate != "largest" && benchmark.autoTemplate != "random")
		throw std::runtime_error("Invalid --auto-template value: " + benchmark.autoTemplate + ". Expected largest|random.");
}

void listTemplates()
{
	std::vector<const CRmgTemplate *> templates = LIBRARY->tplh->getTemplates();
	std::sort(templates.begin(), templates.end(), [](const CRmgTemplate * lhs, const CRmgTemplate * rhs)
	{
		return lhs->getId() < rhs->getId();
	});

	std::cout << "id\tzones\tminSize\tmaxSize\tname\n";
	for(const auto * t : templates)
	{
		const auto sizes = t->getMapSizes();
		std::cout << t->getId() << '\t'
			<< t->getZones().size() << '\t'
			<< sizes.first.x << 'x' << sizes.first.y << 'x' << sizes.first.z << '\t'
			<< sizes.second.x << 'x' << sizes.second.y << 'x' << sizes.second.z << '\t'
			<< t->getName() << '\n';
	}
}
}

int main(int argc, char * argv[])
{
	BenchmarkOptions benchmark;

	std::string waterArg = "random";
	std::string monstersArg = "random";
	std::string schedulerArg = "parallel";

	po::options_description options("vcmi-rmg-bench options");
	options.add_options()
		("help,h", "Show help")
		("list-templates", po::bool_switch(&benchmark.listTemplates), "List available RMG templates and exit")
		("width", po::value<int>(&benchmark.width)->default_value(benchmark.width), "Map width (default: 504)")
		("height", po::value<int>(&benchmark.height)->default_value(benchmark.height), "Map height (default: 504)")
		("levels", po::value<int>(&benchmark.levels)->default_value(benchmark.levels), "Map levels (default: 4)")
		("players", po::value<int>(&benchmark.players)->default_value(benchmark.players), "Standard players (human or AI)")
		("human-players", po::value<int>(&benchmark.humanPlayers)->default_value(benchmark.humanPlayers), "Human players among standard players")
		("comp-only-players", po::value<int>(&benchmark.compOnlyPlayers)->default_value(benchmark.compOnlyPlayers), "Computer-only players")
		("water", po::value<std::string>(&waterArg)->default_value(waterArg), "Water content: random|none|normal|islands")
		("monsters", po::value<std::string>(&monstersArg)->default_value(monstersArg), "Monster strength: random|weak|normal|strong")
		("seed", po::value<int>(&benchmark.seed)->default_value(benchmark.seed), "Base random seed")
		("seed-step", po::value<int>(&benchmark.seedStep)->default_value(benchmark.seedStep), "Seed increment between runs")
		("timestamp", po::value<std::time_t>(&benchmark.creationDateTime)->default_value(benchmark.creationDateTime), "Map creation timestamp")
		("template-id", po::value<std::string>(&benchmark.templateId)->default_value(benchmark.templateId), "RMG template id (for example vcmi:Clash of Dragons)")
		("template-path", po::value<std::string>(&benchmark.templatePath)->default_value(benchmark.templatePath), "Path to template json file")
		("template-key", po::value<std::string>(&benchmark.templateKey)->default_value(benchmark.templateKey), "Template key inside template json file")
		("auto-template", po::value<std::string>(&benchmark.autoTemplate)->default_value(benchmark.autoTemplate), "Auto template policy: largest|random")
		("expected-zones", po::value<int>(&benchmark.expectedZones)->default_value(benchmark.expectedZones), "Expected zone count for heavy-template checks")
		("scheduler", po::value<std::string>(&schedulerArg)->default_value(schedulerArg), "Scheduler mode: parallel|single")
		("threads", po::value<int>(&benchmark.threads)->default_value(benchmark.threads), "Max worker threads")
		("warmup", po::value<int>(&benchmark.warmup)->default_value(benchmark.warmup), "Warmup runs")
		("runs", po::value<int>(&benchmark.runs)->default_value(benchmark.runs), "Measured runs")
		("output-csv", po::value<std::string>(&benchmark.outputCsv)->default_value(benchmark.outputCsv), "Optional CSV output path");

	po::variables_map parsed;
	try
	{
		po::store(po::parse_command_line(argc, argv, options), parsed);
		po::notify(parsed);
	}
	catch(const std::exception & e)
	{
		std::cerr << "Failed to parse options: " << e.what() << '\n';
		std::cerr << options << '\n';
		return EXIT_FAILURE;
	}

	if(parsed.count("help"))
	{
		std::cout << options << '\n';
		return EXIT_SUCCESS;
	}

	try
	{
		benchmark.water = parseWater(waterArg);
		benchmark.monsters = parseMonsters(monstersArg);
		benchmark.scheduler = parseScheduler(schedulerArg);
		boost::algorithm::to_lower(benchmark.autoTemplate);
		validateArguments(benchmark);

		ensureResourceWorkingDirectory(argv[0]);

		std::unique_ptr<GameLibrary> library = std::make_unique<GameLibrary>();
		LIBRARY = library.get();

		// Benchmark runs focus on generation throughput, not log throughput.
		CLogger::getGlobalLogger()->setLevel(ELogLevel::WARN);

		LIBRARY->initializeFilesystem(false);
		LIBRARY->initializeLibrary();

		if(benchmark.listTemplates)
		{
			listTemplates();
			return EXIT_SUCCESS;
		}

		std::shared_ptr<CRmgTemplate> fileTemplate;
		if(!benchmark.templatePath.empty())
		{
			fileTemplate = loadTemplateFromFile(benchmark.templatePath, benchmark.templateKey);
			std::cout << "Loaded template from file: " << fileTemplate->getName()
				<< " (" << fileTemplate->getId() << ")\n";
		}

		std::vector<RunResult> warmups;
		std::vector<RunResult> runs;
		warmups.reserve(benchmark.warmup);
		runs.reserve(benchmark.runs);

		for(int i = 0; i < benchmark.warmup; ++i)
		{
			warmups.push_back(runSingleGeneration(benchmark, benchmark.seed + benchmark.seedStep * i, fileTemplate));
		}

		for(int i = 0; i < benchmark.runs; ++i)
		{
			runs.push_back(runSingleGeneration(benchmark, benchmark.seed + benchmark.seedStep * (benchmark.warmup + i), fileTemplate));
		}

		std::vector<double> measurements;
		measurements.reserve(runs.size());
		for(const auto & run : runs)
		{
			measurements.push_back(run.milliseconds);
		}

		const auto [minIt, maxIt] = std::minmax_element(measurements.begin(), measurements.end());
		const double mean = std::accumulate(measurements.begin(), measurements.end(), 0.0) / static_cast<double>(measurements.size());
		const double median = percentileValue(measurements, 0.50);
		const double p95 = percentileValue(measurements, 0.95);

		const auto & sample = runs.front();
		std::cout << std::fixed << std::setprecision(2);
		std::cout << "vcmi-rmg-bench summary\n";
		std::cout << "  scheduler: " << (benchmark.scheduler == SchedulerMode::SINGLE ? "single" : "parallel")
			<< ", threads: " << benchmark.threads << '\n';
		std::cout << "  map: " << benchmark.width << 'x' << benchmark.height << 'x' << benchmark.levels
			<< ", players: " << benchmark.players
			<< " (human " << benchmark.humanPlayers << "), comp-only: " << benchmark.compOnlyPlayers << '\n';
		std::cout << "  template: " << (sample.templateId.empty() ? std::string("<auto>") : sample.templateId)
			<< " (" << sample.templateName << "), zones: " << sample.zoneCount << '\n';
		std::cout << "  runs: " << benchmark.runs << ", warmup: " << benchmark.warmup << '\n';
		std::cout << "  min/mean/median/p95/max [ms]: "
			<< *minIt << " / " << mean << " / " << median << " / " << p95 << " / " << *maxIt << '\n';

		if(benchmark.expectedZones > 0 && static_cast<int>(sample.zoneCount) != benchmark.expectedZones)
		{
			std::cout << "  warning: expected zones " << benchmark.expectedZones
				<< ", actual zones " << sample.zoneCount << '\n';
		}

		if(!benchmark.outputCsv.empty())
		{
			std::ofstream csv(benchmark.outputCsv);
			if(!csv)
				throw std::runtime_error("Failed to open CSV output: " + benchmark.outputCsv);

			csv << "phase,index,seed,ms,template_id,template_name,zones,objects,scheduler,threads,width,height,levels,players,human_players,comp_only_players\n";

			int index = 0;
			for(const auto & run : warmups)
			{
				csv << "warmup," << index++ << ',' << run.seed << ',' << run.milliseconds << ','
					<< csvEscape(run.templateId) << ',' << csvEscape(run.templateName) << ','
					<< run.zoneCount << ',' << run.objectCount << ','
					<< (benchmark.scheduler == SchedulerMode::SINGLE ? "single" : "parallel") << ','
					<< benchmark.threads << ','
					<< benchmark.width << ',' << benchmark.height << ',' << benchmark.levels << ','
					<< benchmark.players << ',' << benchmark.humanPlayers << ',' << benchmark.compOnlyPlayers << '\n';
			}

			index = 0;
			for(const auto & run : runs)
			{
				csv << "run," << index++ << ',' << run.seed << ',' << run.milliseconds << ','
					<< csvEscape(run.templateId) << ',' << csvEscape(run.templateName) << ','
					<< run.zoneCount << ',' << run.objectCount << ','
					<< (benchmark.scheduler == SchedulerMode::SINGLE ? "single" : "parallel") << ','
					<< benchmark.threads << ','
					<< benchmark.width << ',' << benchmark.height << ',' << benchmark.levels << ','
					<< benchmark.players << ',' << benchmark.humanPlayers << ',' << benchmark.compOnlyPlayers << '\n';
			}
		}
	}
	catch(const std::exception & e)
	{
		std::cerr << "Benchmark failed: " << e.what() << '\n';
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
