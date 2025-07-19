/*
 * ShortcutHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "ShortcutHandler.h"
#include "Shortcut.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/json/JsonUtils.h"

ShortcutHandler::ShortcutHandler()
{
	mappedKeyboardShortcuts = loadShortcuts(keyBindingsConfig["keyboard"]);
	mappedJoystickShortcuts = loadShortcuts(keyBindingsConfig["joystickButtons"]);
	mappedJoystickAxes = loadShortcuts(keyBindingsConfig["joystickAxes"]);

#ifndef ENABLE_GOLDMASTER
	std::vector<EShortcut> assignedShortcuts;
	std::vector<EShortcut> missingShortcuts;

	for (auto const & entry : keyBindingsConfig["keyboard"].Struct())
	{
		EShortcut shortcutID = findShortcut(entry.first);
		assert(!vstd::contains(assignedShortcuts, shortcutID));
		assignedShortcuts.push_back(shortcutID);
	}

	for (EShortcut id = vstd::next(EShortcut::NONE, 1); id < EShortcut::AFTER_LAST; id = vstd::next(id, 1))
		if (!vstd::contains(assignedShortcuts, id))
			missingShortcuts.push_back(id);

	if (!missingShortcuts.empty())
		logGlobal->error("Found %d shortcuts without config entry!", missingShortcuts.size());
#endif
}

std::multimap<std::string, EShortcut> ShortcutHandler::loadShortcuts(const JsonNode & data) const
{
	std::multimap<std::string, EShortcut> result;

	for (auto const & entry : data.Struct())
	{
		std::string shortcutName = entry.first;
		EShortcut shortcutID = findShortcut(shortcutName);

		if (shortcutID == EShortcut::NONE)
		{
			logGlobal->warn("Unknown shortcut '%s' found when loading shortcuts config!", shortcutName);
			continue;
		}

		if (entry.second.isString())
		{
			result.emplace(entry.second.String(), shortcutID);
		}

		if (entry.second.isVector())
		{
			for (auto const & entryVector : entry.second.Vector())
				result.emplace(entryVector.String(), shortcutID);
		}
	}

	return result;
}

std::vector<EShortcut> ShortcutHandler::translateShortcut(const std::multimap<std::string, EShortcut> & options, const std::string & key) const
{
	auto range = options.equal_range(key);

	// FIXME: some code expects calls to keyPressed / captureThisKey even without defined hotkeys
	if (range.first == range.second)
		return {EShortcut::NONE};

	std::vector<EShortcut> result;

	for (auto it = range.first; it != range.second; ++it)
		result.push_back(it->second);

	return result;
}

std::vector<EShortcut> ShortcutHandler::translateKeycode(const std::string & key) const
{
	return translateShortcut(mappedKeyboardShortcuts, key);
}

std::vector<EShortcut> ShortcutHandler::translateJoystickButton(const std::string & key) const
{
	return translateShortcut(mappedJoystickShortcuts, key);
}

std::vector<EShortcut> ShortcutHandler::translateJoystickAxis(const std::string & key) const
{
	return translateShortcut(mappedJoystickAxes, key);
}

EShortcut ShortcutHandler::findShortcut(const std::string & identifier ) const
{
	static const std::map<std::string, EShortcut> shortcutNames = {
		{"mouseClickLeft",           EShortcut::MOUSE_LEFT                },
		{"mouseClickRight",          EShortcut::MOUSE_RIGHT               },
		{"mouseCursorX",             EShortcut::MOUSE_CURSOR_X,           },
		{"mouseCursorY",             EShortcut::MOUSE_CURSOR_Y,           },
		{"mouseSwipeX",              EShortcut::MOUSE_SWIPE_X,            },
		{"mouseSwipeY",              EShortcut::MOUSE_SWIPE_Y,            },
		{"globalAccept",             EShortcut::GLOBAL_ACCEPT             },
		{"globalCancel",             EShortcut::GLOBAL_CANCEL             },
		{"globalReturn",             EShortcut::GLOBAL_RETURN             },
		{"globalFullscreen",         EShortcut::GLOBAL_FULLSCREEN         },
		{"globalOptions",            EShortcut::GLOBAL_OPTIONS            },
		{"globalBackspace",          EShortcut::GLOBAL_BACKSPACE          },
		{"globalMoveFocus",          EShortcut::GLOBAL_MOVE_FOCUS         },
		{"moveLeft",                 EShortcut::MOVE_LEFT                 },
		{"moveRight",                EShortcut::MOVE_RIGHT                },
		{"moveUp",                   EShortcut::MOVE_UP                   },
		{"moveDown",                 EShortcut::MOVE_DOWN                 },
		{"moveFirst",                EShortcut::MOVE_FIRST                },
		{"moveLast",                 EShortcut::MOVE_LAST                 },
		{"movePageUp",               EShortcut::MOVE_PAGE_UP              },
		{"movePageDown",             EShortcut::MOVE_PAGE_DOWN            },
		{"selectIndex1",             EShortcut::SELECT_INDEX_1            },
		{"selectIndex2",             EShortcut::SELECT_INDEX_2            },
		{"selectIndex3",             EShortcut::SELECT_INDEX_3            },
		{"selectIndex4",             EShortcut::SELECT_INDEX_4            },
		{"selectIndex5",             EShortcut::SELECT_INDEX_5            },
		{"selectIndex6",             EShortcut::SELECT_INDEX_6            },
		{"selectIndex7",             EShortcut::SELECT_INDEX_7            },
		{"selectIndex8",             EShortcut::SELECT_INDEX_8            },
		{"mainMenuNewGame",          EShortcut::MAIN_MENU_NEW_GAME        },
		{"mainMenuLoadGame",         EShortcut::MAIN_MENU_LOAD_GAME       },
		{"mainMenuHighScores",       EShortcut::MAIN_MENU_HIGH_SCORES     },
		{"mainMenuCredits",          EShortcut::MAIN_MENU_CREDITS         },
		{"mainMenuQuit",             EShortcut::MAIN_MENU_QUIT            },
		{"mainMenuBack",             EShortcut::MAIN_MENU_BACK            },
		{"mainMenuSingleplayer",     EShortcut::MAIN_MENU_SINGLEPLAYER    },
		{"mainMenuMultiplayer",      EShortcut::MAIN_MENU_MULTIPLAYER     },
		{"mainMenuCampaign",         EShortcut::MAIN_MENU_CAMPAIGN        },
		{"mainMenuTutorial",         EShortcut::MAIN_MENU_TUTORIAL        },
		{"mainMenuCampaignSod",      EShortcut::MAIN_MENU_CAMPAIGN_SOD    },
		{"mainMenuCampaignRoe",      EShortcut::MAIN_MENU_CAMPAIGN_ROE    },
		{"mainMenuCampaignAb",       EShortcut::MAIN_MENU_CAMPAIGN_AB     },
		{"mainMenuCampaignCustom",   EShortcut::MAIN_MENU_CAMPAIGN_CUSTOM },
		{"mainMenuCampaignChr",      EShortcut::MAIN_MENU_CAMPAIGN_CHR    },
		{"mainMenuCampaignHota",     EShortcut::MAIN_MENU_CAMPAIGN_HOTA   },
		{"mainMenuCampaignWog",      EShortcut::MAIN_MENU_CAMPAIGN_WOG    },
		{"mainMenuCampaignVCMI",     EShortcut::MAIN_MENU_CAMPAIGN_VCMI   },
		{"mainMenuLobby",            EShortcut::MAIN_MENU_LOBBY           },
		{"lobbyBeginStandardGame",   EShortcut::LOBBY_BEGIN_STANDARD_GAME },
		{"lobbyBeginCampaign",       EShortcut::LOBBY_BEGIN_CAMPAIGN      },
		{"lobbyLoadGame",            EShortcut::LOBBY_LOAD_GAME           },
		{"lobbySaveGame",            EShortcut::LOBBY_SAVE_GAME           },
		{"lobbyRandomMap",           EShortcut::LOBBY_RANDOM_MAP          },
		{"lobbyToggleChat",          EShortcut::LOBBY_TOGGLE_CHAT         },
		{"lobbyAdditionalOptions",   EShortcut::LOBBY_ADDITIONAL_OPTIONS  },
		{"lobbySelectScenario",      EShortcut::LOBBY_SELECT_SCENARIO     },
		{"gameEndTurn",              EShortcut::ADVENTURE_END_TURN        }, // compatibility ID - extra's use this string
		{"adventureEndTurn",         EShortcut::ADVENTURE_END_TURN        },
		{"adventureLoadGame",        EShortcut::ADVENTURE_LOAD_GAME       },
		{"adventureSaveGame",        EShortcut::ADVENTURE_SAVE_GAME       },
		{"adventureRestartGame",     EShortcut::ADVENTURE_RESTART_GAME    },
		{"adventureMainMenu",        EShortcut::ADVENTURE_TO_MAIN_MENU    },
		{"adventureQuitGame",        EShortcut::ADVENTURE_QUIT_GAME       },
		{"adventureMarketplace",     EShortcut::ADVENTURE_MARKETPLACE     },
		{"adventureThievesGuild",    EShortcut::ADVENTURE_THIEVES_GUILD   },
		{"gameActivateConsole",      EShortcut::GAME_ACTIVATE_CONSOLE     },
		{"adventureGameOptions",     EShortcut::ADVENTURE_GAME_OPTIONS    },
		{"adventureToggleGrid",      EShortcut::ADVENTURE_TOGGLE_GRID     },
		{"adventureToggleVisitable", EShortcut::ADVENTURE_TOGGLE_VISITABLE},
		{"adventureToggleBlocked",   EShortcut::ADVENTURE_TOGGLE_BLOCKED  },
		{"adventureToggleSleep",     EShortcut::ADVENTURE_TOGGLE_SLEEP    },
		{"adventureSetHeroAsleep",   EShortcut::ADVENTURE_SET_HERO_ASLEEP },
		{"adventureSetHeroAwake",    EShortcut::ADVENTURE_SET_HERO_AWAKE  },
		{"adventureMoveHero",        EShortcut::ADVENTURE_MOVE_HERO       },
		{"adventureVisitObject",     EShortcut::ADVENTURE_VISIT_OBJECT    },
		{"adventureMoveHeroSW",      EShortcut::ADVENTURE_MOVE_HERO_SW    },
		{"adventureMoveHeroSS",      EShortcut::ADVENTURE_MOVE_HERO_SS    },
		{"adventureMoveHeroSE",      EShortcut::ADVENTURE_MOVE_HERO_SE    },
		{"adventureMoveHeroWW",      EShortcut::ADVENTURE_MOVE_HERO_WW    },
		{"adventureMoveHeroEE",      EShortcut::ADVENTURE_MOVE_HERO_EE    },
		{"adventureMoveHeroNW",      EShortcut::ADVENTURE_MOVE_HERO_NW    },
		{"adventureMoveHeroNN",      EShortcut::ADVENTURE_MOVE_HERO_NN    },
		{"adventureMoveHeroNE",      EShortcut::ADVENTURE_MOVE_HERO_NE    },
		{"adventureViewSelected",    EShortcut::ADVENTURE_VIEW_SELECTED   },
		{"adventureNextObject",      EShortcut::ADVENTURE_NEXT_OBJECT     },
		{"adventureNextTown",        EShortcut::ADVENTURE_NEXT_TOWN       },
		{"adventureNextHero",        EShortcut::ADVENTURE_NEXT_HERO       },
		{"adventureFirstTown",       EShortcut::ADVENTURE_FIRST_TOWN      },
		{"adventureFirstHero",       EShortcut::ADVENTURE_FIRST_HERO      },
		{"adventureViewScenario",    EShortcut::ADVENTURE_VIEW_SCENARIO   },
		{"adventureDigGrail",        EShortcut::ADVENTURE_DIG_GRAIL       },
		{"adventureViewPuzzle",      EShortcut::ADVENTURE_VIEW_PUZZLE     },
		{"adventureViewWorld",       EShortcut::ADVENTURE_VIEW_WORLD      },
		{"adventureViewWorld1",      EShortcut::ADVENTURE_VIEW_WORLD_X1   },
		{"adventureViewWorld2",      EShortcut::ADVENTURE_VIEW_WORLD_X2   },
		{"adventureViewWorld4",      EShortcut::ADVENTURE_VIEW_WORLD_X4   },
		{"adventureTrackHero",       EShortcut::ADVENTURE_TRACK_HERO,     },
		{"adventureToggleMapLevel",  EShortcut::ADVENTURE_TOGGLE_MAP_LEVEL},
		{"adventureKingdomOverview", EShortcut::ADVENTURE_KINGDOM_OVERVIEW},
		{"adventureQuestLog",        EShortcut::ADVENTURE_QUEST_LOG       },
		{"adventureCastSpell",       EShortcut::ADVENTURE_CAST_SPELL      },
		{"adventureThievesGuild",    EShortcut::ADVENTURE_THIEVES_GUILD   },
		{"adventureExitWorldView",   EShortcut::ADVENTURE_EXIT_WORLD_VIEW },
		{"adventureZoomIn",          EShortcut::ADVENTURE_ZOOM_IN         },
		{"adventureZoomOut",         EShortcut::ADVENTURE_ZOOM_OUT        },
		{"adventureZoomReset",       EShortcut::ADVENTURE_ZOOM_RESET      },
		{"adventureSearch",          EShortcut::ADVENTURE_SEARCH          },
		{"adventureSearchContinue",  EShortcut::ADVENTURE_SEARCH_CONTINUE },
		{"battleToggleHeroesStats",  EShortcut::BATTLE_TOGGLE_HEROES_STATS},
		{"battleToggleQueue",        EShortcut::BATTLE_TOGGLE_QUEUE       },
		{"battleUseCreatureSpell",   EShortcut::BATTLE_USE_CREATURE_SPELL },
		{"battleSurrender",          EShortcut::BATTLE_SURRENDER          },
		{"battleRetreat",            EShortcut::BATTLE_RETREAT            },
		{"battleAutocombat",         EShortcut::BATTLE_AUTOCOMBAT         },
		{"battleAutocombatEnd",      EShortcut::BATTLE_END_WITH_AUTOCOMBAT},
		{"battleCastSpell",          EShortcut::BATTLE_CAST_SPELL         },
		{"battleWait",               EShortcut::BATTLE_WAIT               },
		{"battleDefend",             EShortcut::BATTLE_DEFEND             },
		{"battleConsoleUp",          EShortcut::BATTLE_CONSOLE_UP         },
		{"battleConsoleDown",        EShortcut::BATTLE_CONSOLE_DOWN       },
		{"battleTacticsNext",        EShortcut::BATTLE_TACTICS_NEXT       },
		{"battleTacticsEnd",         EShortcut::BATTLE_TACTICS_END        },
		{"battleToggleQuickSpell",   EShortcut::BATTLE_TOGGLE_QUICKSPELL   },
		{"battleSpellShortcut0",     EShortcut::BATTLE_SPELL_SHORTCUT_0   },
		{"battleSpellShortcut1",     EShortcut::BATTLE_SPELL_SHORTCUT_1   },
		{"battleSpellShortcut2",     EShortcut::BATTLE_SPELL_SHORTCUT_2   },
		{"battleSpellShortcut3",     EShortcut::BATTLE_SPELL_SHORTCUT_3   },
		{"battleSpellShortcut4",     EShortcut::BATTLE_SPELL_SHORTCUT_4   },
		{"battleSpellShortcut5",     EShortcut::BATTLE_SPELL_SHORTCUT_5   },
		{"battleSpellShortcut6",     EShortcut::BATTLE_SPELL_SHORTCUT_6   },
		{"battleSpellShortcut7",     EShortcut::BATTLE_SPELL_SHORTCUT_7   },
		{"battleSpellShortcut8",     EShortcut::BATTLE_SPELL_SHORTCUT_8   },
		{"battleSpellShortcut9",     EShortcut::BATTLE_SPELL_SHORTCUT_9   },
		{"battleSpellShortcut10",    EShortcut::BATTLE_SPELL_SHORTCUT_10  },
		{"battleSpellShortcut11",    EShortcut::BATTLE_SPELL_SHORTCUT_11  },
		{"spectateTrackHero",        EShortcut::SPECTATE_TRACK_HERO       },
		{"spectateSkipBattle",       EShortcut::SPECTATE_SKIP_BATTLE      },
		{"spectateSkipBattleResult", EShortcut::SPECTATE_SKIP_BATTLE_RESULT },
		{"townOpenTavern",           EShortcut::TOWN_OPEN_TAVERN          },
		{"townSwapArmies",           EShortcut::TOWN_SWAP_ARMIES          },
		{"recruitmentMax",           EShortcut::RECRUITMENT_MAX           },
		{"recruitmentMin",           EShortcut::RECRUITMENT_MIN           },
		{"recruitmentUpgrade",       EShortcut::RECRUITMENT_UPGRADE       },
		{"recruitmentUpgradeAll",    EShortcut::RECRUITMENT_UPGRADE_ALL   },
		{"kingdomHeroesTab",         EShortcut::KINGDOM_HEROES_TAB        },
		{"kingdomTownsTab",          EShortcut::KINGDOM_TOWNS_TAB         },
		{"heroDismiss",              EShortcut::HERO_DISMISS              },
		{"heroCommander",            EShortcut::HERO_COMMANDER            },
		{"heroLooseFormation",       EShortcut::HERO_LOOSE_FORMATION      },
		{"heroTightFormation",       EShortcut::HERO_TIGHT_FORMATION      },
		{"heroToggleTactics",        EShortcut::HERO_TOGGLE_TACTICS       },
		{"heroCostumeSave0",         EShortcut::HERO_COSTUME_SAVE_0       },
		{"heroCostumeSave1",         EShortcut::HERO_COSTUME_SAVE_1       },
		{"heroCostumeSave2",         EShortcut::HERO_COSTUME_SAVE_2       },
		{"heroCostumeSave3",         EShortcut::HERO_COSTUME_SAVE_3       },
		{"heroCostumeSave4",         EShortcut::HERO_COSTUME_SAVE_4       },
		{"heroCostumeSave5",         EShortcut::HERO_COSTUME_SAVE_5       },
		{"heroCostumeSave6",         EShortcut::HERO_COSTUME_SAVE_6       },
		{"heroCostumeSave7",         EShortcut::HERO_COSTUME_SAVE_7       },
		{"heroCostumeSave8",         EShortcut::HERO_COSTUME_SAVE_8       },
		{"heroCostumeSave9",         EShortcut::HERO_COSTUME_SAVE_9       },
		{"heroCostumeLoad0",         EShortcut::HERO_COSTUME_LOAD_0       },
		{"heroCostumeLoad1",         EShortcut::HERO_COSTUME_LOAD_1       },
		{"heroCostumeLoad2",         EShortcut::HERO_COSTUME_LOAD_2       },
		{"heroCostumeLoad3",         EShortcut::HERO_COSTUME_LOAD_3       },
		{"heroCostumeLoad4",         EShortcut::HERO_COSTUME_LOAD_4       },
		{"heroCostumeLoad5",         EShortcut::HERO_COSTUME_LOAD_5       },
		{"heroCostumeLoad6",         EShortcut::HERO_COSTUME_LOAD_6       },
		{"heroCostumeLoad7",         EShortcut::HERO_COSTUME_LOAD_7       },
		{"heroCostumeLoad8",         EShortcut::HERO_COSTUME_LOAD_8       },
		{"heroCostumeLoad9",         EShortcut::HERO_COSTUME_LOAD_9       },
		{"spellbookTabAdventure",    EShortcut::SPELLBOOK_TAB_ADVENTURE   },
		{"spellbookTabCombat",       EShortcut::SPELLBOOK_TAB_COMBAT      },
		{"spellbookSearchFocus",     EShortcut::SPELLBOOK_SEARCH_FOCUS    },
		{"listHeroUp",               EShortcut::LIST_HERO_UP              },
		{"listHeroDown",             EShortcut::LIST_HERO_DOWN            },
		{"listHeroTop",              EShortcut::LIST_HERO_TOP             },
		{"listHeroBottom",           EShortcut::LIST_HERO_BOTTOM          },
		{"listHeroDismiss",          EShortcut::LIST_HERO_DISMISS         },
		{"listTownUp",               EShortcut::LIST_TOWN_UP              },
		{"listTownDown",             EShortcut::LIST_TOWN_DOWN            },
		{"listTownTop",              EShortcut::LIST_TOWN_TOP             },
		{"listTownBottom",           EShortcut::LIST_TOWN_BOTTOM          },
		{"mainMenuHotseat",          EShortcut::MAIN_MENU_HOTSEAT         },
		{"mainMenuHostGame",         EShortcut::MAIN_MENU_HOST_GAME       },
		{"mainMenuJoinGame",         EShortcut::MAIN_MENU_JOIN_GAME       },
		{"highScoresCampaigns",      EShortcut::HIGH_SCORES_CAMPAIGNS     },
		{"highScoresScenarios",      EShortcut::HIGH_SCORES_SCENARIOS     },
		{"highScoresReset",          EShortcut::HIGH_SCORES_RESET         },
		{"highScoresStatistics",     EShortcut::HIGH_SCORES_STATISTICS    },
		{"lobbyReplayVideo",         EShortcut::LOBBY_REPLAY_VIDEO        },
		{"lobbyExtraOptions",        EShortcut::LOBBY_EXTRA_OPTIONS       },
		{"lobbyTurnOptions",         EShortcut::LOBBY_TURN_OPTIONS        },
		{"lobbyInvitePlayers",       EShortcut::LOBBY_INVITE_PLAYERS      },
		{"lobbyFlipCoin",            EShortcut::LOBBY_FLIP_COIN           },
		{"lobbyRandomTown",          EShortcut::LOBBY_RANDOM_TOWN         },
		{"lobbyRandomTownVs",        EShortcut::LOBBY_RANDOM_TOWN_VS      },
		{"lobbyHandicap",            EShortcut::LOBBY_HANDICAP            },
		{"mapsSizeS",                EShortcut::MAPS_SIZE_S               },
		{"mapsSizeM",                EShortcut::MAPS_SIZE_M               },
		{"mapsSizeL",                EShortcut::MAPS_SIZE_L               },
		{"mapsSizeXl",               EShortcut::MAPS_SIZE_XL              },
		{"mapsSizeAll",              EShortcut::MAPS_SIZE_ALL             },
		{"mapsSortPlayers",          EShortcut::MAPS_SORT_PLAYERS         },
		{"mapsSortSize",             EShortcut::MAPS_SORT_SIZE            },
		{"mapsSortFormat",           EShortcut::MAPS_SORT_FORMAT          },
		{"mapsSortName",             EShortcut::MAPS_SORT_NAME            },
		{"mapsSortVictory",          EShortcut::MAPS_SORT_VICTORY         },
		{"mapsSortDefeat",           EShortcut::MAPS_SORT_DEFEAT          },
		{"mapsSortMaps",             EShortcut::MAPS_SORT_MAPS            },
		{"mapsSortChangedate",       EShortcut::MAPS_SORT_CHANGEDATE      },
		{"settingsLoadGame",         EShortcut::SETTINGS_LOAD_GAME        },
		{"settingsSaveGame",         EShortcut::SETTINGS_SAVE_GAME        },
		{"settingsNewGame",          EShortcut::SETTINGS_NEW_GAME         },
		{"settingsRestartGame",      EShortcut::SETTINGS_RESTART_GAME     },
		{"settingsToMainMenu",       EShortcut::SETTINGS_TO_MAIN_MENU     },
		{"settingsQuitGame",         EShortcut::SETTINGS_QUIT_GAME        },
		{"adventureReplayTurn",      EShortcut::ADVENTURE_REPLAY_TURN     },
		{"adventureNewGame",         EShortcut::ADVENTURE_NEW_GAME        },
		{"battleOpenActiveUnit",     EShortcut::BATTLE_OPEN_ACTIVE_UNIT   },
		{"battleOpenHoveredUnit",    EShortcut::BATTLE_OPEN_HOVERED_UNIT  },
		{"marketDeal",               EShortcut::MARKET_DEAL               },
		{"marketMaxAmount",          EShortcut::MARKET_MAX_AMOUNT         },
		{"marketSacrificeAll",       EShortcut::MARKET_SACRIFICE_ALL      },
		{"marketSacrificeBackpack",  EShortcut::MARKET_SACRIFICE_BACKPACK },
		{"marketResourcePlayer",     EShortcut::MARKET_RESOURCE_PLAYER    },
		{"marketArtifactResource",   EShortcut::MARKET_ARTIFACT_RESOURCE  },
		{"marketResourceArtifact",   EShortcut::MARKET_RESOURCE_ARTIFACT  },
		{"marketCreatureResource",   EShortcut::MARKET_CREATURE_RESOURCE  },
		{"marketResourceResource",   EShortcut::MARKET_RESOURCE_RESOURCE  },
		{"marketCreatureExperience", EShortcut::MARKET_CREATURE_EXPERIENCE },
		{"marketArtifactExperience", EShortcut::MARKET_ARTIFACT_EXPERIENCE },
		{"townOpenHall",             EShortcut::TOWN_OPEN_HALL            },
		{"townOpenFort",             EShortcut::TOWN_OPEN_FORT            },
		{"townOpenMarket",           EShortcut::TOWN_OPEN_MARKET          },
		{"townOpenMageGuild",        EShortcut::TOWN_OPEN_MAGE_GUILD      },
		{"townOpenThievesGuild",     EShortcut::TOWN_OPEN_THIEVES_GUILD   },
		{"townOpenRecruitment",      EShortcut::TOWN_OPEN_RECRUITMENT     },
		{"townOpenHeroExchange",     EShortcut::TOWN_OPEN_HERO_EXCHANGE   },
		{"townOpenHero",             EShortcut::TOWN_OPEN_HERO            },
		{"townOpenVisitingHero",     EShortcut::TOWN_OPEN_VISITING_HERO   },
		{"townOpenGarrisonedHero",   EShortcut::TOWN_OPEN_GARRISONED_HERO },
		{"recruitmentSwitchLevel",   EShortcut::RECRUITMENT_SWITCH_LEVEL  },
		{"heroArmySplit",            EShortcut::HERO_ARMY_SPLIT           },
		{"heroBackpack",             EShortcut::HERO_BACKPACK             },
		{"exchangeArmyToLeft",       EShortcut::EXCHANGE_ARMY_TO_LEFT     },
		{"exchangeArmyToRight",      EShortcut::EXCHANGE_ARMY_TO_RIGHT    },
		{"exchangeArmySwap",         EShortcut::EXCHANGE_ARMY_SWAP        },
		{"exchangeArtifactsToLeft",  EShortcut::EXCHANGE_ARTIFACTS_TO_LEFT },
		{"exchangeArtifactsToRight", EShortcut::EXCHANGE_ARTIFACTS_TO_RIGHT },
		{"exchangeArtifactsSwap",    EShortcut::EXCHANGE_ARTIFACTS_SWAP   },
		{"exchangeBackpackLeft",     EShortcut::EXCHANGE_BACKPACK_LEFT    },
		{"exchangeBackpackRight",    EShortcut::EXCHANGE_BACKPACK_RIGHT   },
		{"exchangeEquippedToLeft",   EShortcut::EXCHANGE_EQUIPPED_TO_LEFT },
		{"exchangeEquippedToRight",  EShortcut::EXCHANGE_EQUIPPED_TO_RIGHT},
		{"exchangeEquippedSwap",     EShortcut::EXCHANGE_EQUIPPED_SWAP    },
		{"exchangeBackpackToLeft",   EShortcut::EXCHANGE_BACKPACK_TO_LEFT },
		{"exchangeBackpackToRight",  EShortcut::EXCHANGE_BACKPACK_TO_RIGHT},
		{"exchangeBackpackSwap",     EShortcut::EXCHANGE_BACKPACK_SWAP    },
	};

#ifndef ENABLE_GOLDMASTER
	std::vector<EShortcut> assignedShortcuts;
	std::vector<EShortcut> missingShortcuts;
	for (auto const & entry : shortcutNames)
		assignedShortcuts.push_back(entry.second);

	for (EShortcut id = vstd::next(EShortcut::NONE, 1); id < EShortcut::AFTER_LAST; id = vstd::next(id, 1))
		if (!vstd::contains(assignedShortcuts, id))
			missingShortcuts.push_back(id);

	if (!missingShortcuts.empty())
		logGlobal->error("Found %d shortcuts without assigned string name!", missingShortcuts.size());
#endif

	if (shortcutNames.count(identifier))
		return shortcutNames.at(identifier);
	return EShortcut::NONE;
}
