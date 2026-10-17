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

#include "BattleActionEntry.h"

#include "../../lib/battle/CBattleInfoCallback.h"

class BattleAction;

class BattleInterface;

/// Class that controls actions that can be performed by player, e.g. moving stacks, attacking, etc
/// As well as all relevant feedback for these actions in user interface
class BattleActionsController
{
	BattleInterface & owner;
	
	/// all entries possible to select at the moment by player
	BattleActionEntries possibleActions;

	/// Preview of the entry selected for a hex
	struct HexSelection
	{
		BattleHex hex;
		ActionPreview preview;
	};

	/// Selection of the last queried hex, reused by the per-frame queries until the next hover or change of the controller state
	std::optional<HexSelection> cachedSelection;

	/// true while the player chooses the target of a hero spell
	bool heroSpellcasting = false;

	/// Destinations chosen so far for the target of the only remaining entry
	battle::Target partialTarget;

	/// Active unit, deactivated while the partial target is not empty
	const CStack * suspendedStack = nullptr;

	/// cached message that was set by this class in status bar
	std::string currentConsoleMsg;

	/// Spell that the active unit rolled for its turn out of its weighted SPELLCASTER bonuses, if any
	SpellID rolledCreatureSpell = SpellID::NONE;

	BattleActionEntries getPossibleActionsForStack (const CStack *stack); //called when stack gets its turn

	/// True if the entry wraps an option of the lib type of the action type
	static bool isLibAction(const IBattleActionEntry & entry, EActionType actionType);

	/// Highest priority entry that is legal for the hex, else the highest priority entry; nullptr if there are no entries
	std::shared_ptr<const IBattleActionEntry> selectEntry(const BattleHex & myNumber);

	/// Selection for the hex, taken from the cache when it holds the same hex
	const HexSelection & getSelection(const BattleHex & hex);

	/// Rolls the spell of the turn for a unit with weighted SPELLCASTER bonuses
	void tryActivateStackSpellcasting(const CStack *casterStack);

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

	/// Approaches of the legal entry selected for the hex, empty if it needs none
	std::vector<ActionApproach> getApproaches(const BattleHex & hoveredHex);

	/// Short presentation of the same legal action used by the pointer entry.
	std::string primaryActionText(const BattleHex & hex);
	std::string secondaryActionText(const BattleHex & hex, bool persistentInfo) const;

	/// Hexes highlighted as the result of clicking the hovered hex
	BattleHexArray getShadedHexes(const BattleHex & hoveredHex);

	/// Units affected by clicking the hovered hex, besides the unit on the hex
	std::vector<uint32_t> getAffectedUnits(const BattleHex & hoveredHex);

	/// enter targeted spellcasting mode for creature, e.g. via "F" hotkey
	void enterCreatureCastingMode();

	/// initialize hero spellcasting mode, e.g. on selecting spell in spellbook
	void castThisSpell(SpellID spellID);

	/// ends casting spell (eg. when spell has been cast or canceled)
	void endCastingSpell();

	const battle::Target & getPartialTarget() const;
	/// Adds a destination chosen for the target of the entry, which stays the only entry until its action is sent or cancelled
	void addTargetStep(const IBattleActionEntry & entry, const battle::Destination & destination);
	/// Sends the action of the unit, or of the hero if there is no actor, and leaves the target selection of a spell
	void sendAction(const BattleAction & action, const CStack * actor);

	/// update cursor and status bar according to new active hex
	void onHexHovered(const BattleHex & hoveredHex);
	void onHexRightHovered(const BattleHex & hoveredHex);

	/// called when cursor is no longer over battlefield and cursor/battle log should be reset
	void onHoverEnded();

	/// performs action according to selected hex
	void onHexLeftClicked(const BattleHex & clickedHex, const std::function<void(const CStack *)> & showStackInfo = {});

	/// performs action according to selected hex
	void onHexRightClicked(const BattleHex & clickedHex, const std::function<void(const CStack *)> & showStackInfo = {}, const std::function<void(const std::string &)> & showTextInfo = {});

	/// Replaces the entries and drops the cached selection
	void setEntries(BattleActionEntries entries);

	/// resets possible actions to original state
	void resetCurrentStackPossibleActions();
};
