/*
 * AdventureMapShortcuts.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../lib/constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN
class Point;
class Rect;
VCMI_LIB_NAMESPACE_END

enum class EShortcut;
class AdventureMapInterface;
enum class EAdventureState;

struct AdventureMapShortcutState
{
	EShortcut shortcut;
	bool isEnabled;
	std::function<void()> callback;
};

/// Class that contains list of functions for shortcuts available from adventure map
class AdventureMapShortcuts
{
	AdventureMapInterface & owner;
	EAdventureState state;
	int mapLevel;

	std::string searchLast;
	int searchPos;
	
	void showOverview();
	void worldViewBack();
	void worldViewScale1x();
	void worldViewScale2x();
	void worldViewScale4x();
	void viewStatistic();
	void switchMapLevel();
	void showQuestlog();
	void toggleTrackHero();
	void toggleGrid();
	void toggleVisitable();
	void toggleBlocked();
	void toggleSleepWake();
	void setHeroSleeping();
	void setHeroAwake();
	void moveHeroAlongPath();
	void showSpellbook();
	void adventureOptions();
	void systemOptions();
	void firstHero();
	void nextHero();
	void endTurn();
	void showThievesGuild();
	void showScenarioInfo();
	void toMainMenu();
	void newGame();
	void quitGame();
	void saveGame();
	void loadGame();
	void quickSaveGame();
	void quickLoadGame();
	void digGrail();
	void viewPuzzleMap();
	void restartGame();
	void visitObject();
	void openObject();
	void showMarketplace();
	void firstTown();
	void nextTown();
	void nextObject();
	void zoom( int distance);
	void search(bool next);
	void moveHeroDirectional(const Point & direction);

public:
	explicit AdventureMapShortcuts(AdventureMapInterface & owner);

	std::vector<AdventureMapShortcutState> getShortcuts();

	bool optionCanViewQuests();
	bool optionCanToggleLevel();
	int optionMapLevel();
	bool optionHeroSleeping();
	bool optionHeroAwake();
	bool optionHeroSelected();
	bool optionHeroCanMove();
	bool optionHasNextHero();
	bool optionCanVisitObject();
	bool optionCanEndTurn();
	bool optionSpellcasting();
	bool optionInMapView();
	bool optionInWorldView();
	bool optionSidePanelActive();
	bool optionMapScrollingActive();
	bool optionMapViewActive();
	bool optionMarketplace();
	bool optionHeroBoat(EPathfindingLayer layer);
	bool optionHeroDig();
	bool optionViewStatistic();
	bool optionIsLocal();
	bool optionQuickSaveLoad();

	void setState(EAdventureState newState);
	EAdventureState getState() const;
	void onMapViewMoved(const Rect & visibleArea, int mapLevel);
};
