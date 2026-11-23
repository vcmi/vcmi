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

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFileInfo>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <ctime>
#include <iostream>
#include <limits>
#include <optional>

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

bool parseIntOption(const QCommandLineParser & parser, const char * name, int min, int max, int & value, std::string & error)
{
	bool ok = false;
	const int parsed = parser.value(QLatin1String(name)).toInt(&ok);
	if(!ok)
	{
		error = std::string("Invalid --") + name + " '" + parser.value(QLatin1String(name)).toStdString() + "'";
		return false;
	}
	if(parsed < min || parsed > max)
	{
		error = std::string("--") + name + " must be in " + std::to_string(min) + ".." + std::to_string(max);
		return false;
	}
	value = parsed;
	return true;
}

bool parseWater(const QString & text, EWaterContent::EWaterContent & value, std::string & error)
{
	if(text.compare(QLatin1String("random"), Qt::CaseInsensitive) == 0)
		value = EWaterContent::RANDOM;
	else if(text.compare(QLatin1String("none"), Qt::CaseInsensitive) == 0)
		value = EWaterContent::NONE;
	else if(text.compare(QLatin1String("normal"), Qt::CaseInsensitive) == 0)
		value = EWaterContent::NORMAL;
	else if(text.compare(QLatin1String("islands"), Qt::CaseInsensitive) == 0)
		value = EWaterContent::ISLANDS;
	else
	{
		error = "Invalid --water '" + text.toStdString() + "' (expected random, none, normal, islands)";
		return false;
	}
	return true;
}

bool parseMonsters(const QString & text, EMonsterStrength::EMonsterStrength & value, std::string & error)
{
	if(text.compare(QLatin1String("random"), Qt::CaseInsensitive) == 0)
		value = EMonsterStrength::RANDOM;
	else if(text.compare(QLatin1String("weak"), Qt::CaseInsensitive) == 0)
		value = EMonsterStrength::GLOBAL_WEAK;
	else if(text.compare(QLatin1String("normal"), Qt::CaseInsensitive) == 0)
		value = EMonsterStrength::GLOBAL_NORMAL;
	else if(text.compare(QLatin1String("strong"), Qt::CaseInsensitive) == 0)
		value = EMonsterStrength::GLOBAL_STRONG;
	else
	{
		error = "Invalid --monsters '" + text.toStdString() + "' (expected random, weak, normal, strong)";
		return false;
	}
	return true;
}

std::string formatSize(const int3 & value)
{
	return std::to_string(value.x) + "x" + std::to_string(value.y) + "x" + std::to_string(value.z);
}

std::string formatWaterAllowed(const std::set<EWaterContent::EWaterContent> & allowed)
{
	static const std::pair<EWaterContent::EWaterContent, const char *> names[] = {
		{EWaterContent::NONE, "none"},
		{EWaterContent::NORMAL, "normal"},
		{EWaterContent::ISLANDS, "islands"},
	};

	std::string result;
	for(const auto & [id, name] : names)
	{
		if(!allowed.count(id))
			continue;
		if(!result.empty())
			result += ", ";
		result += name;
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

int rejectIfTemplateChangedOptions(
	const CMapGenOptions & options,
	const CRmgTemplate & templ,
	std::optional<int> requestedWidth,
	std::optional<int> requestedHeight,
	std::optional<int> requestedLevels,
	std::optional<int> requestedPlayers,
	std::optional<int> requestedComputerPlayers,
	std::optional<EWaterContent::EWaterContent> requestedWater)
{
	const auto sizeRange = templ.getMapSizes();
	const std::string allowedSize = formatSize(sizeRange.first) + "-" + formatSize(sizeRange.second);

	if(requestedWidth && options.getWidth() != *requestedWidth)
		return failArgs("Template " + templ.getId() + " does not allow --width " + std::to_string(*requestedWidth) + " (allowed size " + allowedSize + ")");
	if(requestedHeight && options.getHeight() != *requestedHeight)
		return failArgs("Template " + templ.getId() + " does not allow --height " + std::to_string(*requestedHeight) + " (allowed size " + allowedSize + ")");
	if(requestedLevels && options.getLevels() != *requestedLevels)
		return failArgs("Template " + templ.getId() + " does not allow --levels " + std::to_string(*requestedLevels) + " (allowed size " + allowedSize + ")");
	if(requestedPlayers && options.getHumanOrCpuPlayerCount() != *requestedPlayers)
		return failArgs("Template " + templ.getId() + " does not allow --players " + std::to_string(*requestedPlayers) + " (allowed " + templ.getPlayers().toString() + ")");
	if(requestedComputerPlayers && options.getCompOnlyPlayerCount() != *requestedComputerPlayers)
		return failArgs("Template " + templ.getId() + " does not allow --computer-players " + std::to_string(*requestedComputerPlayers) + " (allowed players " + templ.getPlayers().toString() + ")");
	if(requestedWater && options.getWaterContent() != *requestedWater)
		return failArgs("Template " + templ.getId() + " does not allow --water (allowed " + formatWaterAllowed(templ.getWaterContentAllowed()) + ")");

	return 0;
}
}

int runRandomMapCommand(QCoreApplication & app)
{
	QCommandLineParser parser;
	parser.setApplicationDescription(QCoreApplication::translate("main", "Generate a random map without opening the editor window."));
	parser.addHelpOption();
	addRandomMapOptions(parser);

	if(!parser.parse(app.arguments()))
		return failArgs(parser.errorText().toStdString());

	if(parser.isSet(QLatin1String("help")))
	{
		std::cout << parser.helpText().toStdString() << std::endl;
		return 0;
	}

	if(!parser.positionalArguments().isEmpty())
		return failArgs("Unexpected argument '" + parser.positionalArguments().at(0).toStdString() + "'");

	if(!parser.isSet(QLatin1String("generate")))
		return failArgs("--generate is required");

	const QString outputPath = QFileInfo(parser.value(QLatin1String("generate"))).absoluteFilePath();
	if(outputPath.isEmpty())
		return failArgs("--generate requires an output path");

	std::optional<int> width;
	std::optional<int> height;
	std::optional<int> levels;
	std::optional<int> players;
	std::optional<int> computerPlayers;
	std::optional<EWaterContent::EWaterContent> water;
	std::optional<EMonsterStrength::EMonsterStrength> monsters;
	int seed = static_cast<int>(std::time(nullptr));
	std::string error;

	if(parser.isSet(QLatin1String("width")) != parser.isSet(QLatin1String("height")))
		return failArgs("--width and --height must be given together");

	if(parser.isSet(QLatin1String("width")))
	{
		int value = 0;
		if(!parseIntOption(parser, "width", 1, std::numeric_limits<int>::max(), value, error))
			return failArgs(error);
		width = value;
	}
	if(parser.isSet(QLatin1String("height")))
	{
		int value = 0;
		if(!parseIntOption(parser, "height", 1, std::numeric_limits<int>::max(), value, error))
			return failArgs(error);
		height = value;
	}
	if(parser.isSet(QLatin1String("levels")))
	{
		int value = 0;
		if(!parseIntOption(parser, "levels", 1, std::numeric_limits<int>::max(), value, error))
			return failArgs(error);
		levels = value;
	}
	if(parser.isSet(QLatin1String("players")))
	{
		int value = 0;
		if(!parseIntOption(parser, "players", 1, PlayerColor::PLAYER_LIMIT_I, value, error))
			return failArgs(error);
		players = value;
	}
	if(parser.isSet(QLatin1String("computer-players")))
	{
		int value = 0;
		if(!parseIntOption(parser, "computer-players", 0, PlayerColor::PLAYER_LIMIT_I, value, error))
			return failArgs(error);
		computerPlayers = value;
	}
	if(parser.isSet(QLatin1String("water")))
	{
		EWaterContent::EWaterContent value = EWaterContent::RANDOM;
		if(!parseWater(parser.value(QLatin1String("water")), value, error))
			return failArgs(error);
		water = value;
	}
	if(parser.isSet(QLatin1String("monsters")))
	{
		EMonsterStrength::EMonsterStrength value = EMonsterStrength::RANDOM;
		if(!parseMonsters(parser.value(QLatin1String("monsters")), value, error))
			return failArgs(error);
		monsters = value;
	}
	if(parser.isSet(QLatin1String("seed")))
	{
		int value = 0;
		if(!parseIntOption(parser, "seed", std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), value, error))
			return failArgs(error);
		seed = value;
	}

	std::string summary;
	{
		EditorStartup startup;
		if(!startup.initFilesystem(false))
			return failRun("Heroes III data or VCMI data was not found");
		startup.initLibrary();

		CMapGenOptions options;
		if(width)
			options.setWidth(*width);
		if(height)
			options.setHeight(*height);
		if(levels)
			options.setLevels(*levels);
		if(players)
			options.setHumanOrCpuPlayerCount(static_cast<si8>(*players));
		if(computerPlayers)
			options.setCompOnlyPlayerCount(static_cast<si8>(*computerPlayers));
		// Default to no water; setMapTemplate falls back to the template's allowed water when it forbids none.
		options.setWaterContent(water.value_or(EWaterContent::NONE));
		if(monsters)
			options.setMonsterStrength(*monsters);

		if(parser.isSet(QLatin1String("template")))
		{
			const QString query = parser.value(QLatin1String("template"));
			const auto matches = findTemplates(query);
			if(matches.empty())
				return failArgs("No template matches '" + query.toStdString() + "'");
			if(matches.size() > 1)
				return failArgs("Template '" + query.toStdString() + "' is ambiguous; matching ids: " + joinTemplateIds(matches));

			options.setMapTemplate(matches.front());
			if(const int rc = rejectIfTemplateChangedOptions(options, *matches.front(), width, height, levels, players, computerPlayers, water))
				return rc;
		}
		else if(options.getPossibleTemplates().empty())
		{
			return failArgs("No template matches the requested options");
		}

		try
		{
			EditorCallback cb(nullptr);
			CMapGenerator generator(options, &cb, seed);
			if(parser.isSet(QLatin1String("single-thread")))
				generator.setSingleThread(true);

			const auto started = std::chrono::steady_clock::now();
			std::unique_ptr<CMap> map = generator.generate();
			const auto generateMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count();

			map->mods = MapController::modAssessmentMap(*map);
			cb.setMap(map.get());
			const auto outPath = qstringToPath(outputPath);
			boost::filesystem::create_directories(outPath.parent_path());
			CMapService().saveMap(map, outPath);

			const auto * mapTemplate = generator.getMapGenOptions().getMapTemplate();
			summary = "rmg: saved " + outputPath.toStdString()
				+ " template=" + (mapTemplate ? mapTemplate->getId() : "")
				+ " seed=" + std::to_string(seed)
				+ " size=" + std::to_string(map->width) + "x" + std::to_string(map->height) + "x" + std::to_string(static_cast<int>(map->levels()))
				+ " generate_ms=" + std::to_string(generateMs);
		}
		catch(const std::exception & e)
		{
			return failRun(e.what());
		}
	}

	// Colored console logs write the reset sequence after the newline, so start a fresh line.
	std::cout << '\n' << summary << std::endl;
	return 0;
}
