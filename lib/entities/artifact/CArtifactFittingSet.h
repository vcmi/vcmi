/*
 * CArtifactFittingSet.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CArtifactSet.h"

#include "../../GameCallbackHolder.h"

VCMI_LIB_NAMESPACE_BEGIN

// Used to try on artifacts before the claimed changes have been applied
class DLL_LINKAGE CArtifactFittingSet : public CArtifactSet, public GameCallbackHolder
{
	IGameCallback * getCallback() const final
	{
		return cb;
	}

public:
	CArtifactFittingSet(IGameCallback * cb, ArtBearer::ArtBearer Bearer);
	explicit CArtifactFittingSet(const CArtifactSet & artSet);
	ArtBearer::ArtBearer bearerType() const override;

protected:
	ArtBearer::ArtBearer bearer;
};

VCMI_LIB_NAMESPACE_END
