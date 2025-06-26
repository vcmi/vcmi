/*
 * ArtBearer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

#define ART_BEARER_LIST \
ART_BEARER(HERO)\
	ART_BEARER(CREATURE)\
	ART_BEARER(COMMANDER)\
	ART_BEARER(ALTAR)

enum class ArtBearer
{
#define ART_BEARER(x) x,
		ART_BEARER_LIST
#undef ART_BEARER
};

VCMI_LIB_NAMESPACE_END
