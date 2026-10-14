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

#include "../scripting/ScriptVariablesStorage.h"
#include "../constants/EntityIdentifiers.h"

class MapReaderH3M;
class TextIdentifier;
class CMap;

/// Converts the HotA (Horn of the Abyss) event-scripting bytecode embedded in a
/// H3M map into equivalent Lua source. Pure recursive-descent emission: each
/// helper consumes one node from the reader and returns its Lua text.
class HotaScriptConverter
{
public:
	/// Reads a map text field and returns the stable identifier it was registered under.
	using LocalizeCallback = std::function<std::string(const TextIdentifier &)>;

	HotaScriptConverter(MapReaderH3M & reader, std::string mapName, LocalizeCallback localizeString);

	void readScript();


	/// Consumes the whole HotA event-system block. The result is empty when the map has
	/// no active event system.
	void convert(CMap * map, const std::map<si32, ObjectInstanceID> & questIdentifierToId);

	/// Stable key of the Lua handler for an event bucket + id. Shared by the converter (which emits
	/// the handler table key) and the H3M loader (which tags objects/events with the handler to fire).
	std::string eventHandlerName(const std::string & bucket, int eventID);

private:
	MapReaderH3M & reader;
	std::string mapName;
	LocalizeCallback localizeString;

	// identify the event currently being emitted, used to generate text identifiers
	std::string currentBucket;
	int currentEventID = 0;
	int stringCounter = 0;

	// declarations collected while parsing, used to seed and persist the engine-side variables
	std::vector<ScriptVariableDefinition> variables;

	// H3M object identifiers referenced by object-identity predicates, resolved to names by the loader
	std::set<uint32_t> referencedObjects;

	std::string events;
	std::string variablesTable;

	std::string loadEventList(const std::string & bucket);
	std::string loadVariables(); // reads declarations, returns the `Vars` id->name Lua table
	void loadEventMap();

	std::string loadActions(int indent); ///< multi-line statement block, each line indented by `indent` tabs
	std::string loadCondition(); ///< single Lua boolean expression
	std::string loadConditionInternal();
	std::string loadExpression(); ///< single Lua numeric expression
	std::string loadExpressionInternal();

	/// Error for a field whose meaning is not known yet, naming the map and the event being converted
	std::runtime_error unsupported(const std::string & message) const;

	std::string loadImageList(int count);
	std::string localizedText(const std::string & role); ///< reads a text field, returns a Lua MetaString table holding its identifier

	std::string loadQuestReferences(CMap * map, const std::map<si32, ObjectInstanceID> & questIdentifierToId);

	/// Records an H3M object identifier and returns the Lua expression that looks up its runtime
	/// object name in the loader-populated `questObjects` table.
	std::string questObjectRef(uint32_t identifier);
};
