/*
 * ScriptTypeDescription.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../json/JsonNode.h"

class ICombatEventScript;
class IDamageCalculatorScript;

/// (modScope, sourcePath) pair identifying one script source layer
using ScriptSource = std::pair<std::string, std::string>;

/// Which engine interface a script implements, declared as `implements` in its json.
/// A script is of exactly one kind - the kind decides what the engine may call on it.
enum class ScriptKind
{
	SPELL_EFFECT,
	COMBAT_EVENT,
	DAMAGE_CALCULATOR,

	INVALID
};

/// Everything the engine knows about one script.
/// Shared by every script kind - a kind that has no use for a field simply ignores it.
struct ScriptTypeDescription
{
	std::string identifier; ///< "lifeDrain"
	std::string modScope; ///< "core"
	std::string scriptId; ///< modScope + ':' + identifier, unique across all scripts
	std::string sourcePath; ///< path to the base source, without the SCRIPTS/ prefix
	ScriptKind kind = ScriptKind::INVALID;
	std::vector<ScriptSource> patches; ///< extra layers stacked over the base, in declared order
	JsonNode parametersSchema; ///< json schema validating the parameters of every instance
	std::vector<std::string> stringRegistrations; ///< parameter fields holding translatable text
	std::string descriptionTextID; ///< text shown to the player, empty if the script declares none
	int priority = 0; ///< scripts reacting to the same event run from lowest to highest

	/// a combat event script is stateless and shared between every unit running it, so the single
	/// instance is created on load and handed out as is. Null for every other kind
	std::shared_ptr<ICombatEventScript> combatEventScript;

	/// the damage calculator is one for the whole game rather than one per bearer, so the single
	/// instance is likewise created on load. Null for every other kind
	std::shared_ptr<IDamageCalculatorScript> damageCalculatorScript;
};
