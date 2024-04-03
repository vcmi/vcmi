/*
* GameControllerShortcuts.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#include <map>
#include <SDL.h>
#include "../gui/Shortcut.h"

using ButtonShortcutsMap = std::map<int, std::vector<EShortcut> >;
using TriggerShortcutsMap = std::map<int, std::vector<EShortcut> >;

const ButtonShortcutsMap & getButtonShortcutsMap();
const TriggerShortcutsMap & getTriggerShortcutsMap();