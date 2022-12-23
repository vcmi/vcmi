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

VCMI_LIB_NAMESPACE_BEGIN

JsonSerializer::JsonSerializer(const IInstanceResolver * instanceResolver_, JsonNode & root_):
	JsonTreeSerializer(instanceResolver_, &root_, true, false)
{

}

void JsonSerializer::serializeInternal(const std::string & fieldName, boost::logic::tribool & value)
{
	if(!boost::logic::indeterminate(value))
		currentObject->operator[](fieldName).Bool() = (bool)value;
}

void JsonSerializer::serializeInternal(const std::string & fieldName, si32 & value, const boost::optional<si32> & defaultValue, const TDecoder & decoder, const TEncoder & encoder)
{
	if(!defaultValue || defaultValue.get() != value)
	{
		std::string identifier = encoder(value);
		serializeString(fieldName, identifier);
	}
}

void JsonSerializer::serializeInternal(const std::string & fieldName, double & value, const boost::optional<double> & defaultValue)
{
	if(!defaultValue || defaultValue.get() != value)
		currentObject->operator[](fieldName).Float() = value;
}

void JsonSerializer::serializeInternal(const std::string & fieldName, si64 & value, const boost::optional<si64> & defaultValue)
{
	if(!defaultValue || defaultValue.get() != value)
		currentObject->operator[](fieldName).Integer() = value;
}

void JsonSerializer::serializeInternal(const std::string & fieldName, si32 & value, const boost::optional<si32> & defaultValue, const std::vector<std::string> & enumMap)
{
	if(!defaultValue || defaultValue.get() != value)
		currentObject->operator[](fieldName).String() = enumMap.at(value);
}

void JsonSerializer::serializeInternal(const std::string & fieldName, std::vector<si32> & value, const TDecoder & decoder, const TEncoder & encoder)
{
	if(value.empty())
		return;

	JsonVector & data = currentObject->operator[](fieldName).Vector();
	data.reserve(value.size());

	for(const si32 rawId : value)
	{
        JsonNode jsonElement(JsonNode::JsonType::DATA_STRING);
        jsonElement.String() = encoder(rawId);
        data.push_back(std::move(jsonElement));
	}
}

void JsonSerializer::serializeInternal(std::string & value)
{
	currentObject->String() = value;
}

void JsonSerializer::serializeInternal(int64_t & value)
{
	currentObject->Integer() = value;
}

void JsonSerializer::serializeLIC(const std::string & fieldName, const TDecoder & decoder, const TEncoder & encoder, const std::vector<bool> & standard, std::vector<bool> & value)
{
	assert(standard.size() == value.size());
	if(standard == value)
		return;

	writeLICPart(fieldName, "anyOf", encoder, value);
}

void JsonSerializer::serializeLIC(const std::string & fieldName, LIC & value)
{
	if(value.any != value.standard)
		writeLICPart(fieldName, "anyOf", value.encoder, value.any);

	writeLICPart(fieldName, "allOf", value.encoder, value.all);
	writeLICPart(fieldName, "noneOf", value.encoder, value.none);
}

void JsonSerializer::serializeLIC(const std::string & fieldName, LICSet & value)
{
	if(value.any != value.standard)
		writeLICPart(fieldName, "anyOf", value.encoder, value.any);

	writeLICPart(fieldName, "allOf", value.encoder, value.all);
	writeLICPart(fieldName, "noneOf", value.encoder, value.none);
}

void JsonSerializer::serializeString(const std::string & fieldName, std::string & value)
{
	if(!value.empty())
		currentObject->operator[](fieldName).String() = value;
}

void JsonSerializer::serializeRaw(const std::string & fieldName, JsonNode & value, const boost::optional<const JsonNode &> defaultValue)
{
	if(!defaultValue || value != defaultValue.get())
		currentObject->operator[](fieldName) = value;
}

void JsonSerializer::pushStruct(const std::string & fieldName)
{
	JsonTreeSerializer::pushStruct(fieldName);
	currentObject->setType(JsonNode::JsonType::DATA_STRUCT);
}

void JsonSerializer::pushArray(const std::string & fieldName)
{
	JsonTreeSerializer::pushArray(fieldName);
	currentObject->setType(JsonNode::JsonType::DATA_VECTOR);
}

void JsonSerializer::pushArrayElement(const size_t index)
{
	JsonTreeSerializer::pushArrayElement(index);
	currentObject->setType(JsonNode::JsonType::DATA_STRUCT);
}

void JsonSerializer::resizeCurrent(const size_t newSize, JsonNode::JsonType type)
{
	currentObject->Vector().resize(newSize);
	if(type != JsonNode::JsonType::DATA_NULL)
	{
		for(JsonNode & n : currentObject->Vector())
			if(n.getType() == JsonNode::JsonType::DATA_NULL)
				n.setType(type);
	}
}

void JsonSerializer::writeLICPart(const std::string & fieldName, const std::string & partName, const TEncoder & encoder, const std::vector<bool> & data)
{
	std::vector<std::string> buf;
	buf.reserve(data.size());

	for(si32 idx = 0; idx < data.size(); idx++)
		if(data[idx])
			buf.push_back(encoder(idx));

	writeLICPartBuffer(fieldName, partName, buf);
}

void JsonSerializer::writeLICPart(const std::string & fieldName, const std::string & partName, const TEncoder & encoder, const std::set<si32> & data)
{
	std::vector<std::string> buf;
	buf.reserve(data.size());

	for(const si32 item : data)
		buf.push_back(encoder(item));

	writeLICPartBuffer(fieldName, partName, buf);
}

void JsonSerializer::writeLICPartBuffer(const std::string & fieldName, const std::string & partName, std::vector<std::string> & buffer)
{
	if(!buffer.empty())
	{
		std::sort(buffer.begin(), buffer.end());

		auto & target = currentObject->operator[](fieldName)[partName].Vector();

		for(auto & s : buffer)
		{
			JsonNode val(JsonNode::JsonType::DATA_STRING);
			std::swap(val.String(), s);
			target.push_back(std::move(val));
		}
	}
}


VCMI_LIB_NAMESPACE_END
