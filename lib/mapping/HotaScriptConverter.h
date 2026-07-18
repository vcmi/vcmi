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

class MapReaderH3M;
class TextIdentifier;

/// Converts the HotA (Horn of the Abyss) event-scripting bytecode embedded in a
/// H3M map into equivalent Lua source. Pure recursive-descent emission: each
/// helper consumes one node from the reader and returns its Lua text.
class HotaScriptConverter
{
public:
	/// Reads a map text field and returns the stable identifier it was registered under.
	using LocalizeCallback = std::function<std::string(const TextIdentifier &)>;

	HotaScriptConverter(MapReaderH3M & reader, std::string mapName, LocalizeCallback localizeString);

	/// Consumes the whole HotA event-system block and returns the generated Lua
	/// source, or an empty string when the map has no active event system.
	std::string convert();

private:
	MapReaderH3M & reader;
	std::string mapName;
	LocalizeCallback localizeString;

	// identify the event currently being emitted, used to generate text identifiers
	std::string currentBucket;
	int currentEventID = 0;
	int stringCounter = 0;

	std::string loadEventList(const std::string & bucket);
	std::string loadVariables();
	void loadEventMap();

	std::string loadActions(int indent); ///< multi-line statement block, each line indented by `indent` tabs
	std::string loadCondition(); ///< single Lua boolean expression
	std::string loadConditionInternal();
	std::string loadExpression(); ///< single Lua numeric expression
	std::string loadExpressionInternal();

	std::string loadImageList(int count);
	std::string localizedText(const std::string & role); ///< reads a text field, returns its identifier as a quoted Lua literal
};
