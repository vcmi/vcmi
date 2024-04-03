/*
* GameControllerShortcuts.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#include "StdInc.h"
#include "GameControllerShortcuts.h"
#include "../gui/CGuiHandler.h"
#include "../gui/ShortcutHandler.h"


std::vector<EShortcut> getButtonBShortcuts()
{
    auto shortcuts =  GH.shortcuts().translateKeycode(SDLK_ESCAPE);
    // avoid someday ADVENTURE_EXIT_WORLD_VIEW be add in SDLK_ESCAPE shortcuts
    if(std::find(shortcuts.begin(), shortcuts.end(), EShortcut::ADVENTURE_EXIT_WORLD_VIEW) == shortcuts.end())
        shortcuts.push_back(EShortcut::ADVENTURE_EXIT_WORLD_VIEW);
    return shortcuts;
}

std::vector<EShortcut> getButtonXShortcuts()
{
    auto shortcuts =  GH.shortcuts().translateKeycode(SDLK_RETURN);
    // avoid someday ADVENTURE_EXIT_WORLD_VIEW be add in SDLK_RETURN shortcuts
    if(std::find(shortcuts.begin(), shortcuts.end(), EShortcut::ADVENTURE_EXIT_WORLD_VIEW) == shortcuts.end())
        shortcuts.push_back(EShortcut::ADVENTURE_EXIT_WORLD_VIEW);
    return shortcuts;
}

const ButtonShortcutsMap & getButtonShortcutsMap() {
    static const ButtonShortcutsMap buttonShortcutsMap =
    {
        // SDL_CONTROLLER_BUTTON_A for mouse left click
        {SDL_CONTROLLER_BUTTON_B, getButtonBShortcuts()},
        {SDL_CONTROLLER_BUTTON_X, getButtonXShortcuts()},
        // SDL_CONTROLLER_BUTTON_Y for mouse right click
        {SDL_CONTROLLER_BUTTON_LEFTSHOULDER, {EShortcut::ADVENTURE_NEXT_HERO, EShortcut::BATTLE_DEFEND}},
        {SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, {EShortcut::ADVENTURE_NEXT_TOWN, EShortcut::BATTLE_WAIT}},
        {SDL_CONTROLLER_BUTTON_BACK, {EShortcut::GAME_END_TURN, EShortcut::BATTLE_END_WITH_AUTOCOMBAT}},
        {SDL_CONTROLLER_BUTTON_START, {EShortcut::GLOBAL_OPTIONS, EShortcut::ADVENTURE_GAME_OPTIONS}},
        {SDL_CONTROLLER_BUTTON_DPAD_UP, {EShortcut::MOVE_UP, EShortcut::ADVENTURE_VIEW_WORLD,
                                         EShortcut::RECRUITMENT_UPGRADE,
                                         EShortcut::RECRUITMENT_UPGRADE_ALL,
                                         EShortcut::BATTLE_CONSOLE_UP, EShortcut::RECRUITMENT_MAX}},
        {SDL_CONTROLLER_BUTTON_DPAD_DOWN, {EShortcut::MOVE_DOWN, EShortcut::ADVENTURE_KINGDOM_OVERVIEW,
                                           EShortcut::BATTLE_CONSOLE_DOWN, EShortcut::RECRUITMENT_MIN}},
        {SDL_CONTROLLER_BUTTON_DPAD_LEFT, {EShortcut::MOVE_LEFT, EShortcut::ADVENTURE_VIEW_SCENARIO}},
        {SDL_CONTROLLER_BUTTON_DPAD_RIGHT, {EShortcut::MOVE_RIGHT, EShortcut::ADVENTURE_THIEVES_GUILD}},
        {SDL_CONTROLLER_BUTTON_LEFTSTICK, {EShortcut::ADVENTURE_TOGGLE_MAP_LEVEL,
                                           EShortcut::BATTLE_TOGGLE_HEROES_STATS}},
        {SDL_CONTROLLER_BUTTON_RIGHTSTICK, {EShortcut::ADVENTURE_TOGGLE_GRID, EShortcut::BATTLE_TOGGLE_QUEUE}}
    };
    return buttonShortcutsMap;
}

const TriggerShortcutsMap & getTriggerShortcutsMap()
{
    static const TriggerShortcutsMap triggerShortcutsMap = {
        {SDL_CONTROLLER_AXIS_TRIGGERLEFT, {EShortcut::ADVENTURE_VISIT_OBJECT, EShortcut::BATTLE_TACTICS_NEXT,
                                           EShortcut::BATTLE_USE_CREATURE_SPELL}},
        {SDL_CONTROLLER_AXIS_TRIGGERRIGHT, {EShortcut::ADVENTURE_CAST_SPELL, EShortcut::BATTLE_CAST_SPELL}}
    };
    return triggerShortcutsMap;
}