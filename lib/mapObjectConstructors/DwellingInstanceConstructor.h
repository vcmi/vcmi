/*
* DwellingInstanceConstructor.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "CDefaultObjectTypeHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGDwelling;

class DwellingInstanceConstructor : public CDefaultObjectTypeHandler<CGDwelling>
{
	std::vector<std::vector<const CCreature *>> availableCreatures;

	JsonNode guards;

protected:
	bool objectFilter(const CGObjectInstance * obj, std::shared_ptr<const ObjectTemplate> tmpl) const override;
	void initTypeData(const JsonNode & input) override;

public:
	bool hasNameTextID() const override;

	void initializeObject(CGDwelling * object) const override;
	void randomizeObject(CGDwelling * object, CRandomGenerator & rng) const override;

	bool producesCreature(const CCreature * crea) const;
	std::vector<const CCreature *> getProducedCreatures() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & availableCreatures;
		h & guards;
		h & static_cast<CDefaultObjectTypeHandler<CGDwelling>&>(*this);
	}
};

VCMI_LIB_NAMESPACE_END
