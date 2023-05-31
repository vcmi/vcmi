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

	void showOverview();
	void worldViewBack();
	void worldViewScale1x();
	void worldViewScale2x();
	void worldViewScale4x();
	void switchMapLevel();
	void showQuestlog();
	void toggleSleepWake();
	void setHeroSleeping();
	void setHeroAwake();
	void moveHeroAlongPath();
	void showSpellbook();
	void adventureOptions();
	void systemOptions();
	void nextHero();
	void endTurn();
	void showThievesGuild();
	void showScenarioInfo();
	void saveGame();
	void loadGame();
	void digGrail();
	void viewPuzzleMap();
	void restartGame();
	void visitObject();
	void openObject();
	void showMarketplace();
	void nextTown();
	void nextObject();
	void zoom( int distance);
	void moveHeroDirectional(const Point & direction);

public:
	explicit AdventureMapShortcuts(AdventureMapInterface & owner);

	std::vector<AdventureMapShortcutState> getShortcuts();

	bool optionCanViewQuests();
	bool optionCanToggleLevel();
	bool optionMapLevelSurface();
	bool optionHeroSleeping();
	bool optionHeroAwake();
	bool optionHeroSelected();
	bool optionHeroCanMove();
	bool optionHasNextHero();
	bool optionSpellcasting();
	bool optionInMapView();
	bool optionInWorldView();
	bool optionSidePanelActive();
	bool optionMapViewActive();

	void setState(EAdventureState newState);
	void onMapViewMoved(const Rect & visibleArea, int mapLevel);
};
