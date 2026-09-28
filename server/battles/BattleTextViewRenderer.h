/*
 * BattleTextViewRenderer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/constants/EntityIdentifiers.h"

#include <string>
#include <vector>

class BattleInfo;

namespace battleTextView
{
	struct RenderOptions
	{
		bool ansi = true; // false - plain, ESC-free output
	};

	/// Pure: battle + log tail -> full text frame.
	std::string renderBattleTextView(const BattleInfo & battle, const std::vector<std::string> & logTail, RenderOptions options);

	/// Frame for an ended battle (battle is already erased from state - pack fields only).
	std::string renderBattleSummary(const BattleID & id, const PlayerColor & victor, RenderOptions options);

	/// Frame for a cancelled battle (no victor exists).
	std::string renderBattleCancelled(const BattleID & id, RenderOptions options);
}
