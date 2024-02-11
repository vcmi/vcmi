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

#include "../ScopeGuard.h"
#include "../bonuses/BonusParams.h"
#include "../bonuses/Bonus.h"
#include "../bonuses/Limiters.h"
#include "../bonuses/Propagators.h"
#include "../bonuses/Updaters.h"
#include "../filesystem/Filesystem.h"
#include "../modding/IdentifierStorage.h"
#include "../VCMI_Lib.h" //for identifier resolution
#include "../CGeneralTextHandler.h"
#include "../constants/StringConstants.h"
#include "../battle/BattleHex.h"

VCMI_LIB_NAMESPACE_BEGIN

static const JsonNode nullNode;

//returns first Key with value equal to given one
template<class Key, class Val>
Key reverseMapFirst(const Val & val, const std::map<Key, Val> & map)
{
	for(auto it : map)
	{
		if(it.second == val)
		{
			return it.first;
		}
	}
	assert(0);
	return "";
}

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

	vstd::erase_if(node.Struct(), [&](const auto & node){
		return !vstd::contains(foundEntries, node.first);
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
	std::string log = Validation::check(schemaName, node);
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
			if(!ignoreOverride && vstd::contains(source.flags, "override"))
			{
				std::swap(dest, source);
			}
			else
			{
				if (copyMeta)
					dest.meta = source.meta;

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

JsonNode JsonUtils::intersect(const std::vector<JsonNode> & nodes, bool pruneEmpty)
{
	if(nodes.empty())
		return nullNode;

	JsonNode result = nodes[0];
	for(int i = 1; i < nodes.size(); i++)
	{
		if(result.isNull())
			break;
		result = JsonUtils::intersect(result, nodes[i], pruneEmpty);
	}
	return result;
}

JsonNode JsonUtils::intersect(const JsonNode & a, const JsonNode & b, bool pruneEmpty)
{
	if(a.getType() == JsonNode::JsonType::DATA_STRUCT && b.getType() == JsonNode::JsonType::DATA_STRUCT)
	{
		// intersect individual properties
		JsonNode result(JsonNode::JsonType::DATA_STRUCT);
		for(const auto & property : a.Struct())
		{
			if(vstd::contains(b.Struct(), property.first))
			{
				JsonNode propertyIntersect = JsonUtils::intersect(property.second, b.Struct().find(property.first)->second);
				if(pruneEmpty && !propertyIntersect.containsBaseData())
					continue;
				result[property.first] = propertyIntersect;
			}
		}
		return result;
	}
	else
	{
		// not a struct - same or different, no middle ground
		if(a == b)
			return a;
	}
	return nullNode;
}

JsonNode JsonUtils::difference(const JsonNode & node, const JsonNode & base)
{
	auto addsInfo = [](JsonNode diff) -> bool
	{
		switch(diff.getType())
		{
		case JsonNode::JsonType::DATA_NULL:
			return false;
		case JsonNode::JsonType::DATA_STRUCT:
			return !diff.Struct().empty();
		default:
			return true;
		}
	};

	if(node.getType() == JsonNode::JsonType::DATA_STRUCT && base.getType() == JsonNode::JsonType::DATA_STRUCT)
	{
		// subtract individual properties
		JsonNode result(JsonNode::JsonType::DATA_STRUCT);
		for(const auto & property : node.Struct())
		{
			if(vstd::contains(base.Struct(), property.first))
			{
				const JsonNode propertyDifference = JsonUtils::difference(property.second, base.Struct().find(property.first)->second);
				if(addsInfo(propertyDifference))
					result[property.first] = propertyDifference;
			}
			else
			{
				result[property.first] = property.second;
			}
		}
		return result;
	}
	else
	{
		if(node == base)
			return nullNode;
	}
	return node;
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
		// FIXME: some way to make this code more readable
		auto stream = loader->load(resID);
		std::unique_ptr<ui8[]> textData(new ui8[stream->getSize()]);
		stream->read(textData.get(), stream->getSize());

		JsonNode section(reinterpret_cast<char *>(textData.get()), stream->getSize());
		merge(result, section);
	}
	return result;
}

DLL_LINKAGE JsonNode JsonUtils::boolNode(bool value)
{
	JsonNode node;
	node.Bool() = value;
	return node;
}

DLL_LINKAGE JsonNode JsonUtils::floatNode(double value)
{
	JsonNode node;
	node.Float() = value;
	return node;
}

DLL_LINKAGE JsonNode JsonUtils::stringNode(const std::string & value)
{
	JsonNode node;
	node.String() = value;
	return node;
}

DLL_LINKAGE JsonNode JsonUtils::intNode(si64 value)
{
	JsonNode node;
	node.Integer() = value;
	return node;
}

VCMI_LIB_NAMESPACE_END
