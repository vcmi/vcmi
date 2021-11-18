/*
 * JsonUpdater.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "JsonUpdater.h"

#include "../JsonNode.h"
#include "../HeroBonus.h"

JsonUpdater::JsonUpdater(const IInstanceResolver * instanceResolver_, const JsonNode & root_)
	: JsonTreeSerializer(instanceResolver_, &root_, false, true)
{

}

void JsonUpdater::serializeInternal(const std::string & fieldName, boost::logic::tribool & value)
{
	const JsonNode & data = currentObject->operator[](fieldName);
	if(data.getType() == JsonNode::JsonType::DATA_BOOL)
		value = data.Bool();
}

void JsonUpdater::serializeInternal(const std::string & fieldName, si32 & value, const boost::optional<si32> & defaultValue, const TDecoder & decoder, const TEncoder & encoder)
{
//	std::string identifier;
//	serializeString(fieldName, identifier);
//
//	value = defaultValue ? defaultValue.get() : 0;
//
//	if(identifier != "")
//	{
//		si32 rawId = decoder(identifier);
//		if(rawId >= 0)
//			value = rawId;
//	}
}

void JsonUpdater::serializeInternal(const std::string & fieldName, std::vector<si32> & value, const TDecoder & decoder, const TEncoder & encoder)
{
//	const JsonVector & data = currentObject->operator[](fieldName).Vector();
//
//	value.clear();
//	value.reserve(data.size());
//
//	for(const JsonNode elem : data)
//	{
//		si32 rawId = decoder(elem.String());
//
//		if(rawId >= 0)
//			value.push_back(rawId);
//	}
}

void JsonUpdater::serializeInternal(const std::string & fieldName, double & value, const boost::optional<double> & defaultValue)
{
	const JsonNode & data = currentObject->operator[](fieldName);

	if(data.isNumber())
		value = data.Float();
}

void JsonUpdater::serializeInternal(const std::string & fieldName, si64 & value, const boost::optional<si64> &)
{
	const JsonNode & data = currentObject->operator[](fieldName);

	if(data.isNumber())
		value = data.Integer();
}

void JsonUpdater::serializeInternal(const std::string & fieldName, si32 & value, const boost::optional<si32> & defaultValue, const std::vector<std::string> & enumMap)
{
//	const std::string & valueName = currentObject->operator[](fieldName).String();
//
//	const si32 actualOptional = defaultValue ? defaultValue.get() : 0;
//
//	si32 rawValue = vstd::find_pos(enumMap, valueName);
//	if(rawValue < 0)
//		value = actualOptional;
//	else
//		value = rawValue;
}

void JsonUpdater::serializeInternal(std::string & value)
{
	value = currentObject->String();
}

void JsonUpdater::serializeInternal(int64_t & value)
{
	value = currentObject->Integer();
}

void JsonUpdater::serializeLIC(const std::string & fieldName, const TDecoder & decoder, const TEncoder & encoder, const std::vector<bool> & standard, std::vector<bool> & value)
{
	const JsonNode & field = currentObject->operator[](fieldName);

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

void JsonUpdater::serializeLIC(const std::string & fieldName, LIC & value)
{
	const JsonNode & field = currentObject->operator[](fieldName);

	if(field.isNull())
		return;

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

void JsonUpdater::serializeLIC(const std::string & fieldName, LICSet & value)
{
	const JsonNode & field = currentObject->operator[](fieldName);

	if(field.isNull())
		return;

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

void JsonUpdater::serializeString(const std::string & fieldName, std::string & value)
{
	const JsonNode & data = currentObject->operator[](fieldName);
	if(data.getType() == JsonNode::JsonType::DATA_STRING)
		value = data.String();
}

void JsonUpdater::serializeRaw(const std::string & fieldName, JsonNode & value, const boost::optional<const JsonNode &> defaultValue)
{
	const JsonNode & data = currentObject->operator[](fieldName);
	if(data.getType() != JsonNode::JsonType::DATA_NULL)
		value = data;
}

void JsonUpdater::serializeBonuses(const std::string & fieldName, CBonusSystemNode * value)
{
	const JsonNode & data = currentObject->operator[](fieldName);

	const JsonNode & toAdd = data["toAdd"];

	if(toAdd.getType() == JsonNode::JsonType::DATA_VECTOR)
	{
		for(auto & item : toAdd.Vector())
		{
			auto b = JsonUtils::parseBonus(item);
			value->addNewBonus(b);
		}
	}

	const JsonNode & toRemove = data["toRemove"];

	if(toRemove.getType() == JsonNode::JsonType::DATA_VECTOR)
	{
		for(auto & item : toRemove.Vector())
		{
			auto mask = JsonUtils::parseBonus(item);

			auto selector = [mask](const Bonus * b)
			{
				//compare everything but turnsRemain, limiter and propagator
				return mask->duration == b->duration
				&& mask->type == b->type
				&& mask->subtype == b->subtype
				&& mask->source == b->source
				&& mask->val == b->val
				&& mask->sid == b->sid
				&& mask->valType == b->valType
				&& mask->additionalInfo == b->additionalInfo
				&& mask->effectRange == b->effectRange
				&& mask->description == b->description;
			};

			value->removeBonuses(selector);
		}
	}
}

void JsonUpdater::readLICPart(const JsonNode & part, const TDecoder & decoder, const bool val, std::vector<bool> & value)
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
				logGlobal->error("JsonUpdater::serializeLIC: id out of bounds %d", rawId);
		}
	}
}

void JsonUpdater::readLICPart(const JsonNode & part, const TDecoder & decoder, std::set<si32> & value)
{
	for(size_t index = 0; index < part.Vector().size(); index++)
	{
		const std::string & identifier = part.Vector()[index].String();

		const si32 rawId = decoder(identifier);
		if(rawId != -1)
			value.insert(rawId);
	}
}

