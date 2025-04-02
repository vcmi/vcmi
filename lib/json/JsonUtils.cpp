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

				if (dest.isStruct())
				{
					//recursively merge all entries from struct
					for(auto & node : source.Struct())
						merge(dest[node.first], node.second, ignoreOverride);
					break;
				}
				if (dest.isVector())
				{
					auto getIndexSafe = [&dest](const std::string & keyName) -> std::optional<int>
					{
						try {
							int index = std::stoi(keyName);
							if (index <= 0 || index > dest.Vector().size())
								throw std::out_of_range("dummy");
							return index - 1; // 1-based index -> 0-based index
						}
						catch(const std::invalid_argument &)
						{
							logMod->warn("Failed to interpret key '%s' when replacing individual items in array. Expected 'appendItem', 'appendItems', 'modify@NUM' or 'insert@NUM", keyName);
							return std::nullopt;
						}
						catch(const std::out_of_range & )
						{
							logMod->warn("Failed to replace index when replacing individual items in array. Value '%s' does not exists in targeted array of %d items", keyName, dest.Vector().size());
							return std::nullopt;
						}
					};

					for(auto & node : source.Struct())
					{
						if (node.first == "append")
						{
							dest.Vector().push_back(std::move(node.second));
						}
						else if (node.first == "appendItems")
						{
							assert(node.second.isVector());
							std::move(dest.Vector().begin(), dest.Vector().end(), std::back_inserter(dest.Vector()));
						}
						else if (boost::algorithm::starts_with(node.first, "insert@"))
						{
							constexpr int numberPosition = std::char_traits<char>::length("insert@");
							auto index = getIndexSafe(node.first.substr(numberPosition));
							if (index)
								dest.Vector().insert(dest.Vector().begin() + index.value(), std::move(node.second));
						}
						else if (boost::algorithm::starts_with(node.first, "modify@"))
						{
							constexpr int numberPosition = std::char_traits<char>::length("modify@");
							auto index = getIndexSafe(node.first.substr(numberPosition));
							if (index)
								merge(dest.Vector().at(index.value()), node.second, ignoreOverride);
						}
					}
					break;
				}
				assert(false);
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

JsonNode JsonUtils::assembleFromFiles(const JsonNode & files, bool & isValid)
{
	if (files.isVector())
	{
		assert(!files.getModScope().empty());
		auto configList = files.convertTo<std::vector<std::string> >();
		JsonNode result = JsonUtils::assembleFromFiles(configList, files.getModScope(), isValid);

		return result;
	}
	else
	{
		isValid = true;
		return files;
	}
}

JsonNode JsonUtils::assembleFromFiles(const JsonNode & files)
{
	bool isValid = false;
	return assembleFromFiles(files, isValid);
}

JsonNode JsonUtils::assembleFromFiles(const std::vector<std::string> & files)
{
	bool isValid = false;
	return assembleFromFiles(files, "", isValid);
}

JsonNode JsonUtils::assembleFromFiles(const std::vector<std::string> & files, std::string modName, bool & isValid)
{
	isValid = true;
	JsonNode result;

	for(const auto & file : files)
	{
		JsonPath path = JsonPath::builtinTODO(file);

		if (CResourceHandler::get(modName)->existsResource(path))
		{
			bool isValidFile = false;
			JsonNode section(JsonPath::builtinTODO(file), modName, isValidFile);
			merge(result, section);
			isValid |= isValidFile;
		}
		else
		{
			logMod->error("Failed to find file %s", file);
			isValid = false;
		}
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
		JsonNode section(reinterpret_cast<std::byte *>(textData.first.get()), textData.second, resID.getName());
		merge(result, section);
	}
	return result;
}

void JsonUtils::detectConflicts(JsonNode & result, const JsonNode & left, const JsonNode & right, const std::string & keyName)
{
	switch (left.getType())
	{
		case JsonNode::JsonType::DATA_NULL:
		case JsonNode::JsonType::DATA_BOOL:
		case JsonNode::JsonType::DATA_FLOAT:
		case JsonNode::JsonType::DATA_INTEGER:
		case JsonNode::JsonType::DATA_STRING:
		case JsonNode::JsonType::DATA_VECTOR: // NOTE: comparing vectors as whole - since merge will overwrite it in its entirety
		{
			result[keyName][left.getModScope()] = left;
			result[keyName][right.getModScope()] = right;
			return;
		}
		case JsonNode::JsonType::DATA_STRUCT:
		{
			for(const auto & node : left.Struct())
				if (!right[node.first].isNull())
					detectConflicts(result, node.second, right[node.first], keyName + "/" + node.first);
		}
	}
}

VCMI_LIB_NAMESPACE_END
