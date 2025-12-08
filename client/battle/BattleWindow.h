/*
 * BattleWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"
#include "../gui/InterfaceObjectConfigurable.h"
#include "../../lib/battle/CBattleInfoCallback.h"
#include "../../lib/battle/PossiblePlayerBattleAction.h"

VCMI_LIB_NAMESPACE_BEGIN
class CStack;

VCMI_LIB_NAMESPACE_END

class CButton;
class BattleInterface;
class BattleConsole;
class BattleRenderer;
class StackQueue;
class TurnTimerWidget;
class HeroInfoBasicPanel;
class StackInfoBasicPanel;
class QuickSpellPanel;
class UnitActionPanel;

/// GUI object that handles functionality of panel at the bottom of combat screen
class BattleWindow : public InterfaceObjectConfigurable
{
	BattleInterface & owner;

	std::shared_ptr<StackQueue> queue;
	std::shared_ptr<BattleConsole> console;
	std::shared_ptr<HeroInfoBasicPanel> attackerHeroWindow;
	std::shared_ptr<HeroInfoBasicPanel> defenderHeroWindow;
	std::shared_ptr<StackInfoBasicPanel> attackerStackWindow;
	std::shared_ptr<StackInfoBasicPanel> defenderStackWindow;

	std::shared_ptr<QuickSpellPanel> quickSpellWindow;
	std::shared_ptr<UnitActionPanel> unitActionWindow;

	std::shared_ptr<TurnTimerWidget> attackerTimerWidget;
	std::shared_ptr<TurnTimerWidget> defenderTimerWidget;

	/// button press handling functions
	void bOptionsf();
	void bSurrenderf();
	void bFleef();
	void bAutofightf();
	void bSpellf();
	void bWaitf();
	void bDefencef();
	void bConsoleUpf();
	void bConsoleDownf();
	void bTacticNextStack();
	void bTacticPhaseEnd();
	void bOpenActiveUnit();
	void bOpenHoveredUnit();

	/// functions for handling actions after they were confirmed by popup window
	void reallyFlee();
	void reallySurrender();
	
	void useSpellIfPossible(int slot);

	/// flip battle queue visibility to opposite
	void toggleQueueVisibility();
	void createQueue();

	void toggleStickyHeroWindowsVisibility();
	void toggleStickyQuickSpellVisibility();
	void createStickyHeroInfoWindows();
	void createQuickSpellWindow();
	void createTimerInfoWindows();

	std::shared_ptr<BattleConsole> buildBattleConsole(const JsonNode &) const;

	bool onlyOnePlayerHuman;

	bool hasSpaceForQuickActions() const;
	bool quickActionsPanelActive() const;
	bool placeInfoWindowsOutside() const;

public:
	BattleWindow(BattleInterface & owner );

	/// Closes window once battle finished
	void close();

	/// Toggle StackQueue visibility
	void hideQueue();
	void showQueue();

	/// Toggle permanent hero info windows visibility (HD mod feature)
	void hideStickyHeroWindows();
	void showStickyHeroWindows();

	/// Toggle permanent quickspell windows visibility
	void hideStickyQuickSpellWindow();
	void showStickyQuickSpellWindow();

	/// Event handler for netpack changing hero mana points
	void heroManaPointsChanged(const CGHeroInstance * hero);

	/// block all UI elements when player is not allowed to act, e.g. during enemy turn
	void blockUI(bool on);

	/// Refresh queue after turn order changes
	void updateQueue();

	// Set positions for hero & stack info window
	void setPositionInfoWindow();

	/// Refresh sticky variant of hero info window after spellcast, side same as in BattleSpellCast::side
	void updateHeroInfoWindow(uint8_t side, const InfoAboutHero & hero);

	/// Refresh sticky variant of hero info window after spellcast, side same as in BattleSpellCast::side
	void updateStackInfoWindow(const CStack * stack);

	/// Get mouse-hovered battle queue unit ID if any found
	std::optional<uint32_t> getQueueHoveredUnitId();

	void activate() override;
	void deactivate() override;
	void keyPressed(EShortcut key) override;
	bool captureThisKey(EShortcut key) override;
	void clickPressed(const Point & cursorPosition) override;
	void show(Canvas & to) override;
	void showAll(Canvas & to) override;
	void onScreenResize() override;

	/// Toggle UI to displaying tactics phase
	void tacticPhaseStarted();

	/// Toggle UI to displaying battle log in place of tactics UI
	void tacticPhaseEnded();

	/// Set possible alternative options to fill unit actions panel
	void setPossibleActions(const std::vector<PossiblePlayerBattleAction> & allActions);

	/// ends battle with autocombat
	void endWithAutocombat();
};

