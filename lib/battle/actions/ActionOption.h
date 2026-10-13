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

/// One way for the actor to reach the hovered hex: the side the hex is approached from, and the hex the actor acts from
struct ActionApproach
{
	BattleHex::EDir direction = BattleHex::NONE;
	BattleHex from;
};

/// Acting unit and hovered hex of a query about an option
struct ActionContext
{
	const battle::Unit * actor = nullptr;
	BattleHex hoveredHex;
	/// Picked by the client from BattleActionType::getApproaches, if the option offers any for the hex
	std::optional<ActionApproach> approach = std::nullopt;
};

/// What the client shows for an option on the hovered hex
struct ActionPreview
{
	std::string cursor; ///< cursor id from config/cursors.json
	MetaString statusText;
	BattleHexArray shadedHexes;
	std::vector<uint32_t> affectedUnits; ///< units affected besides the one on the hovered hex
};
