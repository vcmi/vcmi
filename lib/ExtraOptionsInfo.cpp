/*
 * ExtraOptionsInfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ExtraOptionsInfo.h"

VCMI_LIB_NAMESPACE_BEGIN

bool ExtraOptionsInfo::operator == (const ExtraOptionsInfo & other) const
{
	return cheatsAllowed == other.cheatsAllowed &&
			unlimitedReplay == other.unlimitedReplay;
}

VCMI_LIB_NAMESPACE_END
