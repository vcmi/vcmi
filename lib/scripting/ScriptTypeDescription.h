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

/// (modScope, sourcePath) pair identifying one script source layer
using ScriptSource = std::pair<std::string, std::string>;

/// Which engine interface a script implements, declared as `implements` in its json.
/// A script is of exactly one kind - the kind decides what the engine may call on it.
enum class ScriptKind
{
	SPELL_EFFECT,
	COMBAT_EVENT,

	INVALID
};

/// Everything the engine knows about one script without having loaded its source.
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
};
