/*
 * ArtifactsUIController.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
 #pragma once

#include "../lib/constants/EntityIdentifiers.h"
 
VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;

VCMI_LIB_NAMESPACE_END

class ArtifactsUIController
{
public:
	size_t numOfMovedArts;
	size_t numOfArtsAskAssembleSession;
	std::set<ArtifactID> ignoredArtifacts;

	boost::mutex askAssembleArtifactsMutex;

	bool askToAssemble(const CGHeroInstance * hero, const ArtifactPosition & slot, std::set<ArtifactID> * ignoredArtifacts = nullptr);
	bool askToDisassemble(const CGHeroInstance * hero, const ArtifactPosition & slot);

	void artifactRemoved();
	void artifactMoved();
	void bulkArtMovementStart(size_t numOfArts);
	void artifactAssembled();
	void artifactDisassembled();
};
 