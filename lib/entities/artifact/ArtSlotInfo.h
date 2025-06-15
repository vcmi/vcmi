/*
 * ArtSlotInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CArtifactInstance.h"

#include "../../constants/EntityIdentifiers.h"
#include "../../callback/GameCallbackHolder.h"

VCMI_LIB_NAMESPACE_BEGIN

struct DLL_LINKAGE ArtSlotInfo : public GameCallbackHolder
{
	ArtifactInstanceID artifactID;
	bool locked = false; //if locked, then artifact points to the combined artifact

	explicit ArtSlotInfo(IGameInfoCallback * cb);
	ArtSlotInfo(const CArtifactInstance * artifact, bool locked);

	const CArtifactInstance * getArt() const;
	ArtifactInstanceID getID() const;

	template<typename Handler>
	void serialize(Handler & h)
	{
		if(h.saving || h.hasFeature(Handler::Version::NO_RAW_POINTERS_IN_SERIALIZER))
		{
			h & artifactID;
		}
		else
		{
			std::shared_ptr<CArtifactInstance> pointer;
			h & pointer;
			if(pointer->getId() == ArtifactInstanceID())
				CArtifactInstance::saveCompatibilityFixArtifactID(pointer);
			artifactID = pointer->getId();
		}
		h & locked;
	}
};

VCMI_LIB_NAMESPACE_END
