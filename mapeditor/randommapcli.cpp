/*
 * randommapcli.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "randommapcli.h"
#include "editorstartup.h"
#include "mapcontroller.h"

#include "../lib/GameLibrary.h"
#include "../lib/callback/EditorCallback.h"
#include "../lib/constants/EntityIdentifiers.h"
#include "../lib/mapping/CMap.h"
#include "../lib/mapping/CMapService.h"
#include "../lib/rmg/CMapGenOptions.h"
#include "../lib/rmg/CMapGenerator.h"
#include "../lib/rmg/CRmgTemplate.h"
#include "../lib/rmg/CRmgTemplateStorage.h"
#include "../lib/rmg/Functions.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFileInfo>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <ctime>
#include <iostream>
#include <limits>
#include <optional>
#include <string_view>

void addRandomMapOptions(QCommandLineParser & parser)
{
	parser.addOptions({
		{"generate", QCoreApplication::translate("main", "Generate a random map and save it to <file> (no window)."), "file"},
		{"template", QCoreApplication::translate("main", "Random map template (id, name, or substring)."), "text"},
		{"width", QCoreApplication::translate("main", "Map width in tiles. Must be given together with --height."), "n"},
		{"height", QCoreApplication::translate("main", "Map height in tiles. Must be given together with --width."), "n"},
		{"levels", QCoreApplication::translate("main", "Number of map levels."), "n"},
		{"players", QCoreApplication::translate("main", "Human-or-CPU player count."), "n"},
		{"computer-players", QCoreApplication::translate("main", "Computer-only player count."), "n"},
		{"water", QCoreApplication::translate("main", "Water content: random, none, normal, islands. Default: none, or the template's choice when it requires water."), "v"},
		{"monsters", QCoreApplication::translate("main", "Monster strength: random, weak, normal, strong."), "v"},
		{"seed", QCoreApplication::translate("main", "RNG seed."), "n"},
		{"single-thread", QCoreApplication::translate("main", "Run the random map generator on one thread.")},
	});
}

bool isRandomMapCommand(int argc, char * argv[])
{
	for(int i = 1; i < argc; ++i)
	{
		const char * arg = argv[i];
		if(std::strcmp(arg, "--generate") == 0)
			return true;
		if(std::strncmp(arg, "--generate=", 11) == 0)
			return true;
	}
	return false;
}

namespace
{
int failArgs(const std::string & message)
{
	std::cerr << message << std::endl;
	return 2;
}

int failRun(const std::string & message)
{
	std::cerr << message << std::endl;
	return 1;
}

template<typename T>
using NamedValues = std::array<std::pair<std::string_view, T>, 4>;

const NamedValues<EWaterContent::EWaterContent> waterNames = {{
	{"random", EWaterContent::RANDOM},
	{"none", EWaterContent::NONE},
	{"normal", EWaterContent::NORMAL},
	{"islands", EWaterContent::ISLANDS},
}};

const NamedValues<EMonsterStrength::EMonsterStrength> monsterNames = {{
	{"random", EMonsterStrength::RANDOM},
	{"weak", EMonsterStrength::GLOBAL_WEAK},
	{"normal", EMonsterStrength::GLOBAL_NORMAL},
	{"strong", EMonsterStrength::GLOBAL_STRONG},
}};

struct RandomMapRequest
{
	QString outputPath;
	std::optional<QString> templateQuery;
	std::optional<int> width;
	std::optional<int> height;
	std::optional<int> levels;
	std::optional<int> players;
	std::optional<int> computerPlayers;
	std::optional<EWaterContent::EWaterContent> water;
	std::optional<EMonsterStrength::EMonsterStrength> monsters;
	int seed = 0;
	bool singleThread = false;
};

/// Leaves value empty when the option is not given. False, with error set, when the value is invalid.
bool readInt(const QCommandLineParser & parser, const QString & name, int min, int max, std::optional<int> & value, std::string & error)
{
	if(!parser.isSet(name))
		return true;

	bool ok = false;
	const int parsed = parser.value(name).toInt(&ok);
	if(!ok)
	{
		error = "Invalid --" + name.toStdString() + " '" + parser.value(name).toStdString() + "'";
		return false;
	}
	if(parsed < min || parsed > max)
	{
		error = "--" + name.toStdString() + " must be in " + std::to_string(min) + ".." + std::to_string(max);
		return false;
	}
	value = parsed;
	return true;
}

template<typename T>
std::string joinNames(const NamedValues<T> & names)
{
	std::string result;
	for(const auto & [label, id] : names)
	{
		if(!result.empty())
			result += ", ";
		result += label;
	}
	return result;
}

/// Case-insensitive lookup in names. Leaves value empty when the option is not given.
template<typename T>
bool readEnum(const QCommandLineParser & parser, const QString & name, const NamedValues<T> & names, std::optional<T> & value, std::string & error)
{
	if(!parser.isSet(name))
		return true;

	const QString text = parser.value(name);
	for(const auto & [label, id] : names)
	{
		if(text.compare(QLatin1String(label.data(), static_cast<int>(label.size())), Qt::CaseInsensitive) == 0)
		{
			value = id;
			return true;
		}
	}
	error = "Invalid --" + name.toStdString() + " '" + text.toStdString() + "' (expected " + joinNames(names) + ")";
	return false;
}

std::string formatSize(const int3 & value)
{
	return std::to_string(value.x) + "x" + std::to_string(value.y) + "x" + std::to_string(value.z);
}

std::string formatWaterAllowed(const std::set<EWaterContent::EWaterContent> & allowed)
{
	std::string result;
	for(const auto & [label, id] : waterNames)
	{
		if(!allowed.count(id))
			continue;
		if(!result.empty())
			result += ", ";
		result += label;
	}
	return result;
}

std::vector<const CRmgTemplate *> findTemplates(const QString & query)
{
	std::vector<const CRmgTemplate *> exactId;
	std::vector<const CRmgTemplate *> exactName;
	std::vector<const CRmgTemplate *> substring;
	const auto queryStd = query.toStdString();

	for(const auto * templ : LIBRARY->tplh->getTemplates())
	{
		const QString id = QString::fromStdString(templ->getId());
		const QString name = QString::fromStdString(templ->getName());
		if(templ->getId() == queryStd)
			exactId.push_back(templ);
		if(name.compare(query, Qt::CaseInsensitive) == 0)
			exactName.push_back(templ);
		if(id.contains(query, Qt::CaseInsensitive) || name.contains(query, Qt::CaseInsensitive))
			substring.push_back(templ);
	}

	if(!exactId.empty())
		return exactId;
	if(!exactName.empty())
		return exactName;
	return substring;
}

std::string joinTemplateIds(const std::vector<const CRmgTemplate *> & templates)
{
	std::vector<std::string> ids;
	ids.reserve(templates.size());
	for(const auto * templ : templates)
		ids.push_back(templ->getId());
	std::sort(ids.begin(), ids.end());

	std::string result;
	for(const auto & id : ids)
	{
		if(!result.empty())
			result += ", ";
		result += id;
	}
	return result;
}

/// Exit code 2 when the command line is invalid, else 0.
int parseRequest(const QCommandLineParser & parser, RandomMapRequest & request)
{
	if(!parser.positionalArguments().isEmpty())
		return failArgs("Unexpected argument '" + parser.positionalArguments().at(0).toStdString() + "'");

	if(!parser.isSet("generate"))
		return failArgs("--generate is required");

	request.outputPath = QFileInfo(parser.value("generate")).absoluteFilePath();
	if(request.outputPath.isEmpty())
		return failArgs("--generate requires an output path");

	if(parser.isSet("width") != parser.isSet("height"))
		return failArgs("--width and --height must be given together");

	constexpr int intMin = std::numeric_limits<int>::min();
	constexpr int intMax = std::numeric_limits<int>::max();
	std::optional<int> seed;
	std::string error;
	const bool valid = readInt(parser, "width", 1, intMax, request.width, error)
		&& readInt(parser, "height", 1, intMax, request.height, error)
		&& readInt(parser, "levels", 1, intMax, request.levels, error)
		&& readInt(parser, "players", 1, PlayerColor::PLAYER_LIMIT_I, request.players, error)
		&& readInt(parser, "computer-players", 0, PlayerColor::PLAYER_LIMIT_I, request.computerPlayers, error)
		&& readEnum(parser, "water", waterNames, request.water, error)
		&& readEnum(parser, "monsters", monsterNames, request.monsters, error)
		&& readInt(parser, "seed", intMin, intMax, seed, error);
	if(!valid)
		return failArgs(error);

	request.seed = seed.value_or(static_cast<int>(std::time(nullptr)));
	if(parser.isSet("template"))
		request.templateQuery = parser.value("template");
	request.singleThread = parser.isSet("single-thread");
	return 0;
}

void applyRequest(CMapGenOptions & options, const RandomMapRequest & request)
{
	if(request.width)
		options.setWidth(*request.width);
	if(request.height)
		options.setHeight(*request.height);
	if(request.levels)
		options.setLevels(*request.levels);
	if(request.players)
		options.setHumanOrCpuPlayerCount(static_cast<si8>(*request.players));
	if(request.computerPlayers)
		options.setCompOnlyPlayerCount(static_cast<si8>(*request.computerPlayers));
	// Default to no water; setMapTemplate falls back to the template's allowed water when it forbids none.
	options.setWaterContent(request.water.value_or(EWaterContent::NONE));
	if(request.monsters)
		options.setMonsterStrength(*request.monsters);
}

/// setMapTemplate silently replaces values the template does not allow; exit code 2 if it replaced one the user gave.
int rejectIfTemplateChangedOptions(const CMapGenOptions & options, const CRmgTemplate & templ, const RandomMapRequest & request)
{
	const auto sizeRange = templ.getMapSizes();
	const std::string allowedSize = formatSize(sizeRange.first) + "-" + formatSize(sizeRange.second);

	if(request.width && options.getWidth() != *request.width)
		return failArgs("Template " + templ.getId() + " does not allow --width " + std::to_string(*request.width) + " (allowed size " + allowedSize + ")");
	if(request.height && options.getHeight() != *request.height)
		return failArgs("Template " + templ.getId() + " does not allow --height " + std::to_string(*request.height) + " (allowed size " + allowedSize + ")");
	if(request.levels && options.getLevels() != *request.levels)
		return failArgs("Template " + templ.getId() + " does not allow --levels " + std::to_string(*request.levels) + " (allowed size " + allowedSize + ")");
	if(request.players && options.getHumanOrCpuPlayerCount() != *request.players)
		return failArgs("Template " + templ.getId() + " does not allow --players " + std::to_string(*request.players) + " (allowed " + templ.getPlayers().toString() + ")");
	if(request.computerPlayers && options.getCompOnlyPlayerCount() != *request.computerPlayers)
		return failArgs("Template " + templ.getId() + " does not allow --computer-players " + std::to_string(*request.computerPlayers) + " (allowed players " + templ.getPlayers().toString() + ")");
	if(request.water && options.getWaterContent() != *request.water)
		return failArgs("Template " + templ.getId() + " does not allow --water (allowed " + formatWaterAllowed(templ.getWaterContentAllowed()) + ")");

	return 0;
}

/// Exit code 2 when no template fits, else 0.
int selectTemplate(CMapGenOptions & options, const RandomMapRequest & request)
{
	if(!request.templateQuery)
		return options.getPossibleTemplates().empty() ? failArgs("No template matches the requested options") : 0;

	const auto matches = findTemplates(*request.templateQuery);
	if(matches.empty())
		return failArgs("No template matches '" + request.templateQuery->toStdString() + "'");
	if(matches.size() > 1)
		return failArgs("Template '" + request.templateQuery->toStdString() + "' is ambiguous; matching ids: " + joinTemplateIds(matches));

	options.setMapTemplate(matches.front());
	return rejectIfTemplateChangedOptions(options, *matches.front(), request);
}

/// Exit code 1 when generation or saving throws, else 0 with the summary line filled in.
int generateAndSave(CMapGenOptions & options, const RandomMapRequest & request, std::string & summary)
{
	try
	{
		EditorCallback cb(nullptr);
		CMapGenerator generator(options, &cb, request.seed);
		if(request.singleThread)
			generator.setSingleThread(true);

		const auto started = std::chrono::steady_clock::now();
		std::unique_ptr<CMap> map = generator.generate();
		const auto generateMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count();

		map->mods = MapController::modAssessmentMap(*map);
		cb.setMap(map.get());
		const auto outPath = qstringToPath(request.outputPath);
		boost::filesystem::create_directories(outPath.parent_path());
		CMapService().saveMap(map, outPath);

		const auto * mapTemplate = generator.getMapGenOptions().getMapTemplate();
		summary = "rmg: saved " + request.outputPath.toStdString()
			+ " template=" + (mapTemplate ? mapTemplate->getId() : "")
			+ " seed=" + std::to_string(request.seed)
			+ " size=" + std::to_string(map->width) + "x" + std::to_string(map->height) + "x" + std::to_string(static_cast<int>(map->levels()))
			+ " generate_ms=" + std::to_string(generateMs);
	}
	catch(const rmgException & e)
	{
		return failRun(e.what());
	}
	catch(const std::runtime_error & e) // also boost::filesystem::filesystem_error
	{
		return failRun(e.what());
	}
	return 0;
}
}

int runRandomMapCommand(const QStringList & arguments)
{
	QCommandLineParser parser;
	parser.setApplicationDescription(QCoreApplication::translate("main", "Generate a random map without opening the editor window."));
	parser.addHelpOption();
	addRandomMapOptions(parser);

	if(!parser.parse(arguments))
		return failArgs(parser.errorText().toStdString());

	if(parser.isSet("help"))
	{
		std::cout << parser.helpText().toStdString() << std::endl;
		return 0;
	}

	RandomMapRequest request;
	if(const int rc = parseRequest(parser, request))
		return rc;

	std::string summary;
	{
		EditorStartup startup;
		if(!startup.initFilesystem(false))
			return failRun("Heroes III data or VCMI data was not found");
		startup.initLibrary();

		CMapGenOptions options;
		applyRequest(options, request);
		if(const int rc = selectTemplate(options, request))
			return rc;
		if(const int rc = generateAndSave(options, request, summary))
			return rc;
	}

	// Colored console logs write the reset sequence after the newline, so start a fresh line.
	std::cout << '\n' << summary << std::endl;
	return 0;
}
