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

void JsonDeserializer::serializeEnum(const std::string & fieldName, const std::string & trueValue, const std::string & falseValue, bool & value)
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

void JsonDeserializer::serializeIntId(const std::string & fieldName, const TDecoder & decoder, const TEncoder & encoder, const si32 defaultValue, si32 & value)
{
	std::string identifier;
	serializeString(fieldName, identifier);

	if(identifier == "")
	{
		value = defaultValue;
		return;
	}

	si32 rawId = decoder(identifier);
	if(rawId >= 0)
		value = rawId;
	else
		value = defaultValue;
}

void JsonDeserializer::serializeLIC(const std::string & fieldName, const TDecoder & decoder, const TEncoder & encoder, const std::vector<bool> & standard, std::vector<bool> & value)
{
	const JsonNode & field = current->operator[](fieldName);
	if(field.isNull())
		return;

	const JsonNode & anyOf = field["anyOf"];
	const JsonNode & allOf = field["allOf"];
	const JsonNode & noneOf = field["noneOf"];

	if(anyOf.Vector().empty() && allOf.Vector().empty())
	{
		//permissive mode
		value = standard;
	}
	else
	{
		//restrictive mode
		value.clear();
		value.resize(standard.size(), false);

		readLICPart(anyOf, decoder, true, value);
		readLICPart(allOf, decoder, true, value);
	}

	readLICPart(noneOf, decoder, false, value);
}

void JsonDeserializer::serializeLIC(const std::string & fieldName, LIC & value)
{
	const JsonNode & field = current->operator[](fieldName);

	const JsonNode & anyOf = field["anyOf"];
	const JsonNode & allOf = field["allOf"];
	const JsonNode & noneOf = field["noneOf"];

	if(anyOf.Vector().empty())
	{
		//permissive mode
		value.any = value.standard;
	}
	else
	{
		//restrictive mode
		value.any.clear();
		value.any.resize(value.standard.size(), false);

		readLICPart(anyOf, value.decoder, true, value.any);
	}

	readLICPart(allOf, value.decoder, true, value.all);
	readLICPart(noneOf, value.decoder, true, value.none);

	//remove any banned from allowed and required
	for(si32 idx = 0; idx < value.none.size(); idx++)
	{
		if(value.none[idx])
		{
			value.all[idx] = false;
			value.any[idx] = false;
		}
	}

	//add all required to allowed
	for(si32 idx = 0; idx < value.all.size(); idx++)
	{
		if(value.all[idx])
		{
			value.any[idx] = true;
		}
	}
}

void JsonDeserializer::serializeString(const std::string & fieldName, std::string & value)
{
	value = current->operator[](fieldName).String();
}

void JsonDeserializer::readLICPart(const JsonNode & part, const TDecoder & decoder, const bool val, std::vector<bool> & value)
{
	for(size_t index = 0; index < part.Vector().size(); index++)
	{
		const std::string & identifier = part.Vector()[index].String();

		const si32 rawId = decoder(identifier);
		if(rawId >= 0)
		{
			if(rawId < value.size())
				value[rawId] = val;
			else
				logGlobal->errorStream() << "JsonDeserializer::serializeLIC: id out of bounds " << rawId;
		}
	}
}

