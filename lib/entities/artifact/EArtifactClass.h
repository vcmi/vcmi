/*
 * EArtifactClass.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

enum class EArtifactClass
{
	ART_SPECIAL = 1,
	ART_TREASURE = 2,
	ART_MINOR = 4,
	ART_MAJOR = 8,
	ART_RELIC = 16
};

VCMI_LIB_NAMESPACE_END
