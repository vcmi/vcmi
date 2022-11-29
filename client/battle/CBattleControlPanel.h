/*
 * CBattleControlPanel.h, part of VCMI engine
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
class CBattleInterface;
class CBattleConsole;

class CBattleControlPanel : public CIntObject
{
	CBattleInterface * owner;

	std::shared_ptr<CPicture> menu;

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

	//button handle funcs:
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

	void reallyFlee(); //performs fleeing without asking player
	void reallySurrender(); //performs surrendering without asking player

public:
	std::shared_ptr<CBattleConsole> console;

	// block all UI elements, e.g. during enemy turn
	// unlike activate/deactivate this method will correctly grey-out all elements
	void blockUI(bool on);

	void show(SDL_Surface * to) override;
	void showAll(SDL_Surface * to) override;

	void tacticPhaseStarted();
	void tacticPhaseEnded();

	CBattleControlPanel(CBattleInterface * owner, const Point & position);
};

