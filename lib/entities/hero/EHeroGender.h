/*
 * EHeroGender.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

	enum class EHeroGender : int8_t
{
	DEFAULT = -1, // from h3m, instance has same gender as hero type
	MALE = 0,
	FEMALE = 1,
};

VCMI_LIB_NAMESPACE_END
