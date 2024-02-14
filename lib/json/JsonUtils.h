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

VCMI_LIB_NAMESPACE_BEGIN

namespace JsonUtils
{
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
}

VCMI_LIB_NAMESPACE_END
