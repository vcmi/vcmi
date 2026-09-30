/*
 * ActionOption.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../BattleHexArray.h"
#include "../../constants/EntityIdentifiers.h"
#include "../../filesystem/ResourcePath.h"
#include "../../texts/MetaString.h"

class BattleActionType;

namespace battle
{
class Unit;
}

/// Button of the unit action panel, shared by all options with an equal button
struct UnitActionButton
{
	int order = 0; ///< position in the panel, lower first
	SpellID spell; ///< spell of a spell button, which shows the spell icon and description
	ImagePath icon; ///< icon of a button without a spell
	std::string tooltipTextID; ///< tooltip of a button without a spell

	bool operator==(const UnitActionButton & other) const = default;
};

/// One way for a unit to use an action type, offered to the player on the unit's turn
struct ActionOption
{
	const BattleActionType * type = nullptr;
	/// Unit action panel button that selects the option, if it has one
	std::optional<UnitActionButton> button;
};

/// Acting unit and hovered hex of a query about an option
struct ActionContext
{
	const battle::Unit * actor = nullptr;
	BattleHex hoveredHex;
};

/// What the client shows for an option on the hovered hex
struct ActionPreview
{
	std::string cursor; ///< cursor id from config/cursors.json
	MetaString statusText;
	BattleHexArray shadedHexes;
};
