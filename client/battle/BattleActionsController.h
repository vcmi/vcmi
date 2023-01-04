/*
 * BattleActionsController.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/battle/CBattleInfoCallback.h"

VCMI_LIB_NAMESPACE_BEGIN

class BattleAction;

VCMI_LIB_NAMESPACE_END

class BattleInterface;

enum class MouseHoveredHexContext
{
	UNOCCUPIED_HEX,
	OCCUPIED_HEX
};

/// Class that controls actions that can be performed by player, e.g. moving stacks, attacking, etc
/// As well as all relevant feedback for these actions in user interface
class BattleActionsController
{
	BattleInterface & owner;

	/// all actions possible to call at the moment by player
	std::vector<PossiblePlayerBattleAction> possibleActions;

	/// actions possible to take on hovered hex
	std::vector<PossiblePlayerBattleAction> localActions;

	/// these actions display message in case of illegal target
	std::vector<PossiblePlayerBattleAction> illegalActions;

	/// action that will be performed on l-click
	PossiblePlayerBattleAction currentAction;

	/// last action chosen (and saved) by player
	PossiblePlayerBattleAction selectedAction;

	/// if there are not possible actions to choose from, this action should be show as "illegal" in UI
	PossiblePlayerBattleAction illegalAction;

	/// if true, stack currently aims to cats a spell
	bool creatureCasting;

	/// if true, player is choosing destination for his spell - only for GUI / console
	bool spellDestSelectMode;

	/// spell for which player is choosing destination
	std::shared_ptr<BattleAction> spellToCast;

	/// spell for which player is choosing destination, pointer for convenience
	const CSpell *currentSpell;

	/// cached message that was set by this class in status bar
	std::string currentConsoleMsg;

	bool isCastingPossibleHere (const CStack *sactive, const CStack *shere, BattleHex myNumber);
	bool canStackMoveHere (const CStack *sactive, BattleHex MyNumber) const; //TODO: move to BattleState / callback
	std::vector<PossiblePlayerBattleAction> getPossibleActionsForStack (const CStack *stack) const; //called when stack gets its turn
	void reorderPossibleActionsPriority(const CStack * stack, MouseHoveredHexContext context);

public:
	BattleActionsController(BattleInterface & owner);

	/// initialize list of potential actions for new active stack
	void activateStack();

	/// initialize potential actions for spells that can be cast by active stack
	void enterCreatureCastingMode();

	/// initialize potential actions for selected spell
	void castThisSpell(SpellID spellID);

	/// ends casting spell (eg. when spell has been cast or canceled)
	void endCastingSpell();

	/// update UI (e.g. status bar/cursor) according to new active hex
	void handleHex(BattleHex myNumber, int eventType);

	/// returns currently selected spell or SpellID::NONE on error
	SpellID selectedSpell() const;

	/// returns true if UI is currently in target selection mode
	bool spellcastingModeActive() const;
	
	/// methods to work with array of possible actions, needed to control special creatures abilities
	const std::vector<PossiblePlayerBattleAction> & getPossibleActions() const;
	void removePossibleAction(PossiblePlayerBattleAction);
	
	/// inserts possible action in the beggining in order to prioritize it
	void pushFrontPossibleAction(PossiblePlayerBattleAction);

};
