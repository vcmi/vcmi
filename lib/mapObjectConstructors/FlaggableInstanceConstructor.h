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
#include "../bonuses/Bonus.h"
#include "../mapObjects/FlaggableMapObject.h"

VCMI_LIB_NAMESPACE_BEGIN

class FlaggableInstanceConstructor final : public CDefaultObjectTypeHandler<FlaggableMapObject>
{
	std::vector<std::shared_ptr<Bonus>> providedBonuses;

	/// ID of message to show on hero visit
	std::string visitMessageTextID;

protected:
	void initTypeData(const JsonNode & config) override;
	void initializeObject(FlaggableMapObject * object) const override;

public:
	const std::string & getVisitMessageTextID() const;
	const std::vector<std::shared_ptr<Bonus>> & getProvidedBonuses() const;
};

VCMI_LIB_NAMESPACE_END
