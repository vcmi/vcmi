/*
 * BattleOnlyMap.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

class BattleOnlyModeStartInfo;

/// Map on which battle-only mode is played, not shown in the scenario list
namespace BattleOnlyMap
{
extern const std::string fileURI;

/// Builds map with heroes, armies and battlefield of the start info and writes it to the user data directory as fileURI
void write(const BattleOnlyModeStartInfo & startInfo);
}
