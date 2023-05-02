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
VCMI_LIB_NAMESPACE_END

enum class EShortcut;
class CAdventureMapInterface;

/// Class that contains list of functions for shortcuts available from adventure map
class AdventureMapShortcuts
{
	CAdventureMapInterface & owner;

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
	void viewWorldMap();
	void restartGame();
	void visitObject();
	void openObject();
	void abortSpellcasting();
	void showMarketplace();
	void nextTown();
	void nextObject();
	void moveHeroDirectional(const Point & direction);

public:
	explicit AdventureMapShortcuts(CAdventureMapInterface & owner);

	std::map<EShortcut, std::function<void()>> getFunctors();
};
