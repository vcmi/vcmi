/*
 * HeroType.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "Entity.h"

VCMI_LIB_NAMESPACE_BEGIN

class HeroTypeID;

class DLL_LINKAGE HeroType : public EntityT<HeroTypeID>
{
	virtual std::string getBiographyTranslated() const = 0;
	virtual std::string getSpecialtyNameTranslated() const = 0;
	virtual std::string getSpecialtyDescriptionTranslated() const = 0;
	virtual std::string getSpecialtyTooltipTranslated() const = 0;

	virtual std::string getBiographyTextID() const = 0;
	virtual std::string getSpecialtyNameTextID() const = 0;
	virtual std::string getSpecialtyDescriptionTextID() const = 0;
	virtual std::string getSpecialtyTooltipTextID() const = 0;

};


VCMI_LIB_NAMESPACE_END
