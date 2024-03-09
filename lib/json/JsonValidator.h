/*
 * JsonValidator.h, part of VCMI engine
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

/// Class for Json validation. Mostly compilant with json-schema v6 draf
struct JsonValidator
{
	/// path from root node to current one.
	/// JsonNode is used as variant - either string (name of node) or as float (index in list)
	std::vector<JsonNode> currentPath;

	/// Stack of used schemas. Last schema is the one used currently.
	/// May contain multiple items in case if remote references were found
	std::vector<std::string> usedSchemas;

	/// generates error message
	std::string makeErrorMessage(const std::string &message);

	using TFormatValidator = std::function<std::string(const JsonNode &)>;
	using TFormatMap = std::unordered_map<std::string, TFormatValidator>;
	using TFieldValidator = std::function<std::string(JsonValidator &, const JsonNode &, const JsonNode &, const JsonNode &)>;
	using TValidatorMap = std::unordered_map<std::string, TFieldValidator>;

	/// map of known fields in schema
	const TValidatorMap & getKnownFieldsFor(JsonNode::JsonType type);
	const TFormatMap & getKnownFormats();

	std::string check(const std::string & schemaName, const JsonNode & data);
	std::string check(const JsonNode & schema, const JsonNode & data);
};

VCMI_LIB_NAMESPACE_END
