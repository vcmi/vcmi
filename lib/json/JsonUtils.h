/*
 * JsonUtils.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "JsonNode.h"
#include "../GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace JsonUtils
{
	DLL_LINKAGE std::shared_ptr<Bonus> parseBonus(const JsonVector & ability_vec);
	DLL_LINKAGE std::shared_ptr<Bonus> parseBonus(const JsonNode & ability);
	DLL_LINKAGE std::shared_ptr<Bonus> parseBuildingBonus(const JsonNode & ability, const FactionID & faction, const BuildingID & building, const std::string & description);
	DLL_LINKAGE bool parseBonus(const JsonNode & ability, Bonus * placement);
	DLL_LINKAGE std::shared_ptr<ILimiter> parseLimiter(const JsonNode & limiter);
	DLL_LINKAGE CSelector parseSelector(const JsonNode &ability);
	DLL_LINKAGE void resolveAddInfo(CAddInfo & var, const JsonNode & node);

	/**
	 * @brief recursively merges source into dest, replacing identical fields
	 * struct : recursively calls this function
	 * arrays : each entry will be merged recursively
	 * values : value in source will replace value in dest
	 * null   : if value in source is present but set to null it will delete entry in dest
	 * @note this function will destroy data in source
	 */
	DLL_LINKAGE void merge(JsonNode & dest, JsonNode & source, bool ignoreOverride = false, bool copyMeta = false);

	/**
	 * @brief recursively merges source into dest, replacing identical fields
	 * struct : recursively calls this function
	 * arrays : each entry will be merged recursively
	 * values : value in source will replace value in dest
	 * null   : if value in source is present but set to null it will delete entry in dest
	 * @note this function will preserve data stored in source by creating copy
	 */
	DLL_LINKAGE void mergeCopy(JsonNode & dest, JsonNode source, bool ignoreOverride = false, bool copyMeta = false);

	/** @brief recursively merges descendant into copy of base node
	* Result emulates inheritance semantic
	*/
	DLL_LINKAGE void inherit(JsonNode & descendant, const JsonNode & base);

	/**
	 * @brief construct node representing the common structure of input nodes
	 * @param pruneEmpty - omit common properties whose intersection is empty
	 * different types: null
	 * struct: recursive intersect on common properties
	 * other: input if equal, null otherwise
	 */
	DLL_LINKAGE JsonNode intersect(const JsonNode & a, const JsonNode & b, bool pruneEmpty = true);
	DLL_LINKAGE JsonNode intersect(const std::vector<JsonNode> & nodes, bool pruneEmpty = true);

	/**
	 * @brief construct node representing the difference "node - base"
	 * merging difference with base gives node
	 */
	DLL_LINKAGE JsonNode difference(const JsonNode & node, const JsonNode & base);

	/**
	 * @brief generate one Json structure from multiple files
	 * @param files - list of filenames with parts of json structure
	 */
	DLL_LINKAGE JsonNode assembleFromFiles(const std::vector<std::string> & files);
	DLL_LINKAGE JsonNode assembleFromFiles(const std::vector<std::string> & files, bool & isValid);

	/// This version loads all files with same name (overridden by mods)
	DLL_LINKAGE JsonNode assembleFromFiles(const std::string & filename);

	/**
	 * @brief removes all nodes that are identical to default entry in schema
	 * @param node - JsonNode to minimize
	 * @param schemaName - name of schema to use
	 * @note for minimizing data must be valid against given schema
	 */
	DLL_LINKAGE void minimize(JsonNode & node, const std::string & schemaName);
	/// opposed to minimize, adds all missing, required entries that have default value
	DLL_LINKAGE void maximize(JsonNode & node, const std::string & schemaName);

	/**
	* @brief validate node against specified schema
	* @param node - JsonNode to check
	* @param schemaName - name of schema to use
	* @param dataName - some way to identify data (printed in console in case of errors)
	* @returns true if data in node fully compilant with schema
	*/
	DLL_LINKAGE bool validate(const JsonNode & node, const std::string & schemaName, const std::string & dataName);

	/// get schema by json URI: vcmi:<name of file in schemas directory>#<entry in file, optional>
	/// example: schema "vcmi:settings" is used to check user settings
	DLL_LINKAGE const JsonNode & getSchema(const std::string & URI);

	/// for easy construction of JsonNodes; helps with inserting primitives into vector node
	DLL_LINKAGE JsonNode boolNode(bool value);
	DLL_LINKAGE JsonNode floatNode(double value);
	DLL_LINKAGE JsonNode stringNode(const std::string & value);
	DLL_LINKAGE JsonNode intNode(si64 value);
}

VCMI_LIB_NAMESPACE_END
