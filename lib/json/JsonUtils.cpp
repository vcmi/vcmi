/*
 * JsonUtils.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "JsonUtils.h"

#include "JsonValidator.h"

#include "../filesystem/Filesystem.h"

VCMI_LIB_USING_NAMESPACE

static const JsonNode nullNode;

static JsonNode getDefaultValue(const JsonNode & schema, std::string fieldName)
{
	const JsonNode & fieldProps = schema["properties"][fieldName];

#if defined(VCMI_IOS)
	if (!fieldProps["defaultIOS"].isNull())
		return fieldProps["defaultIOS"];
#elif defined(VCMI_ANDROID)
	if (!fieldProps["defaultAndroid"].isNull())
		return fieldProps["defaultAndroid"];
#elif defined(VCMI_WINDOWS)
	if (!fieldProps["defaultWindows"].isNull())
		return fieldProps["defaultWindows"];
#endif

#if !defined(VCMI_MOBILE)
	if (!fieldProps["defaultDesktop"].isNull())
		return fieldProps["defaultDesktop"];
#endif
	return fieldProps["default"];
}

static void eraseOptionalNodes(JsonNode & node, const JsonNode & schema)
{
	assert(schema["type"].String() == "object");

	std::set<std::string> foundEntries;

	for(const auto & entry : schema["required"].Vector())
		foundEntries.insert(entry.String());

	vstd::erase_if(node.Struct(), [&foundEntries](const auto & structEntry){
		return !vstd::contains(foundEntries, structEntry.first);
	});
}

static void minimizeNode(JsonNode & node, const JsonNode & schema)
{
	if (schema["type"].String() != "object")
		return;

	for(const auto & entry : schema["required"].Vector())
	{
		const std::string & name = entry.String();
		minimizeNode(node[name], schema["properties"][name]);

		if (vstd::contains(node.Struct(), name) && node[name] == getDefaultValue(schema, name))
			node.Struct().erase(name);
	}
	eraseOptionalNodes(node, schema);
}

static void maximizeNode(JsonNode & node, const JsonNode & schema)
{
	// "required" entry can only be found in object/struct
	if (schema["type"].String() != "object")
		return;

	// check all required entries that have default version
	for(const auto & entry : schema["required"].Vector())
	{
		const std::string & name = entry.String();

		if (node[name].isNull() && !getDefaultValue(schema, name).isNull())
			node[name] = getDefaultValue(schema, name);

		maximizeNode(node[name], schema["properties"][name]);
	}

	eraseOptionalNodes(node, schema);
}

VCMI_LIB_NAMESPACE_BEGIN

void JsonUtils::minimize(JsonNode & node, const std::string & schemaName)
{
	minimizeNode(node, getSchema(schemaName));
}

void JsonUtils::maximize(JsonNode & node, const std::string & schemaName)
{
	maximizeNode(node, getSchema(schemaName));
}

bool JsonUtils::validate(const JsonNode & node, const std::string & schemaName, const std::string & dataName)
{
	JsonValidator validator;
	std::string log = validator.check(schemaName, node);
	if (!log.empty())
	{
		logMod->warn("Data in %s is invalid!", dataName);
		logMod->warn(log);
		logMod->trace("%s json: %s", dataName, node.toCompactString());
	}
	return log.empty();
}

const JsonNode & getSchemaByName(const std::string & name)
{
	// cached schemas to avoid loading json data multiple times
	static std::map<std::string, JsonNode> loadedSchemas;

	if (vstd::contains(loadedSchemas, name))
		return loadedSchemas[name];

	auto filename = JsonPath::builtin("config/schemas/" + name);

	if (CResourceHandler::get()->existsResource(filename))
	{
		loadedSchemas[name] = JsonNode(filename);
		return loadedSchemas[name];
	}

	logMod->error("Error: missing schema with name %s!", name);
	assert(0);
	return nullNode;
}

const JsonNode & JsonUtils::getSchema(const std::string & URI)
{
	size_t posColon = URI.find(':');
	size_t posHash  = URI.find('#');
	std::string filename;
	if(posColon == std::string::npos)
	{
		filename = URI.substr(0, posHash);
	}
	else
	{
		std::string protocolName = URI.substr(0, posColon);
		filename = URI.substr(posColon + 1, posHash - posColon - 1) + ".json";
		if(protocolName != "vcmi")
		{
			logMod->error("Error: unsupported URI protocol for schema: %s", URI);
			return nullNode;
		}
	}

	// check if json pointer if present (section after hash in string)
	if(posHash == std::string::npos || posHash == URI.size() - 1)
	{
		auto const & result = getSchemaByName(filename);
		if (result.isNull())
			logMod->error("Error: missing schema %s", URI);
		return result;
	}
	else
	{
		auto const & result = getSchemaByName(filename).resolvePointer(URI.substr(posHash + 1));
		if (result.isNull())
			logMod->error("Error: missing schema %s", URI);
		return result;
	}
}

void JsonUtils::merge(JsonNode & dest, JsonNode & source, bool ignoreOverride, bool copyMeta)
{
	if (dest.getType() == JsonNode::JsonType::DATA_NULL)
	{
		std::swap(dest, source);
		return;
	}

	switch (source.getType())
	{
		case JsonNode::JsonType::DATA_NULL:
		{
			dest.clear();
			break;
		}
		case JsonNode::JsonType::DATA_BOOL:
		case JsonNode::JsonType::DATA_FLOAT:
		case JsonNode::JsonType::DATA_INTEGER:
		case JsonNode::JsonType::DATA_STRING:
		case JsonNode::JsonType::DATA_VECTOR:
		{
			std::swap(dest, source);
			break;
		}
		case JsonNode::JsonType::DATA_STRUCT:
		{
			if(!ignoreOverride && source.getOverrideFlag())
			{
				std::swap(dest, source);
			}
			else
			{
				if (copyMeta)
					dest.setModScope(source.getModScope(), false);

				//recursively merge all entries from struct
				for(auto & node : source.Struct())
					merge(dest[node.first], node.second, ignoreOverride);
			}
		}
	}
}

void JsonUtils::mergeCopy(JsonNode & dest, JsonNode source, bool ignoreOverride, bool copyMeta)
{
	// uses copy created in stack to safely merge two nodes
	merge(dest, source, ignoreOverride, copyMeta);
}

void JsonUtils::inherit(JsonNode & descendant, const JsonNode & base)
{
	JsonNode inheritedNode(base);
	merge(inheritedNode, descendant, true, true);
	std::swap(descendant, inheritedNode);
}

JsonNode JsonUtils::assembleFromFiles(const std::vector<std::string> & files)
{
	bool isValid = false;
	return assembleFromFiles(files, isValid);
}

JsonNode JsonUtils::assembleFromFiles(const std::vector<std::string> & files, bool & isValid)
{
	isValid = true;
	JsonNode result;

	for(const auto & file : files)
	{
		bool isValidFile = false;
		JsonNode section(JsonPath::builtinTODO(file), isValidFile);
		merge(result, section);
		isValid |= isValidFile;
	}
	return result;
}

JsonNode JsonUtils::assembleFromFiles(const std::string & filename)
{
	JsonNode result;
	JsonPath resID = JsonPath::builtinTODO(filename);

	for(auto & loader : CResourceHandler::get()->getResourcesWithName(resID))
	{
		auto textData = loader->load(resID)->readAll();
		JsonNode section(reinterpret_cast<std::byte *>(textData.first.get()), textData.second);
		merge(result, section);
	}
	return result;
}

VCMI_LIB_NAMESPACE_END
