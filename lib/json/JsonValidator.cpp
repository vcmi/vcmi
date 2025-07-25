/*
 * JsonValidator.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "JsonValidator.h"

#include "JsonUtils.h"

#include "../GameLibrary.h"
#include "../filesystem/Filesystem.h"
#include "../modding/ModScope.h"
#include "../modding/CModHandler.h"
#include "../texts/TextOperations.h"
#include "../ScopeGuard.h"
#include "modding/CModVersion.h"

VCMI_LIB_NAMESPACE_BEGIN

/// Searches for keys similar to 'target' in 'candidates' map
/// Returns closest match or empty string if no suitable candidates are found
static std::string findClosestMatch(const JsonMap & candidates, const std::string & target)
{
	// Maximum distance at which we can consider strings to be similar
	// If strings have more different symbols than this number then it is not a typo, but a completely different word
	static constexpr int maxDistance = 5;
	int bestDistance = maxDistance;
	std::string bestMatch;

	for (auto const & candidate : candidates)
	{
		int newDistance = TextOperations::getLevenshteinDistance(candidate.first, target);

		if (newDistance < bestDistance)
		{
			bestDistance = newDistance;
			bestMatch = candidate.first;
		}
	}
	return bestMatch;
}

static std::string emptyCheck(JsonValidator & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
{
	// check is not needed - e.g. incorporated into another check
	return "";
}

static std::string notImplementedCheck(JsonValidator & validator,
								const JsonNode & baseSchema,
								const JsonNode & schema,
								const JsonNode & data)
{
	return "Not implemented entry in schema";
}

static std::string schemaListCheck(JsonValidator & validator,
							const JsonNode & baseSchema,
							const JsonNode & schema,
							const JsonNode & data,
							const std::string & errorMsg,
							const std::function<bool(size_t)> & isValid)
{
	std::string errors = "<tested schemas>\n";
	size_t result = 0;

	for(const auto & schemaEntry : schema.Vector())
	{
		std::string error = validator.check(schemaEntry, data);
		if (error.empty())
		{
			result++;
		}
		else
		{
			errors += error;
			errors += "<end of schema>\n";
		}
	}
	if (isValid(result))
		return "";
	else
		return validator.makeErrorMessage(errorMsg) + errors;
}

static std::string allOfCheck(JsonValidator & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
{
	return schemaListCheck(validator, baseSchema, schema, data, "Failed to pass all schemas", [&schema](size_t count)
	{
		return count == schema.Vector().size();
	});
}

static std::string anyOfCheck(JsonValidator & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
{
	return schemaListCheck(validator, baseSchema, schema, data, "Failed to pass any schema", [](size_t count)
	{
		return count > 0;
	});
}

static std::string oneOfCheck(JsonValidator & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
{
	return schemaListCheck(validator, baseSchema, schema, data, "Failed to pass exactly one schema", [](size_t count)
	{
		return count == 1;
	});
}

static std::string notCheck(JsonValidator & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
{
	if (validator.check(schema, data).empty())
		return validator.makeErrorMessage("Successful validation against negative check");
	return "";
}

static std::string ifCheck(JsonValidator & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
{
	if (validator.check(schema, data).empty())
		return validator.check(baseSchema["then"], data);
	return "";
}

static std::string enumCheck(JsonValidator & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
{
	for(const auto & enumEntry : schema.Vector())
	{
		if (data == enumEntry)
			return "";
	}

	std::string errorMessage = "Key must have one of predefined values:" + schema.toCompactString();

	return validator.makeErrorMessage(errorMessage);
}

static std::string constCheck(JsonValidator & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
{
	if (data == schema)
		return "";

	return validator.makeErrorMessage("Key must have have constant value");
}

static std::string typeCheck(JsonValidator & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
{
	static const std::unordered_map<std::string, JsonNode::JsonType> stringToType =
	{
		{"null",   JsonNode::JsonType::DATA_NULL},
		{"boolean", JsonNode::JsonType::DATA_BOOL},
		{"number", JsonNode::JsonType::DATA_FLOAT},
		{"integer", JsonNode::JsonType::DATA_INTEGER},
		{"string",  JsonNode::JsonType::DATA_STRING},
		{"array",  JsonNode::JsonType::DATA_VECTOR},
		{"object",  JsonNode::JsonType::DATA_STRUCT}
	};

	const auto & typeName = schema.String();
	auto it = stringToType.find(typeName);
	if(it == stringToType.end())
	{
		return validator.makeErrorMessage("Unknown type in schema:" + typeName);
	}

	JsonNode::JsonType type = it->second;

	// for "number" type both float and integer are allowed
	if(type == JsonNode::JsonType::DATA_FLOAT && data.isNumber())
		return "";

	if(type != data.getType() && data.getType() != JsonNode::JsonType::DATA_NULL)
		return validator.makeErrorMessage("Type mismatch! Expected " + schema.String());
	return "";
}

static std::string refCheck(JsonValidator & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
{
	std::string URI = schema.String();
	//node must be validated using schema pointed by this reference and not by data here
	//Local reference. Turn it into more easy to handle remote ref
	if (boost::algorithm::starts_with(URI, "#"))
	{
		const std::string name = validator.usedSchemas.back();
		const std::string nameClean = name.substr(0, name.find('#'));
		URI = nameClean + URI;
	}
	return validator.check(URI, data);
}

static std::string formatCheck(JsonValidator & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
{
	auto formats = validator.getKnownFormats();
	std::string errors;
	auto checker = formats.find(schema.String());
	if (checker != formats.end())
	{
		if (data.isString())
		{
			std::string result = checker->second(data);
			if (!result.empty())
				errors += validator.makeErrorMessage(result);
		}
		else
		{
			errors += validator.makeErrorMessage("Format value must be string: " + schema.String());
		}
	}
	else
		errors += validator.makeErrorMessage("Unsupported format type: " + schema.String());
	return errors;
}

static std::string maxLengthCheck(JsonValidator & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
{
	if (data.String().size() > schema.Float())
		return validator.makeErrorMessage((boost::format("String is longer than %d symbols") % schema.Float()).str());
	return "";
}

static std::string minLengthCheck(JsonValidator & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
{
	if (data.String().size() < schema.Float())
		return validator.makeErrorMessage((boost::format("String is shorter than %d symbols") % schema.Float()).str());
	return "";
}

static std::string maximumCheck(JsonValidator & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
{
	if (data.Float() > schema.Float())
		return validator.makeErrorMessage((boost::format("Value is bigger than %d") % schema.Float()).str());
	return "";
}

static std::string minimumCheck(JsonValidator & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
{
	if (data.Float() < schema.Float())
		return validator.makeErrorMessage((boost::format("Value is smaller than %d") % schema.Float()).str());
	return "";
}

static std::string exclusiveMaximumCheck(JsonValidator & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
{
	if (data.Float() >= schema.Float())
		return validator.makeErrorMessage((boost::format("Value is bigger than %d") % schema.Float()).str());
	return "";
}

static std::string exclusiveMinimumCheck(JsonValidator & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
{
	if (data.Float() <= schema.Float())
		return validator.makeErrorMessage((boost::format("Value is smaller than %d") % schema.Float()).str());
	return "";
}

static std::string multipleOfCheck(JsonValidator & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
{
	double result = data.Integer() / schema.Integer();
	if (!vstd::isAlmostEqual(floor(result), result))
		return validator.makeErrorMessage((boost::format("Value is not divisible by %d") % schema.Float()).str());
	return "";
}

static std::string itemEntryCheck(JsonValidator & validator, const JsonVector & items, const JsonNode & schema, size_t index)
{
	validator.currentPath.emplace_back();
	validator.currentPath.back().Float() = static_cast<double>(index);
	auto onExit = vstd::makeScopeGuard([&validator]()
	{
		validator.currentPath.pop_back();
	});

	if (!schema.isNull())
		return validator.check(schema, items[index]);
	return "";
}

static std::string itemsCheck(JsonValidator & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
{
	std::string errors;
	for (size_t i=0; i<data.Vector().size(); i++)
	{
		if (schema.getType() == JsonNode::JsonType::DATA_VECTOR)
		{
			if (schema.Vector().size() > i)
				errors += itemEntryCheck(validator, data.Vector(), schema.Vector()[i], i);
		}
		else
		{
			errors += itemEntryCheck(validator, data.Vector(), schema, i);
		}
	}
	return errors;
}

static std::string additionalItemsCheck(JsonValidator & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
{
	std::string errors;
	// "items" is struct or empty (defaults to empty struct) - validation always successful
	const JsonNode & items = baseSchema["items"];
	if (items.getType() != JsonNode::JsonType::DATA_VECTOR)
		return "";

	for (size_t i=items.Vector().size(); i<data.Vector().size(); i++)
	{
		if (schema.getType() == JsonNode::JsonType::DATA_STRUCT)
			errors += itemEntryCheck(validator, data.Vector(), schema, i);
		else if(!schema.isNull() && !schema.Bool())
			errors += validator.makeErrorMessage("Unknown entry found");
	}
	return errors;
}

static std::string minItemsCheck(JsonValidator & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
{
	if (data.Vector().size() < schema.Float())
		return validator.makeErrorMessage((boost::format("Length is smaller than %d") % schema.Float()).str());
	return "";
}

static std::string maxItemsCheck(JsonValidator & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
{
	if (data.Vector().size() > schema.Float())
		return validator.makeErrorMessage((boost::format("Length is bigger than %d") % schema.Float()).str());
	return "";
}

static std::string uniqueItemsCheck(JsonValidator & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
{
	if (schema.Bool())
	{
		for (auto itA = schema.Vector().begin(); itA != schema.Vector().end(); itA++)
		{
			auto itB = itA;
			while (++itB != schema.Vector().end())
			{
				if (*itA == *itB)
					return validator.makeErrorMessage("List must consist from unique items");
			}
		}
	}
	return "";
}

static std::string maxPropertiesCheck(JsonValidator & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
{
	if (data.Struct().size() > schema.Float())
		return validator.makeErrorMessage((boost::format("Number of entries is bigger than %d") % schema.Float()).str());
	return "";
}

static std::string minPropertiesCheck(JsonValidator & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
{
	if (data.Struct().size() < schema.Float())
		return validator.makeErrorMessage((boost::format("Number of entries is less than %d") % schema.Float()).str());
	return "";
}

static std::string uniquePropertiesCheck(JsonValidator & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
{
	for (auto itA = data.Struct().begin(); itA != data.Struct().end(); itA++)
	{
		auto itB = itA;
		while (++itB != data.Struct().end())
		{
			if (itA->second == itB->second)
				return validator.makeErrorMessage("List must consist from unique items");
		}
	}
	return "";
}

static std::string requiredCheck(JsonValidator & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
{
	std::string errors;
	for(const auto & required : schema.Vector())
	{
		if (data[required.String()].isNull() && data.getModScope() != "core")
			errors += validator.makeErrorMessage("Required entry " + required.String() + " is missing");
	}
	return errors;
}

static std::string dependenciesCheck(JsonValidator & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
{
	std::string errors;
	for(const auto & deps : schema.Struct())
	{
		if (!data[deps.first].isNull())
		{
			if (deps.second.getType() == JsonNode::JsonType::DATA_VECTOR)
			{
				JsonVector depList = deps.second.Vector();
				for(auto & depEntry : depList)
				{
					if (data[depEntry.String()].isNull())
						errors += validator.makeErrorMessage("Property " + depEntry.String() + " required for " + deps.first + " is missing");
				}
			}
			else
			{
				if (!validator.check(deps.second, data).empty())
					errors += validator.makeErrorMessage("Requirements for " + deps.first + " are not fulfilled");
			}
		}
	}
	return errors;
}

static std::string propertyEntryCheck(JsonValidator & validator, const JsonNode &node, const JsonNode & schema, const std::string & nodeName)
{
	validator.currentPath.emplace_back();
	validator.currentPath.back().String() = nodeName;
	auto onExit = vstd::makeScopeGuard([&validator]()
	{
		validator.currentPath.pop_back();
	});

	// there is schema specifically for this item
	if (!schema.isNull())
		return validator.check(schema, node);
	return "";
}

static std::string propertiesCheck(JsonValidator & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
{
	std::string errors;

	for(const auto & entry : data.Struct())
		errors += propertyEntryCheck(validator, entry.second, schema[entry.first], entry.first);
	return errors;
}

static std::string additionalPropertiesCheck(JsonValidator & validator, const JsonNode & baseSchema, const JsonNode & schema, const JsonNode & data)
{
	std::string errors;
	for(const auto & entry : data.Struct())
	{
		if (baseSchema["properties"].Struct().count(entry.first) == 0)
		{
			// try generic additionalItems schema
			if (schema.getType() == JsonNode::JsonType::DATA_STRUCT)
				errors += propertyEntryCheck(validator, entry.second, schema, entry.first);

			// or, additionalItems field can be bool which indicates if such items are allowed
			else if(!schema.isNull() && !schema.Bool()) // present and set to false - error
			{
				std::string bestCandidate = findClosestMatch(baseSchema["properties"].Struct(), entry.first);
				if (!bestCandidate.empty())
					errors += validator.makeErrorMessage("Unknown entry found: '" + entry.first + "'. Perhaps you meant '" + bestCandidate + "'?");
				else
					errors += validator.makeErrorMessage("Unknown entry found: " + entry.first);
			}
		}
	}
	return errors;
}

static bool testFilePresence(const std::string & scope, const ResourcePath & resource)
{
#ifndef ENABLE_MINIMAL_LIB
	std::set<std::string> allowedScopes;
	if(scope != ModScope::scopeBuiltin() && !scope.empty()) // all real mods may have dependencies
	{
		//NOTE: recursive dependencies are not allowed at the moment - update code if this changes
		bool found = true;
		allowedScopes = LIBRARY->modh->getModDependencies(scope, found);

		if(!found)
			return false;

		allowedScopes.insert(ModScope::scopeBuiltin()); // all mods can use H3 files
	}
	allowedScopes.insert(scope); // mods can use their own files

	for(const auto & entry : allowedScopes)
	{
		if (CResourceHandler::get(entry)->existsResource(resource))
			return true;
	}
#endif
	return false;
}

#define TEST_FILE(scope, prefix, file, type) \
	if (testFilePresence(scope, ResourcePath(prefix + file, type))) \
	return ""

static std::string testAnimation(const std::string & path, const std::string & scope)
{
	TEST_FILE(scope, "Sprites/", path, EResType::ANIMATION);
	TEST_FILE(scope, "Sprites/", path, EResType::JSON);
	return "Animation file \"" + path + "\" was not found";
}

static std::string textFile(const JsonNode & node)
{
	TEST_FILE(node.getModScope(), "", node.String(), EResType::JSON);
	return "Text file \"" + node.String() + "\" was not found";
}

static std::string musicFile(const JsonNode & node)
{
	TEST_FILE(node.getModScope(), "Music/", node.String(), EResType::SOUND);
	TEST_FILE(node.getModScope(), "", node.String(), EResType::SOUND);
	return "Music file \"" + node.String() + "\" was not found";
}

static std::string soundFile(const JsonNode & node)
{
	TEST_FILE(node.getModScope(), "Sounds/", node.String(), EResType::SOUND);
	return "Sound file \"" + node.String() + "\" was not found";
}

static std::string animationFile(const JsonNode & node)
{
	return testAnimation(node.String(), node.getModScope());
}

static std::string imageFile(const JsonNode & node)
{
	TEST_FILE(node.getModScope(), "Data/", node.String(), EResType::IMAGE);
	TEST_FILE(node.getModScope(), "Sprites/", node.String(), EResType::IMAGE);
	if (node.String().find(':') != std::string::npos)
		return testAnimation(node.String().substr(0, node.String().find(':')), node.getModScope());
	return "Image file \"" + node.String() + "\" was not found";
}

static std::string videoFile(const JsonNode & node)
{
	TEST_FILE(node.getModScope(), "Video/", node.String(), EResType::VIDEO);
	TEST_FILE(node.getModScope(), "Video/", node.String(), EResType::VIDEO_LOW_QUALITY);
	return "Video file \"" + node.String() + "\" was not found";
}
#undef TEST_FILE

static std::string version(const JsonNode & node)
{
	auto version = CModVersion::fromString(node.String());
	if (version == CModVersion())
		return "Failed to parse mod version: " + node.toCompactString() + ". Expected format X.Y.Z, where X, Y, Z are non-negative numbers";
	return "";
}

JsonValidator::TValidatorMap createCommonFields()
{
	JsonValidator::TValidatorMap ret;

	ret["format"] =  formatCheck;
	ret["allOf"] = allOfCheck;
	ret["anyOf"] = anyOfCheck;
	ret["oneOf"] = oneOfCheck;
	ret["enum"]  = enumCheck;
	ret["const"]  = constCheck;
	ret["type"]  = typeCheck;
	ret["not"]   = notCheck;
	ret["$ref"]  = refCheck;
	ret["if"]  = ifCheck;
	ret["then"]  = emptyCheck; // implemented as part of "if check"

	// fields that don't need implementation
	ret["title"] = emptyCheck;
	ret["$schema"] = emptyCheck;
	ret["default"] = emptyCheck;
	ret["defaultIOS"] = emptyCheck;
	ret["defaultAndroid"] = emptyCheck;
	ret["defaultWindows"] = emptyCheck;
	ret["description"] = emptyCheck;
	ret["definitions"] = emptyCheck;

	// Not implemented
	ret["propertyNames"] = notImplementedCheck;
	ret["contains"] = notImplementedCheck;
	ret["examples"] = notImplementedCheck;

	return ret;
}

JsonValidator::TValidatorMap createStringFields()
{
	JsonValidator::TValidatorMap ret = createCommonFields();
	ret["maxLength"] = maxLengthCheck;
	ret["minLength"] = minLengthCheck;

	ret["pattern"] = notImplementedCheck;
	return ret;
}

JsonValidator::TValidatorMap createNumberFields()
{
	JsonValidator::TValidatorMap ret = createCommonFields();
	ret["maximum"]    = maximumCheck;
	ret["minimum"]    = minimumCheck;
	ret["multipleOf"] = multipleOfCheck;

	ret["exclusiveMaximum"] = exclusiveMaximumCheck;
	ret["exclusiveMinimum"] = exclusiveMinimumCheck;
	return ret;
}

JsonValidator::TValidatorMap createVectorFields()
{
	JsonValidator::TValidatorMap ret = createCommonFields();
	ret["items"]           = itemsCheck;
	ret["minItems"]        = minItemsCheck;
	ret["maxItems"]        = maxItemsCheck;
	ret["uniqueItems"]     = uniqueItemsCheck;
	ret["additionalItems"] = additionalItemsCheck;
	return ret;
}

JsonValidator::TValidatorMap createStructFields()
{
	JsonValidator::TValidatorMap ret = createCommonFields();
	ret["additionalProperties"]  = additionalPropertiesCheck;
	ret["uniqueProperties"]      = uniquePropertiesCheck;
	ret["maxProperties"]         = maxPropertiesCheck;
	ret["minProperties"]         = minPropertiesCheck;
	ret["dependencies"]          = dependenciesCheck;
	ret["properties"]            = propertiesCheck;
	ret["required"]              = requiredCheck;

	ret["patternProperties"] = notImplementedCheck;
	return ret;
}

JsonValidator::TFormatMap createFormatMap()
{
	JsonValidator::TFormatMap ret;
	ret["textFile"]      = textFile;
	ret["musicFile"]     = musicFile;
	ret["soundFile"]     = soundFile;
	ret["animationFile"] = animationFile;
	ret["imageFile"]     = imageFile;
	ret["videoFile"]     = videoFile;
	ret["version"]     = version;

	//TODO:
	// uri-reference
	// uri-template
	// json-pointer

	return ret;
}

std::string JsonValidator::makeErrorMessage(const std::string &message)
{
	std::string errors;
	errors += "At ";
	if (!currentPath.empty())
	{
		for(const JsonNode &path : currentPath)
		{
			errors += "/";
			if (path.getType() == JsonNode::JsonType::DATA_STRING)
				errors += path.String();
			else
				errors += std::to_string(static_cast<unsigned>(path.Float()));
		}
	}
	else
		errors += "<root>";
	errors += "\n\t Error: " + message + "\n";
	return errors;
}

std::string JsonValidator::check(const std::string & schemaName, const JsonNode & data)
{
	usedSchemas.push_back(schemaName);
	auto onscopeExit = vstd::makeScopeGuard([this]()
	{
		usedSchemas.pop_back();
	});
	return check(JsonUtils::getSchema(schemaName), data);
}

std::string JsonValidator::check(const JsonNode & schema, const JsonNode & data)
{
	const TValidatorMap & knownFields = getKnownFieldsFor(data.getType());
	std::string errors;
	for(const auto & entry : schema.Struct())
	{
		auto checker = knownFields.find(entry.first);
		if (checker != knownFields.end())
			errors += checker->second(*this, schema, entry.second, data);
	}
	return errors;
}

const JsonValidator::TValidatorMap & JsonValidator::getKnownFieldsFor(JsonNode::JsonType type)
{
	static const TValidatorMap commonFields = createCommonFields();
	static const TValidatorMap numberFields = createNumberFields();
	static const TValidatorMap stringFields = createStringFields();
	static const TValidatorMap vectorFields = createVectorFields();
	static const TValidatorMap structFields = createStructFields();

	switch (type)
	{
		case JsonNode::JsonType::DATA_FLOAT:
		case JsonNode::JsonType::DATA_INTEGER:
			return numberFields;
		case JsonNode::JsonType::DATA_STRING: return stringFields;
		case JsonNode::JsonType::DATA_VECTOR: return vectorFields;
		case JsonNode::JsonType::DATA_STRUCT: return structFields;
		default: return commonFields;
	}
}

const JsonValidator::TFormatMap & JsonValidator::getKnownFormats()
{
	static const TFormatMap knownFormats = createFormatMap();
	return knownFormats;
}

VCMI_LIB_NAMESPACE_END
