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


//JsonStructSerializer
JsonStructSerializer::JsonStructSerializer(JsonStructSerializer&& other):
	restoreState(false),
	owner(other.owner),
	parentNode(other.parentNode),
	thisNode(other.thisNode)
{

}

JsonStructSerializer::~JsonStructSerializer()
{
	if(restoreState)
		owner.current = parentNode;
}

JsonStructSerializer::JsonStructSerializer(JsonSerializeFormat& owner_, const std::string& fieldName):
	restoreState(true),
	owner(owner_),
	parentNode(owner.current),
	thisNode(&(parentNode->operator[](fieldName)))
{
	owner.current = thisNode;
}

JsonStructSerializer::JsonStructSerializer(JsonStructSerializer & parent, const std::string & fieldName):
	restoreState(true),
	owner(parent.owner),
	parentNode(parent.thisNode),
	thisNode(&(parentNode->operator[](fieldName)))
{
	owner.current = thisNode;
}

JsonStructSerializer JsonStructSerializer::enterStruct(const std::string & fieldName)
{
	return JsonStructSerializer(*this, fieldName);
}

JsonNode& JsonStructSerializer::get()
{
	return *thisNode;
}

JsonSerializeFormat * JsonStructSerializer::operator->()
{
	return &owner;
}

JsonSerializeFormat::LIC::LIC(const std::vector<bool> & Standard, const TDecoder & Decoder, const TEncoder & Encoder):
	standard(Standard), decoder(Decoder), encoder(Encoder)
{
	any.resize(standard.size(), false);
	all.resize(standard.size(), false);
	none.resize(standard.size(), false);
}


//JsonSerializeFormat
JsonSerializeFormat::JsonSerializeFormat(JsonNode & root_, const bool saving_):
	saving(saving_),
	root(&root_),
	current(root)
{

}

JsonStructSerializer JsonSerializeFormat::enterStruct(const std::string & fieldName)
{
	JsonStructSerializer res(*this, fieldName);

	return res;
}
