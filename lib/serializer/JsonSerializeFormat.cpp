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

//JsonSerializeHelper
JsonSerializeHelper::JsonSerializeHelper(JsonSerializeHelper && other):
	owner(other.owner),
	thisNode(other.thisNode),
	parentNode(other.parentNode),
	restoreState(false)
{
	std::swap(restoreState, other.restoreState);
}

JsonSerializeHelper::~JsonSerializeHelper()
{
	if(restoreState)
		owner.current = parentNode;
}

JsonNode & JsonSerializeHelper::get()
{
	return * thisNode;
}

JsonSerializeFormat * JsonSerializeHelper::operator->()
{
	return &owner;
}

JsonSerializeHelper::JsonSerializeHelper(JsonSerializeFormat & owner_, JsonNode * thisNode_):
	owner(owner_),
	thisNode(thisNode_),
	parentNode(owner.current),
	restoreState(true)
{
	owner.current = thisNode;
}

JsonSerializeHelper::JsonSerializeHelper(JsonSerializeHelper & parent, const std::string & fieldName):
	owner(parent.owner),
	thisNode(&(parent.thisNode->operator[](fieldName))),
	parentNode(parent.thisNode),
	restoreState(true)
{
	owner.current = thisNode;
}

JsonStructSerializer JsonSerializeHelper::enterStruct(const std::string & fieldName)
{
	return JsonStructSerializer(*this, fieldName);
}

JsonArraySerializer JsonSerializeHelper::enterArray(const std::string & fieldName)
{
	return JsonArraySerializer(*this, fieldName);
}

//JsonStructSerializer
JsonStructSerializer::JsonStructSerializer(JsonStructSerializer && other):
	JsonSerializeHelper(std::move(static_cast<JsonSerializeHelper &>(other))),
	optional(other.optional)
{

}

JsonStructSerializer::JsonStructSerializer(JsonSerializeFormat & owner_, const std::string & fieldName):
	JsonSerializeHelper(owner_, &(owner_.current->operator[](fieldName))),
	optional(false)
{
	if(owner.saving)
		thisNode->setType(JsonNode::DATA_STRUCT);
}

JsonStructSerializer::JsonStructSerializer(JsonSerializeHelper & parent, const std::string & fieldName):
	JsonSerializeHelper(parent, fieldName),
	optional(false)
{
	if(owner.saving)
		thisNode->setType(JsonNode::DATA_STRUCT);
}

JsonStructSerializer::JsonStructSerializer(JsonSerializeFormat & owner_, JsonNode * thisNode_):
	JsonSerializeHelper(owner_, thisNode_),
	optional(false)
{
	if(owner.saving)
		thisNode->setType(JsonNode::DATA_STRUCT);
}

JsonStructSerializer::~JsonStructSerializer()
{
	//todo: delete empty optional node
	if(owner.saving && optional && thisNode->Struct().empty())
	{
		//
	}
}

//JsonArraySerializer
JsonArraySerializer::JsonArraySerializer(JsonStructSerializer && other):
	JsonSerializeHelper(std::move(static_cast<JsonSerializeHelper &>(other)))
{

}

JsonArraySerializer::JsonArraySerializer(JsonSerializeFormat & owner_, const std::string & fieldName):
	JsonSerializeHelper(owner_, &(owner_.current->operator[](fieldName)))
{

}

JsonArraySerializer::JsonArraySerializer(JsonSerializeHelper & parent, const std::string & fieldName):
	JsonSerializeHelper(parent, fieldName)
{

}

JsonStructSerializer JsonArraySerializer::enterStruct(const size_t index)
{
    return JsonStructSerializer(owner, &(thisNode->Vector()[index]));
}

void JsonArraySerializer::resize(const size_t newSize)
{
    thisNode->Vector().resize(newSize);
}

void JsonArraySerializer::resize(const size_t newSize, JsonNode::JsonType type)
{
	resize(newSize);

	for(JsonNode & n : thisNode->Vector())
		if(n.getType() == JsonNode::DATA_NULL)
			n.setType(type);
}


size_t JsonArraySerializer::size() const
{
    return thisNode->Vector().size();
}

//JsonSerializeFormat::LIC
JsonSerializeFormat::LIC::LIC(const std::vector<bool> & Standard, const TDecoder Decoder, const TEncoder Encoder):
	standard(Standard), decoder(Decoder), encoder(Encoder)
{
	any.resize(standard.size(), false);
	all.resize(standard.size(), false);
	none.resize(standard.size(), false);
}

JsonSerializeFormat::LICSet::LICSet(const std::set<si32>& Standard, const TDecoder Decoder, const TEncoder Encoder):
	standard(Standard), decoder(Decoder), encoder(Encoder)
{

}

//JsonSerializeFormat
JsonSerializeFormat::JsonSerializeFormat(const IInstanceResolver * instanceResolver_, JsonNode & root_, const bool saving_):
	saving(saving_),
	root(&root_),
	current(root),
	instanceResolver(instanceResolver_)
{

}

JsonStructSerializer JsonSerializeFormat::enterStruct(const std::string & fieldName)
{
	return JsonStructSerializer(*this, fieldName);
}

JsonArraySerializer JsonSerializeFormat::enterArray(const std::string & fieldName)
{
	return JsonArraySerializer(*this, fieldName);
}

void JsonSerializeFormat::serializeBool(const std::string & fieldName, bool & value)
{
	serializeBool<bool>(fieldName, value, true, false, false);
}
