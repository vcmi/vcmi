/*
 * ArtSlotInfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "ArtSlotInfo.h"

#include "../../IGameCallback.h"

VCMI_LIB_NAMESPACE_BEGIN

ArtSlotInfo::ArtSlotInfo(IGameCallback * cb)
	: GameCallbackHolder(cb)
{
}

ArtSlotInfo::ArtSlotInfo(const CArtifactInstance * artifact, bool locked)
	: GameCallbackHolder(artifact->cb)
	, artifactID(artifact->getId())
	, locked(locked)
{
}

const CArtifactInstance * ArtSlotInfo::getArt() const
{
	if(!artifactID.hasValue())
		return nullptr;
	return cb->getArtInstance(artifactID);
}

ArtifactInstanceID ArtSlotInfo::getID() const
{
	return artifactID;
}

VCMI_LIB_NAMESPACE_END
