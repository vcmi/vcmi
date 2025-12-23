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
enum class Mode;
}

VCMI_LIB_NAMESPACE_END

class BattleInterface;

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
	std::vector<const CSpell *> creatureSpells;

	/// stack that has been selected as first target for multi-target spells (Teleport & Sacrifice)
	const CStack * selectedStack;

	bool isCastingPossibleHere (const CSpell * spell, const CStack *shere, const BattleHex & myNumber);
	std::vector<PossiblePlayerBattleAction> getPossibleActionsForStack (const CStack *stack) const; //called when stack gets its turn
	void reorderPossibleActionsPriority(const CStack * stack, const CStack * targetStack);

	bool actionIsLegal(PossiblePlayerBattleAction action, const BattleHex & hoveredHex);

	void actionSetCursor(PossiblePlayerBattleAction action, const BattleHex & hoveredHex);
	void actionSetCursorBlocked(PossiblePlayerBattleAction action, const BattleHex & hoveredHex);

	std::string actionGetStatusMessage(PossiblePlayerBattleAction action, const BattleHex & hoveredHex);
	std::string actionGetStatusMessageBlocked(PossiblePlayerBattleAction action, const BattleHex & hoveredHex);

	void actionRealize(PossiblePlayerBattleAction action, const BattleHex & hoveredHex);

	PossiblePlayerBattleAction selectAction(const BattleHex & myNumber);

	const CStack * getStackForHex(const BattleHex & myNumber) ;

	/// attempts to initialize spellcasting action for stack
	/// will silently return if stack is not a spellcaster
	void tryActivateStackSpellcasting(const CStack *casterStack);

	/// returns spell that is currently being cast by hero or nullptr if none
	const CSpell * getHeroSpellToCast() const;

	/// if current stack is spellcaster, returns spell being cast, or null othervice
	const CSpell * getStackSpellToCast(const BattleHex & hoveredHex);

	/// returns true if current stack is a spellcaster
	bool isActiveStackSpellcaster() const;

public:
	BattleActionsController(BattleInterface & owner);

	/// initialize list of potential actions for new active stack
	void activateStack();

	/// returns true if UI is currently in hero spell target selection mode
	bool heroSpellcastingModeActive() const;
	/// returns true if UI is currently in "F" hotkey creature spell target selection mode
	bool creatureSpellcastingModeActive() const;

	/// returns true if one of the following is true:
	/// - we are casting spell by hero
	/// - we are casting spell by creature in targeted mode (F hotkey)
	/// - current creature is spellcaster and preferred action for current hex is spellcast
	bool currentActionSpellcasting(const BattleHex & hoveredHex);

	/// enter targeted spellcasting mode for creature, e.g. via "F" hotkey
	void enterCreatureCastingMode();

	/// initialize hero spellcasting mode, e.g. on selecting spell in spellbook
	void castThisSpell(SpellID spellID);

	/// ends casting spell (eg. when spell has been cast or canceled)
	void endCastingSpell();

	/// update cursor and status bar according to new active hex
	void onHexHovered(const BattleHex & hoveredHex);

	/// called when cursor is no longer over battlefield and cursor/battle log should be reset
	void onHoverEnded();

	/// performs action according to selected hex
	void onHexLeftClicked(const BattleHex & clickedHex);

	/// performs action according to selected hex
	void onHexRightClicked(const BattleHex & clickedHex);

	const spells::Caster * getCurrentSpellcaster() const;
	const CSpell * getCurrentSpell(const BattleHex & hoveredHex);
	spells::Mode getCurrentCastMode() const;

	/// methods to work with array of possible actions, needed to control special creatures abilities
	const std::vector<PossiblePlayerBattleAction> & getPossibleActions() const;
	
	/// sets list of high-priority actions that should be selected before any other actions
	void setPriorityActions(const std::vector<PossiblePlayerBattleAction> &);

	/// resets possible actions to original state
	void resetCurrentStackPossibleActions();
};
