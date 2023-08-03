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

#include <vstd/StringUtils.h>

VCMI_LIB_NAMESPACE_BEGIN

JsonDeserializer::JsonDeserializer(const IInstanceResolver * instanceResolver_, const JsonNode & root_):
	JsonTreeSerializer(instanceResolver_, &root_, false, false)
{

}

void JsonDeserializer::serializeInternal(const std::string & fieldName, boost::logic::tribool & value)
{
	const JsonNode & data = currentObject->operator[](fieldName);
	if(data.getType() != JsonNode::JsonType::DATA_BOOL)
		value = boost::logic::indeterminate;
	else
		value = data.Bool();
}

void JsonDeserializer::serializeInternal(const std::string & fieldName, si32 & value, const std::optional<si32> & defaultValue, const TDecoder & decoder, const TEncoder & encoder)
{
	std::string identifier;
	serializeString(fieldName, identifier);

	value = defaultValue.value_or(0);

	if(!identifier.empty())
	{
		si32 rawId = decoder(identifier);

		if(rawId < 0) //may be, user has installed the mod into another directory...
		{
			auto internalId = vstd::splitStringToPair(identifier, ':').second;
			auto currentScope = getCurrent().meta;
			auto actualId = currentScope.length() > 0 ? currentScope + ":" + internalId : internalId;

			rawId = decoder(actualId);

			if(rawId >= 0)
				logMod->warn("Identifier %s has been resolved as %s instead of %s", internalId, actualId, identifier);
		}
		if(rawId >= 0)
			value = rawId;
	}
}

void JsonDeserializer::serializeInternal(const std::string & fieldName, std::vector<si32> & value, const TDecoder & decoder, const TEncoder & encoder)
{
	const JsonVector & data = currentObject->operator[](fieldName).Vector();

	value.clear();
	value.reserve(data.size());

	for(const JsonNode& elem : data)
	{
		si32 rawId = decoder(elem.String());

		if(rawId >= 0)
			value.push_back(rawId);
	}
}

void JsonDeserializer::serializeInternal(const std::string & fieldName, double & value, const std::optional<double> & defaultValue)
{
	const JsonNode & data = currentObject->operator[](fieldName);

	if(!data.isNumber())
		value = defaultValue.value_or(0);//todo: report error on not null?
	else
		value = data.Float();
}

void JsonDeserializer::serializeInternal(const std::string & fieldName, si64 & value, const std::optional<si64> & defaultValue)
{
	const JsonNode & data = currentObject->operator[](fieldName);

	if(!data.isNumber())
		value = defaultValue.value_or(0);//todo: report error on not null?
	else
		value = data.Integer();
}

void JsonDeserializer::serializeInternal(const std::string & fieldName, si32 & value, const std::optional<si32> & defaultValue, const std::vector<std::string> & enumMap)
{
	const std::string & valueName = currentObject->operator[](fieldName).String();

	const si32 actualOptional = defaultValue.value_or(0);

	si32 rawValue = vstd::find_pos(enumMap, valueName);
	if(rawValue < 0)
		value = actualOptional;
	else
		value = rawValue;
}

void JsonDeserializer::serializeInternal(std::string & value)
{
	value = currentObject->String();
}

void JsonDeserializer::serializeInternal(int64_t & value)
{
	value = currentObject->Integer();
}

void JsonDeserializer::serializeLIC(const std::string & fieldName, const TDecoder & decoder, const TEncoder & encoder, const std::vector<bool> & standard, std::vector<bool> & value)
{
	const JsonNode & field = currentObject->operator[](fieldName);

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
	const JsonNode & field = currentObject->operator[](fieldName);

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

void JsonDeserializer::serializeLIC(const std::string & fieldName, LICSet & value)
{
	const JsonNode & field = currentObject->operator[](fieldName);

	const JsonNode & anyOf = field["anyOf"];
	const JsonNode & allOf = field["allOf"];
	const JsonNode & noneOf = field["noneOf"];

	value.all.clear();
	value.none.clear();

	if(anyOf.Vector().empty())
	{
		//permissive mode
		value.any = value.standard;
	}
	else
	{
		//restrictive mode
		value.any.clear();
		readLICPart(anyOf, value.decoder, value.any);

		for(si32 item : value.standard)
			if(!vstd::contains(value.any, item))
				value.none.insert(item);
	}

	readLICPart(allOf, value.decoder, value.all);
	readLICPart(noneOf, value.decoder, value.none);

	//remove any banned from allowed and required
	auto isBanned = [&value](const si32 item)->bool
	{
		return vstd::contains(value.none, item);
	};
	vstd::erase_if(value.all, isBanned);
	vstd::erase_if(value.any, isBanned);

	//add all required to allowed
	for(si32 item : value.all)
	{
		value.any.insert(item);
	}
}

void JsonDeserializer::serializeString(const std::string & fieldName, std::string & value)
{
	value = currentObject->operator[](fieldName).String();
}

void JsonDeserializer::serializeRaw(const std::string & fieldName, JsonNode & value, const std::optional<std::reference_wrapper<const JsonNode>> defaultValue)
{
	const JsonNode & data = currentObject->operator[](fieldName);
	if(data.getType() == JsonNode::JsonType::DATA_NULL)
	{
		if(defaultValue)
			value = defaultValue.value();
		else
			value.clear();
	}
	else
	{
		value = data;
	}
}

VCMI_LIB_NAMESPACE_END
