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
namespace spells {
class Caster;
}

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

	/// spell for which player's hero is choosing destination
	std::shared_ptr<BattleAction> heroSpellToCast;

	/// cached message that was set by this class in status bar
	std::string currentConsoleMsg;

	/// if true, active stack could possibly cast some target spell
	const CSpell * creatureSpellToCast;

	bool isCastingPossibleHere (const CStack *sactive, const CStack *shere, BattleHex myNumber);
	bool canStackMoveHere (const CStack *sactive, BattleHex MyNumber) const; //TODO: move to BattleState / callback
	std::vector<PossiblePlayerBattleAction> getPossibleActionsForStack (const CStack *stack) const; //called when stack gets its turn
	void reorderPossibleActionsPriority(const CStack * stack, MouseHoveredHexContext context);

	bool actionIsLegal(PossiblePlayerBattleAction action, BattleHex hoveredHex);

	void actionSetCursor(PossiblePlayerBattleAction action, BattleHex hoveredHex);
	void actionSetCursorBlocked(PossiblePlayerBattleAction action, BattleHex hoveredHex);

	std::string actionGetStatusMessage(PossiblePlayerBattleAction action, BattleHex hoveredHex);
	std::string actionGetStatusMessageBlocked(PossiblePlayerBattleAction action, BattleHex hoveredHex);

	void actionRealize(PossiblePlayerBattleAction action, BattleHex hoveredHex);

	PossiblePlayerBattleAction selectAction(BattleHex myNumber);

	const CStack * getStackForHex(BattleHex myNumber);

	/// attempts to initialize spellcasting action for stack
	/// will silently return if stack is not a spellcaster
	void tryActivateStackSpellcasting(const CStack *casterStack);

	const spells::Caster * getCurrentSpellcaster() const;
public:
	BattleActionsController(BattleInterface & owner);

	/// initialize list of potential actions for new active stack
	void activateStack();

	/// returns true if UI is currently in target selection mode
	bool spellcastingModeActive() const;

	/// enter targeted spellcasting mode for creature, e.g. via "F" hotkey
	void enterCreatureCastingMode();

	/// initialize hero spellcasting mode, e.g. on selecting spell in spellbook
	void castThisSpell(SpellID spellID);

	/// ends casting spell (eg. when spell has been cast or canceled)
	void endCastingSpell();

	/// update cursor and status bar according to new active hex
	void onHexHovered(BattleHex hoveredHex);

	/// performs action according to selected hex
	void onHexClicked(BattleHex clickedHex);

	/// returns spell that is currently being cast by hero or nullptr if none
	const CSpell * getHeroSpellToCast() const;

	/// if current stack is spellcaster, returns
	const CSpell * getStackSpellToCast( BattleHex targetHex ) const;
	const CSpell * getAnySpellToCast( BattleHex targetHex ) const;

	/// returns true if current stack is a spellcaster
	bool isActiveStackFixedSpellcaster() const;

	/// returns true if current stack is random spellcaster (e.g. Genie)
	bool isActiveStackRandomSpellcaster() const;

	/// methods to work with array of possible actions, needed to control special creatures abilities
	const std::vector<PossiblePlayerBattleAction> & getPossibleActions() const;
	void removePossibleAction(PossiblePlayerBattleAction);
	
	/// inserts possible action in the beggining in order to prioritize it
	void pushFrontPossibleAction(PossiblePlayerBattleAction);

};
