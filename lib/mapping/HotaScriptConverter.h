/*
 * HotaScriptConverter.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <functional>
#include <string>

#include "../scripting/ScriptVariablesStorage.h"

class MapReaderH3M;
class TextIdentifier;

/// Result of converting a map's event system: the generated Lua source plus the variable
/// declarations the engine uses to seed and persist the map's script variables.
struct HotaScriptConversionResult
{
	std::string luaSource;
	std::vector<ScriptVariableDeclaration> variables;
};

/// Converts the HotA (Horn of the Abyss) event-scripting bytecode embedded in a
/// H3M map into equivalent Lua source. Pure recursive-descent emission: each
/// helper consumes one node from the reader and returns its Lua text.
class HotaScriptConverter
{
public:
	/// Reads a map text field and returns the stable identifier it was registered under.
	using LocalizeCallback = std::function<std::string(const TextIdentifier &)>;

	HotaScriptConverter(MapReaderH3M & reader, std::string mapName, LocalizeCallback localizeString);

	/// Consumes the whole HotA event-system block. The result is empty when the map has
	/// no active event system.
	HotaScriptConversionResult convert();

private:
	MapReaderH3M & reader;
	std::string mapName;
	LocalizeCallback localizeString;

	// identify the event currently being emitted, used to generate text identifiers
	std::string currentBucket;
	int currentEventID = 0;
	int stringCounter = 0;

	// declarations collected while parsing, used to seed and persist the engine-side variables
	std::vector<ScriptVariableDeclaration> variables;

	std::string loadEventList(const std::string & bucket);
	std::string loadVariables(); // reads declarations, returns the `Vars` id->name Lua table
	void loadEventMap();

	std::string loadActions(int indent); ///< multi-line statement block, each line indented by `indent` tabs
	std::string loadCondition(); ///< single Lua boolean expression
	std::string loadConditionInternal();
	std::string loadExpression(); ///< single Lua numeric expression
	std::string loadExpressionInternal();

	std::string loadImageList(int count);
	std::string localizedText(const std::string & role); ///< reads a text field, returns its identifier as a quoted Lua literal
};
