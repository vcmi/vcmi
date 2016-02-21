/*
 * JsonSerializer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */


#include "StdInc.h"
#include "JsonSerializer.h"

#include "../JsonNode.h"

JsonSerializer::JsonSerializer(JsonNode & root_):
	JsonSerializeFormat(root_, true)
{

}

void JsonSerializer::serializeBool(const std::string & fieldName, bool & value)
{
	current->operator[](fieldName).Bool() = value;
}

void JsonSerializer::serializeBoolEnum(const std::string & fieldName, const std::string & trueValue, const std::string & falseValue, bool & value)
{
	current->operator[](fieldName).String() = value ? trueValue : falseValue;
}

void JsonSerializer::serializeFloat(const std::string & fieldName, double & value)
{
	current->operator[](fieldName).Float() = value;
}

void JsonSerializer::serializeIntEnum(const std::string & fieldName, const std::vector<std::string> & enumMap, const si32 defaultValue, si32 & value)
{
	current->operator[](fieldName).String() = enumMap.at(value);
}

void JsonSerializer::serializeIntId(const std::string & fieldName, const TDecoder & decoder, const TEncoder & encoder, const si32 defaultValue, si32& value)
{
	if(defaultValue == value)
		return;

	std::string identifier = encoder(value);
	serializeString(fieldName, identifier);
}

void JsonSerializer::serializeLIC(const std::string & fieldName, const TDecoder & decoder, const TEncoder & encoder, const std::vector<bool> & standard, std::vector<bool> & value)
{
	assert(standard.size() == value.size());
	if(standard == value)
		return;
	auto & target = current->operator[](fieldName)["anyOf"].Vector();
	for(si32 idx = 0; idx < value.size(); idx ++)
	{
		if(value[idx])
		{
			JsonNode val(JsonNode::DATA_STRING);
			val.String() = encoder(idx);
			target.push_back(std::move(val));
		}
	}
}


void JsonSerializer::serializeString(const std::string & fieldName, std::string & value)
{
	current->operator[](fieldName).String() = value;
}

