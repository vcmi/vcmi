/*
 * BattleControlPanel.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

VCMI_LIB_NAMESPACE_BEGIN
class CStack;

VCMI_LIB_NAMESPACE_END

class CButton;
class BattleInterface;
class BattleConsole;
class BattleRenderer;
class StackQueue;

/// GUI object that handles functionality of panel at the bottom of combat screen
class BattleControlPanel : public WindowBase
{
	BattleInterface & owner;

	std::shared_ptr<StackQueue> queue;

	std::shared_ptr<BattleConsole> console;

	std::shared_ptr<CPicture> menuTactics;
	std::shared_ptr<CPicture> menuBattle;

	std::shared_ptr<CButton> bOptions;
	std::shared_ptr<CButton> bSurrender;
	std::shared_ptr<CButton> bFlee;
	std::shared_ptr<CButton> bAutofight;
	std::shared_ptr<CButton> bSpell;
	std::shared_ptr<CButton> bWait;
	std::shared_ptr<CButton> bDefence;
	std::shared_ptr<CButton> bConsoleUp;
	std::shared_ptr<CButton> bConsoleDown;
	std::shared_ptr<CButton> btactNext;
	std::shared_ptr<CButton> btactEnd;

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

	/// functions for handling actions after they were confirmed by popup window
	void reallyFlee();
	void reallySurrender();

	/// Toggle StackQueue visibility
	void hideQueue();
	void showQueue();

public:
	BattleControlPanel(BattleInterface & owner );

	/// Closes window once battle finished (explicit declaration to move into public visibility)
	using WindowBase::close;

	/// block all UI elements when player is not allowed to act, e.g. during enemy turn
	void blockUI(bool on);

	/// Refresh queue after turn order changes
	void updateQueue();

	void activate() override;
	void deactivate() override;
	void keyPressed(const SDL_KeyboardEvent & key) override;
	void clickRight(tribool down, bool previousState) override;
	void show(SDL_Surface *to) override;
	void showAll(SDL_Surface *to) override;

	/// Toggle UI to displaying tactics phase
	void tacticPhaseStarted();

	/// Toggle UI to displaying battle log in place of tactics UI
	void tacticPhaseEnded();
};

