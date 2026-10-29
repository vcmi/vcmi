/*
 * LastScenarios.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "LastScenarios.h"

#include "ScenarioListBuilder.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/StartInfo.h"

static SettingsStorage & getStorage()
{
	// written only by the server, so a server in a separate process does not overwrite changes of the client
	static SettingsStorage storage;
	static bool initialized = false;
	if(!initialized)
	{
		storage.init("config/serverSettings.json", "");
		initialized = true;
	}
	return storage;
}

static const std::map<ELoadMode, std::string> loadModeNames = {
	{ ELoadMode::SINGLE, "single" },
	{ ELoadMode::MULTI, "multiplayer" },
	{ ELoadMode::CAMPAIGN, "campaign" },
	{ ELoadMode::TUTORIAL, "tutorial" },
};

void LastScenarios::rememberSave(const ScenarioListEntry & entry)
{
	Settings lastSaves = getStorage().write["lastSaves"];
	lastSaves["newest"].String() = entry.originalFileURI;
	for(const auto & [loadMode, name] : loadModeNames)
		if(ScenarioListBuilder::isSaveCompatible(entry, loadMode))
			lastSaves[name].String() = entry.originalFileURI;
}

std::string LastScenarios::getSave(ELoadMode loadMode)
{
	const auto name = loadModeNames.find(loadMode);
	if(name == loadModeNames.end())
		return {};
	return getStorage()["lastSaves"][name->second].String();
}

void LastScenarios::rememberMap(const std::string & name)
{
	Settings lastMap = getStorage().write["lastMap"];
	lastMap->String() = name;
}

std::string LastScenarios::getMap()
{
	const auto & lastMap = getStorage()["lastMap"];
	return lastMap.isNull() ? "Maps/Arrogance" : lastMap.String();
}
