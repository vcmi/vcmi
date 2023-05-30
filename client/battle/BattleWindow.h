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

/// GUI object that handles functionality of panel at the bottom of combat screen
class BattleWindow : public InterfaceObjectConfigurable
{
	BattleInterface & owner;

	std::shared_ptr<StackQueue> queue;
	std::shared_ptr<BattleConsole> console;

	/// button press handling functions
	void bOptionsf();
	void bSurrenderf();
	void bFleef();
	void bAutofightf();
	void bSpellf();
	void bWaitf();
	void bSwitchActionf();
	void bDefencef();
	void bConsoleUpf();
	void bConsoleDownf();
	void bTacticNextStack();
	void bTacticPhaseEnd();

	/// functions for handling actions after they were confirmed by popup window
	void reallyFlee();
	void reallySurrender();
	
	/// management of alternative actions
	std::list<PossiblePlayerBattleAction> alternativeActions;
	PossiblePlayerBattleAction defaultAction;
	void showAlternativeActionIcon(PossiblePlayerBattleAction);

	/// flip battle queue visibility to opposite
	void toggleQueueVisibility();
	void createQueue();

	std::shared_ptr<BattleConsole> buildBattleConsole(const JsonNode &) const;

public:
	BattleWindow(BattleInterface & owner );
	~BattleWindow();

	/// Closes window once battle finished
	void close();

	/// Toggle StackQueue visibility
	void hideQueue();
	void showQueue();

	/// block all UI elements when player is not allowed to act, e.g. during enemy turn
	void blockUI(bool on);

	/// Refresh queue after turn order changes
	void updateQueue();

	/// Get mouse-hovered battle queue unit ID if any found
	std::optional<uint32_t> getQueueHoveredUnitId();

	void activate() override;
	void deactivate() override;
	void keyPressed(EShortcut key) override;
	bool captureThisKey(EShortcut key) override;
	void show(Canvas & to) override;
	void showAll(Canvas & to) override;

	/// Toggle UI to displaying tactics phase
	void tacticPhaseStarted();

	/// Toggle UI to displaying battle log in place of tactics UI
	void tacticPhaseEnded();

	/// Set possible alternative options. If more than 1 - the last will be considered as default option
	void setAlternativeActions(const std::list<PossiblePlayerBattleAction> &);
};

