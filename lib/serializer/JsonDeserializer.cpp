/*
 * JsonDeserializer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */


#include "StdInc.h"
#include "JsonDeserializer.h"

#include "../JsonNode.h"

JsonDeserializer::JsonDeserializer(JsonNode & root_):
	JsonSerializeFormat(root_, false)
{

}

void JsonDeserializer::serializeBool(const std::string & fieldName, bool & value)
{
	value = current->operator[](fieldName).Bool();
}

void JsonDeserializer::serializeBoolEnum(const std::string & fieldName, const std::string & trueValue, const std::string & falseValue, bool & value)
{
	const JsonNode & tmp = current->operator[](fieldName);

	value = tmp.String() == trueValue;
}

void JsonDeserializer::serializeFloat(const std::string & fieldName, double & value)
{
	value = current->operator[](fieldName).Float();
}

void JsonDeserializer::serializeIntEnum(const std::string & fieldName, const std::vector<std::string> & enumMap, const si32 defaultValue, si32 & value)
{
	const std::string & valueName = current->operator[](fieldName).String();

	si32 rawValue = vstd::find_pos(enumMap, valueName);
	if(rawValue < 0)
		value = defaultValue;
	else
		value = rawValue;
}

void JsonDeserializer::serializeLIC(const std::string & fieldName, const TDecoder & decoder, const TEncoder & encoder, const std::vector<bool> & standard, std::vector<bool> & value)
{
	const JsonNode & field = current->operator[](fieldName);
	if(field.isNull())
		return;

	auto loadPart = [&](const JsonVector & part, const bool val)
	{
		for(size_t index = 0; index < part.size(); index++)
		{
			const std::string & identifier = part[index].String();

			logGlobal->debugStream() << "serializeLIC: " << fieldName << " " << identifier;

			si32 rawId = decoder(identifier);
			if(rawId >= 0)
			{
				if(rawId < value.size())
					value[rawId] = val;
				else
					logGlobal->errorStream() << "JsonDeserializer::serializeLIC: " << fieldName <<" id out of bounds " << rawId;
			}
			else
			{
				logGlobal->errorStream() << "JsonDeserializer::serializeLIC: " << fieldName <<" identifier not resolved " << identifier;
			}
		}
	};

	const JsonVector & anyOf = field["anyOf"].Vector();
	const JsonVector & allOf = field["allOf"].Vector();
	const JsonVector & noneOf = field["noneOf"].Vector();

	if(anyOf.empty() && allOf.empty())
	{
		//permissive mode
		value = standard;
	}
	else
	{
		//restrictive mode
		value.clear();
		value.resize(standard.size(), false);

		loadPart(anyOf, true);
		loadPart(allOf, true);
	}

	loadPart(noneOf, false);
}

void JsonDeserializer::serializeString(const std::string & fieldName, std::string & value)
{
	value = current->operator[](fieldName).String();
}

