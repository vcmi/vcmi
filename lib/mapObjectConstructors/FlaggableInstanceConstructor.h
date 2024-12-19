/*
* FlaggableInstanceConstructor.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "CDefaultObjectTypeHandler.h"

#include "../ResourceSet.h"
#include "../bonuses/Bonus.h"
#include "../mapObjects/FlaggableMapObject.h"

VCMI_LIB_NAMESPACE_BEGIN

class FlaggableInstanceConstructor final : public CDefaultObjectTypeHandler<FlaggableMapObject>
{
	/// List of bonuses that are provided by every map object of this type
	std::vector<std::shared_ptr<Bonus>> providedBonuses;

	/// ID of message to show on hero visit
	std::string visitMessageTextID;

	/// Amount of resources granted by this object to owner every day
	ResourceSet dailyIncome;

protected:
	void initTypeData(const JsonNode & config) override;
	void initializeObject(FlaggableMapObject * object) const override;

public:
	const std::string & getVisitMessageTextID() const;
	const std::vector<std::shared_ptr<Bonus>> & getProvidedBonuses() const;
	const ResourceSet & getDailyIncome() const;
};

VCMI_LIB_NAMESPACE_END
