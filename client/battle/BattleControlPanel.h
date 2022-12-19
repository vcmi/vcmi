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
#include "../../lib/battle/CBattleInfoCallback.h"

VCMI_LIB_NAMESPACE_BEGIN
class CStack;

VCMI_LIB_NAMESPACE_END

class CButton;
class BattleInterface;
class BattleConsole;

/// GUI object that handles functionality of panel at the bottom of combat screen
class BattleControlPanel : public CIntObject
{
	BattleInterface & owner;

	std::shared_ptr<CPicture> menu;

	std::shared_ptr<CButton> bOptions;
	std::shared_ptr<CButton> bSurrender;
	std::shared_ptr<CButton> bFlee;
	std::shared_ptr<CButton> bAutofight;
	std::shared_ptr<CButton> bSwitchAction;
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

public:
	std::shared_ptr<BattleConsole> console;

	/// block all UI elements when player is not allowed to act, e.g. during enemy turn
	void blockUI(bool on);

	void show(SDL_Surface * to) override;
	void showAll(SDL_Surface * to) override;

	/// Toggle UI to displaying tactics phase
	void tacticPhaseStarted();

	/// Toggle UI to displaying battle log in place of tactics UI
	void tacticPhaseEnded();
	
	/// Set possible alternative options. If more than 1 - the last will be considered as default option
	void setAlternativeActions(const std::list<PossiblePlayerBattleAction> &);

	BattleControlPanel(BattleInterface & owner, const Point & position);
};

