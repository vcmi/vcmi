/*
 * JsonSerializeFormat.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "JsonSerializeFormat.h"

#include "../JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

//JsonSerializeHelper
JsonSerializeHelper::JsonSerializeHelper(JsonSerializeHelper && other) noexcept: owner(other.owner), restoreState(false)
{
	std::swap(restoreState, other.restoreState);
}

JsonSerializeHelper::~JsonSerializeHelper()
{
	if(restoreState)
		owner->pop();
}

JsonSerializeFormat * JsonSerializeHelper::operator->()
{
	return owner;
}

JsonSerializeHelper::JsonSerializeHelper(JsonSerializeFormat * owner_)
	: owner(owner_),
	restoreState(true)
{
}

//JsonStructSerializer
JsonStructSerializer::JsonStructSerializer(JsonStructSerializer && other) noexcept: JsonSerializeHelper(std::move(static_cast<JsonSerializeHelper &>(other))) {}

JsonStructSerializer::JsonStructSerializer(JsonSerializeFormat * owner_)
	: JsonSerializeHelper(owner_)
{
}

//JsonArraySerializer
JsonArraySerializer::JsonArraySerializer(JsonArraySerializer && other) noexcept: JsonSerializeHelper(std::move(static_cast<JsonSerializeHelper &>(other))) {}

JsonArraySerializer::JsonArraySerializer(JsonSerializeFormat * owner_): JsonSerializeHelper(owner_), thisNode(&owner->getCurrent()) {}

JsonStructSerializer JsonArraySerializer::enterStruct(const size_t index)
{
	owner->pushArrayElement(index);
	return JsonStructSerializer(owner);
}

JsonArraySerializer JsonArraySerializer::enterArray(const size_t index)
{
	owner->pushArrayElement(index);
	return JsonArraySerializer(owner);
}

void JsonArraySerializer::serializeString(const size_t index, std::string & value)
{
	owner->pushArrayElement(index);
	owner->serializeInternal(value);
	owner->pop();
}

void JsonArraySerializer::serializeInt64(const size_t index, int64_t & value)
{
	owner->pushArrayElement(index);
	owner->serializeInternal(value);
	owner->pop();
}

void JsonArraySerializer::resize(const size_t newSize)
{
	resize(newSize, JsonNode::JsonType::DATA_NULL);
}

void JsonArraySerializer::resize(const size_t newSize, JsonNode::JsonType type)
{
	owner->resizeCurrent(newSize, type);
}

size_t JsonArraySerializer::size() const
{
    return thisNode->Vector().size();
}

//JsonSerializeFormat::LIC
JsonSerializeFormat::LIC::LIC(const std::vector<bool> & Standard, TDecoder Decoder, TEncoder Encoder):
	standard(Standard),
	decoder(std::move(Decoder)),
	encoder(std::move(Encoder))
{
	any.resize(standard.size(), false);
	all.resize(standard.size(), false);
	none.resize(standard.size(), false);
}

JsonSerializeFormat::LICSet::LICSet(const std::set<si32> & Standard, TDecoder Decoder, TEncoder Encoder):
	standard(Standard),
	decoder(std::move(Decoder)),
	encoder(std::move(Encoder))
{

}

//JsonSerializeFormat
JsonSerializeFormat::JsonSerializeFormat(const IInstanceResolver * instanceResolver_, const bool saving_, const bool updating_)
	: saving(saving_),
	updating(updating_),
	instanceResolver(instanceResolver_)
{
}

JsonStructSerializer JsonSerializeFormat::enterStruct(const std::string & fieldName)
{
	pushStruct(fieldName);
	return JsonStructSerializer(this);
}

JsonArraySerializer JsonSerializeFormat::enterArray(const std::string & fieldName)
{
	pushArray(fieldName);
	return JsonArraySerializer(this);
}

void JsonSerializeFormat::serializeBool(const std::string & fieldName, bool & value)
{
	serializeBool<bool>(fieldName, value, true, false, false);
}

void JsonSerializeFormat::serializeBool(const std::string & fieldName, bool & value, const bool defaultValue)
{
	serializeBool<bool>(fieldName, value, true, false, defaultValue);
}

VCMI_LIB_NAMESPACE_END
