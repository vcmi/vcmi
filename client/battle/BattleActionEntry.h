/*
 * BattleActionEntry.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/battle/actions/ActionOption.h"

class CStack;

/// Cursor and status bar text for the hovered hex
struct BattleActionPreview
{
	std::string cursor; ///< cursor id from config/cursors.json
	std::string statusText;
};

/// One way to act on a battlefield hex, such as moving, attacking or viewing unit info. BattleActionsController selects one entry for the hovered hex
class IBattleActionEntry
{
public:
	virtual ~IBattleActionEntry() = default;

	/// Order in which entries are tried for a hex, lower first. Target is the unit on the hex, or nullptr
	virtual int getPriority(const CStack * actor, const CStack * target) const = 0;
	virtual bool isLegal(const BattleHex & hex) const = 0;
	/// Cursor and text for the hex, the blocked ones where the entry is not legal
	virtual BattleActionPreview preview(const BattleHex & hex) const = 0;
	/// Hexes highlighted as the result of clicking the hex, none where the entry is not legal
	virtual BattleHexArray getShadedHexes(const BattleHex & hex) const = 0;
	/// Performs the entry on a hex where it is legal
	virtual void realize(const BattleHex & hex) const = 0;
	/// Spell cast by the entry, or SpellID::NONE
	virtual SpellID getSpell() const = 0;
	/// Unit action panel button that selects the entry, if it has one
	virtual std::optional<UnitActionButton> getPanelButton() const = 0;
};

using BattleActionEntries = std::vector<std::shared_ptr<const IBattleActionEntry>>;
