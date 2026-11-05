/*
 * LastScenarios.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

class ScenarioListEntry;
enum class ELoadMode : ui8;

/// Maps and saved games last selected, written or loaded by the server, stored in config/serverSettings.json
namespace LastScenarios
{
/// Remembers saved game for every load mode in which it can be loaded
void rememberSave(const ScenarioListEntry & entry);

/// Returns name of the saved game last remembered for this load mode, empty if there is none or load mode does not load saves
std::string getSave(ELoadMode loadMode);

/// Remembers map selected in the list of maps
void rememberMap(const std::string & name);

/// Returns name of the map last selected in the list of maps
std::string getMap();
}
