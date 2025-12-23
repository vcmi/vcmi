/*
 * CStackBasicDescriptor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;
class CCreature;
class CGHeroInstance;
class CArmedInstance;
class CCreatureArtifactSet;
class JsonSerializeFormat;

class DLL_LINKAGE CStackBasicDescriptor
{
	CreatureID typeID;
	TQuantity count = -1; //exact quantity or quantity ID from CCreature::getQuantityID when getting info about enemy army

public:
	CStackBasicDescriptor();
	CStackBasicDescriptor(const CreatureID & id, TQuantity Count);
	CStackBasicDescriptor(const CCreature * c, TQuantity Count);
	virtual ~CStackBasicDescriptor() = default;

	const Creature * getType() const;
	const CCreature * getCreature() const;
	CreatureID getId() const;
	TQuantity getCount() const;

	virtual void setType(const CCreature * c);
	virtual void setCount(TQuantity amount);

	friend bool operator==(const CStackBasicDescriptor & l, const CStackBasicDescriptor & r);

	template<typename Handler>
	void serialize(Handler & h)
	{
		if(h.saving)
		{
			h & typeID;
		}
		else
		{
			CreatureID creatureID;
			h & creatureID;
			if(creatureID != CreatureID::NONE)
				setType(creatureID.toCreature());
		}

		h & count;
	}

	void serializeJson(JsonSerializeFormat & handler);
};

VCMI_LIB_NAMESPACE_END
