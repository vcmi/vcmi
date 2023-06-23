/*
 * EVictoryLossCheckResult.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "MetaString.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE EVictoryLossCheckResult
{
public:
	static EVictoryLossCheckResult victory(MetaString toSelf, MetaString toOthers)
	{
		return EVictoryLossCheckResult(VICTORY, toSelf, toOthers);
	}

	static EVictoryLossCheckResult defeat(MetaString toSelf, MetaString toOthers)
	{
		return EVictoryLossCheckResult(DEFEAT, toSelf, toOthers);
	}

	EVictoryLossCheckResult():
	intValue(0)
	{
	}

	bool operator==(EVictoryLossCheckResult const & other) const
	{
		return intValue == other.intValue;
	}

	bool operator!=(EVictoryLossCheckResult const & other) const
	{
		return intValue != other.intValue;
	}

	bool victory() const
	{
		return intValue == VICTORY;
	}
	bool loss() const
	{
		return intValue == DEFEAT;
	}

	EVictoryLossCheckResult invert()
	{
		return EVictoryLossCheckResult(-intValue, messageToOthers, messageToSelf);
	}

	MetaString messageToSelf;
	MetaString messageToOthers;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & intValue;
		h & messageToSelf;
		h & messageToOthers;
	}
private:
	enum EResult
	{
		DEFEAT = -1,
		INGAME =  0,
		VICTORY= +1
	};

	EVictoryLossCheckResult(si32 intValue, MetaString toSelf, MetaString toOthers):
		messageToSelf(toSelf),
		messageToOthers(toOthers),
		intValue(intValue)
	{
	}

	si32 intValue; // uses EResultult
};
