/*
 * CStackBasicDescriptor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CStackBasicDescriptor.h"

#include "../../CCreatureHandler.h"
#include "../../GameLibrary.h"
#include "../../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

//This constructor should be placed here to avoid side effects
CStackBasicDescriptor::CStackBasicDescriptor() = default;

CStackBasicDescriptor::CStackBasicDescriptor(const CreatureID & id, TQuantity Count)
	: typeID(id)
	, count(Count)
{
}

CStackBasicDescriptor::CStackBasicDescriptor(const CCreature * c, TQuantity Count)
	: typeID(c ? c->getId() : CreatureID())
	, count(Count)
{
}

const CCreature * CStackBasicDescriptor::getCreature() const
{
	return typeID.hasValue() ? typeID.toCreature() : nullptr;
}

const Creature * CStackBasicDescriptor::getType() const
{
	return typeID.hasValue() ? typeID.toEntity(LIBRARY) : nullptr;
}

CreatureID CStackBasicDescriptor::getId() const
{
	return typeID;
}

TQuantity CStackBasicDescriptor::getCount() const
{
	return count;
}

void CStackBasicDescriptor::setType(const CCreature * c)
{
	typeID = c ? c->getId() : CreatureID();
}

void CStackBasicDescriptor::setCount(TQuantity newCount)
{
	assert(newCount >= 0);
	count = newCount;
}

bool operator==(const CStackBasicDescriptor & l, const CStackBasicDescriptor & r)
{
	return l.typeID == r.typeID && l.count == r.count;
}

void CStackBasicDescriptor::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeInt("amount", count);

	if(handler.saving)
	{
		if(typeID.hasValue())
		{
			std::string typeName = typeID.toEntity(LIBRARY)->getJsonKey();
			handler.serializeString("type", typeName);
		}
	}
	else
	{
		std::string typeName;
		handler.serializeString("type", typeName);
		if(!typeName.empty())
			setType(CreatureID(CreatureID::decode(typeName)).toCreature());
	}
}

VCMI_LIB_NAMESPACE_END
