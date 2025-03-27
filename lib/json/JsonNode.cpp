/*
 * JsonNode.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "JsonNode.h"

#include "JsonParser.h"
#include "JsonWriter.h"
#include "filesystem/Filesystem.h"

#include <boost/lexical_cast.hpp>

// to avoid duplicating const and non-const code
template<typename Node>
Node & resolvePointer(Node & in, const std::string & pointer)
{
	if(pointer.empty())
		return in;
	assert(pointer[0] == '/');

	size_t splitPos = pointer.find('/', 1);

	std::string entry = pointer.substr(1, splitPos - 1);
	std::string remainder = splitPos == std::string::npos ? "" : pointer.substr(splitPos);

	if(in.getType() == VCMI_LIB_WRAP_NAMESPACE(JsonNode)::JsonType::DATA_VECTOR)
	{
		if(entry.find_first_not_of("0123456789") != std::string::npos) // non-numbers in string
			throw std::runtime_error("Invalid Json pointer");

		if(entry.size() > 1 && entry[0] == '0') // leading zeros are not allowed
			throw std::runtime_error("Invalid Json pointer");

		auto index = boost::lexical_cast<size_t>(entry);

		if(in.Vector().size() > index)
			return in.Vector()[index].resolvePointer(remainder);
	}
	return in[entry].resolvePointer(remainder);
}

VCMI_LIB_NAMESPACE_BEGIN

static const JsonNode nullNode;

class GameLibrary;
class CModHandler;

JsonNode::JsonNode(bool boolean)
	: data(boolean)
{
}

JsonNode::JsonNode(int32_t number)
	: data(static_cast<int64_t>(number))
{
}

JsonNode::JsonNode(uint32_t number)
	: data(static_cast<int64_t>(number))
{
}

JsonNode::JsonNode(int64_t number)
	: data(number)
{
}

JsonNode::JsonNode(double number)
	: data(number)
{
}

JsonNode::JsonNode(const char * string)
	: data(std::string(string))
{
}

JsonNode::JsonNode(const std::string & string)
	: data(string)
{
}

JsonNode::JsonNode(const JsonMap & map)
	: data(map)
{
}

JsonNode::JsonNode(const std::byte * data, size_t datasize, const std::string & fileName)
	: JsonNode(data, datasize, JsonParsingSettings(), fileName)
{
}

JsonNode::JsonNode(const std::byte * data, size_t datasize, const JsonParsingSettings & parserSettings, const std::string & fileName)
{
	JsonParser parser(data, datasize, parserSettings);
	*this = parser.parse(fileName);
}

JsonNode::JsonNode(const JsonPath & fileURI)
	:JsonNode(fileURI, JsonParsingSettings())
{
}

JsonNode::JsonNode(const JsonPath & fileURI, const JsonParsingSettings & parserSettings)
{
	auto file = CResourceHandler::get()->load(fileURI)->readAll();

	JsonParser parser(reinterpret_cast<std::byte *>(file.first.get()), file.second, parserSettings);
	*this = parser.parse(fileURI.getName());
}

JsonNode::JsonNode(const JsonPath & fileURI, const std::string & modName)
{
	auto file = CResourceHandler::get(modName)->load(fileURI)->readAll();

	JsonParser parser(reinterpret_cast<std::byte *>(file.first.get()), file.second, JsonParsingSettings());
	*this = parser.parse(fileURI.getName());
}

JsonNode::JsonNode(const JsonPath & fileURI, const std::string & modName, bool & isValidSyntax)
{
	auto file = CResourceHandler::get(modName)->load(fileURI)->readAll();

	JsonParser parser(reinterpret_cast<std::byte *>(file.first.get()), file.second, JsonParsingSettings());
	*this = parser.parse(fileURI.getName());
	isValidSyntax = parser.isValid();
}

bool JsonNode::operator==(const JsonNode & other) const
{
	return data == other.data;
}

bool JsonNode::operator!=(const JsonNode & other) const
{
	return !(*this == other);
}

JsonNode::JsonType JsonNode::getType() const
{
	return static_cast<JsonType>(data.index());
}

const std::string & JsonNode::getModScope() const
{
	return modScope;
}

void JsonNode::setOverrideFlag(bool value)
{
	overrideFlag = value;
}

bool JsonNode::getOverrideFlag() const
{
	return overrideFlag;
}

void JsonNode::setModScope(const std::string & metadata, bool recursive)
{
	modScope = metadata;
	if(recursive)
	{
		switch(getType())
		{
			break;
			case JsonType::DATA_VECTOR:
			{
				for(auto & node : Vector())
				{
					node.setModScope(metadata);
				}
			}
			break;
			case JsonType::DATA_STRUCT:
			{
				for(auto & node : Struct())
				{
					node.second.setModScope(metadata);
				}
			}
		}
	}
}

void JsonNode::setType(JsonType Type)
{
	if(getType() == Type)
		return;

	//float<->int conversion
	if(getType() == JsonType::DATA_FLOAT && Type == JsonType::DATA_INTEGER)
	{
		si64 converted = static_cast<si64>(std::get<double>(data));
		data = JsonData(converted);
		return;
	}
	else if(getType() == JsonType::DATA_INTEGER && Type == JsonType::DATA_FLOAT)
	{
		double converted = static_cast<double>(std::get<si64>(data));
		data = JsonData(converted);
		return;
	}

	//Set new node type
	switch(Type)
	{
		case JsonType::DATA_NULL:
			data = JsonData();
			break;
		case JsonType::DATA_BOOL:
			data = JsonData(false);
			break;
		case JsonType::DATA_FLOAT:
			data = JsonData(0.0);
			break;
		case JsonType::DATA_STRING:
			data = JsonData(std::string());
			break;
		case JsonType::DATA_VECTOR:
			data = JsonData(JsonVector());
			break;
		case JsonType::DATA_STRUCT:
			data = JsonData(JsonMap());
			break;
		case JsonType::DATA_INTEGER:
			data = JsonData(static_cast<si64>(0));
			break;
	}
}

bool JsonNode::isNull() const
{
	return getType() == JsonType::DATA_NULL;
}

bool JsonNode::isNumber() const
{
	return getType() == JsonType::DATA_INTEGER || getType() == JsonType::DATA_FLOAT;
}

bool JsonNode::isString() const
{
	return getType() == JsonType::DATA_STRING;
}

bool JsonNode::isVector() const
{
	return getType() == JsonType::DATA_VECTOR;
}

bool JsonNode::isStruct() const
{
	return getType() == JsonType::DATA_STRUCT;
}

bool JsonNode::containsBaseData() const
{
	switch(getType())
	{
		case JsonType::DATA_NULL:
			return false;
		case JsonType::DATA_STRUCT:
			for(const auto & elem : Struct())
			{
				if(elem.second.containsBaseData())
					return true;
			}
			return false;
		default:
			//other types (including vector) cannot be extended via merge
			return true;
	}
}

bool JsonNode::isCompact() const
{
	switch(getType())
	{
		case JsonType::DATA_VECTOR:
			for(const JsonNode & elem : Vector())
			{
				if(!elem.isCompact())
					return false;
			}
			return true;
		case JsonType::DATA_STRUCT:
		{
			auto propertyCount = Struct().size();
			if(propertyCount == 0)
				return true;
			else if(propertyCount == 1)
				return Struct().begin()->second.isCompact();
		}
			return false;
		default:
			return true;
	}
}

bool JsonNode::TryBoolFromString(bool & success) const
{
	success = true;
	if(getType() == JsonNode::JsonType::DATA_BOOL)
		return Bool();

	success = getType() == JsonNode::JsonType::DATA_STRING;
	if(success)
	{
		auto boolParamStr = String();
		boost::algorithm::trim(boolParamStr);
		boost::algorithm::to_lower(boolParamStr);
		success = boolParamStr == "true";

		if(success)
			return true;

		success = boolParamStr == "false";
	}
	return false;
}

void JsonNode::clear()
{
	setType(JsonType::DATA_NULL);
}

bool & JsonNode::Bool()
{
	setType(JsonType::DATA_BOOL);
	return std::get<bool>(data);
}

double & JsonNode::Float()
{
	setType(JsonType::DATA_FLOAT);
	return std::get<double>(data);
}

si64 & JsonNode::Integer()
{
	setType(JsonType::DATA_INTEGER);
	return std::get<si64>(data);
}

std::string & JsonNode::String()
{
	setType(JsonType::DATA_STRING);
	return std::get<std::string>(data);
}

JsonVector & JsonNode::Vector()
{
	setType(JsonType::DATA_VECTOR);
	return std::get<JsonVector>(data);
}

JsonMap & JsonNode::Struct()
{
	setType(JsonType::DATA_STRUCT);
	return std::get<JsonMap>(data);
}

bool JsonNode::Bool() const
{
	static const bool boolDefault = false;

	assert(getType() == JsonType::DATA_NULL || getType() == JsonType::DATA_BOOL);

	if(getType() == JsonType::DATA_BOOL)
		return std::get<bool>(data);

	return boolDefault;
}

double JsonNode::Float() const
{
	static const double floatDefault = 0;

	assert(getType() == JsonType::DATA_NULL || getType() == JsonType::DATA_INTEGER || getType() == JsonType::DATA_FLOAT);

	if(getType() == JsonType::DATA_FLOAT)
		return std::get<double>(data);

	if(getType() == JsonType::DATA_INTEGER)
		return static_cast<double>(std::get<si64>(data));

	return floatDefault;
}

si64 JsonNode::Integer() const
{
	static const si64 integerDefault = 0;

	assert(getType() == JsonType::DATA_NULL || getType() == JsonType::DATA_INTEGER || getType() == JsonType::DATA_FLOAT);

	if(getType() == JsonType::DATA_INTEGER)
		return std::get<si64>(data);

	if(getType() == JsonType::DATA_FLOAT)
		return static_cast<si64>(std::get<double>(data));

	return integerDefault;
}

const std::string & JsonNode::String() const
{
	static const std::string stringDefault;

	assert(getType() == JsonType::DATA_NULL || getType() == JsonType::DATA_STRING);

	if(getType() == JsonType::DATA_STRING)
		return std::get<std::string>(data);

	return stringDefault;
}

const JsonVector & JsonNode::Vector() const
{
	static const JsonVector vectorDefault;

	assert(getType() == JsonType::DATA_NULL || getType() == JsonType::DATA_VECTOR);

	if(getType() == JsonType::DATA_VECTOR)
		return std::get<JsonVector>(data);

	return vectorDefault;
}

const JsonMap & JsonNode::Struct() const
{
	static const JsonMap mapDefault;

	assert(getType() == JsonType::DATA_NULL || getType() == JsonType::DATA_STRUCT);

	if(getType() == JsonType::DATA_STRUCT)
		return std::get<JsonMap>(data);

	return mapDefault;
}

JsonNode & JsonNode::operator[](const std::string & child)
{
	return Struct()[child];
}

const JsonNode & JsonNode::operator[](const std::string & child) const
{
	auto it = Struct().find(child);
	if(it != Struct().end())
		return it->second;
	return nullNode;
}

JsonNode & JsonNode::operator[](size_t child)
{
	if(child >= Vector().size())
		Vector().resize(child + 1);
	return Vector()[child];
}

const JsonNode & JsonNode::operator[](size_t child) const
{
	if(child < Vector().size())
		return Vector()[child];

	return nullNode;
}

const JsonNode & JsonNode::resolvePointer(const std::string & jsonPointer) const
{
	return ::resolvePointer(*this, jsonPointer);
}

JsonNode & JsonNode::resolvePointer(const std::string & jsonPointer)
{
	return ::resolvePointer(*this, jsonPointer);
}

std::vector<std::byte> JsonNode::toBytes() const
{
	std::string jsonString = toString();
	auto dataBegin = reinterpret_cast<const std::byte *>(jsonString.data());
	auto dataEnd = dataBegin + jsonString.size();
	std::vector<std::byte> result(dataBegin, dataEnd);
	return result;
}

std::string JsonNode::toCompactString() const
{
	std::ostringstream out;
	JsonWriter writer(out, true);
	writer.writeNode(*this);
	return out.str();
}

std::string JsonNode::toString() const
{
	std::ostringstream out;
	JsonWriter writer(out, false);
	writer.writeNode(*this);
	return out.str();
}

VCMI_LIB_NAMESPACE_END
