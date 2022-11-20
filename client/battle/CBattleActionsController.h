/*
 * CBattleActionsController.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/battle/CBattleInfoCallback.h"

class BattleAction;
class CBattleInterface;

enum class MouseHoveredHexContext
{
	UNOCCUPIED_HEX,
	OCCUPIED_HEX
};

class CBattleActionsController
{
	CBattleInterface * owner;

	std::vector<PossiblePlayerBattleAction> possibleActions; //all actions possible to call at the moment by player
	std::vector<PossiblePlayerBattleAction> localActions; //actions possible to take on hovered hex
	std::vector<PossiblePlayerBattleAction> illegalActions; //these actions display message in case of illegal target
	PossiblePlayerBattleAction currentAction; //action that will be performed on l-click
	PossiblePlayerBattleAction selectedAction; //last action chosen (and saved) by player
	PossiblePlayerBattleAction illegalAction; //most likely action that can't be performed here

	bool creatureCasting; //if true, stack currently aims to cats a spell
	bool spellDestSelectMode; //if true, player is choosing destination for his spell - only for GUI / console
	std::shared_ptr<BattleAction> spellToCast; //spell for which player is choosing destination
	const CSpell *sp; //spell pointer for convenience

	bool isCastingPossibleHere (const CStack *sactive, const CStack *shere, BattleHex myNumber);
	bool canStackMoveHere (const CStack *sactive, BattleHex MyNumber); //TODO: move to BattleState / callback

	std::vector<PossiblePlayerBattleAction> getPossibleActionsForStack (const CStack *stack); //called when stack gets its turn
	void reorderPossibleActionsPriority(const CStack * stack, MouseHoveredHexContext context);

public:
	CBattleActionsController(CBattleInterface * owner);

	void activateStack();
	void endCastingSpell(); //ends casting spell (eg. when spell has been cast or canceled)
	void enterCreatureCastingMode();

	SpellID selectedSpell();
	bool spellcastingModeActive();
	void castThisSpell(SpellID spellID); //called when player has chosen a spell from spellbook
	void handleHex(BattleHex myNumber, int eventType);

};
