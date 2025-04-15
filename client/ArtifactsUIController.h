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
#include "../lib/networkPacks/ArtifactLocation.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;

VCMI_LIB_NAMESPACE_END

class ArtifactsUIController
{
	size_t numOfMovedArts;
	size_t numOfArtsAskAssembleSession;
	std::set<ArtifactID> ignoredArtifacts;

public:
	ArtifactsUIController();
	bool askToAssemble(const ArtifactLocation & al, const bool onlyEquipped = false, const bool checkIgnored = false);
	bool askToAssemble(const CGHeroInstance * hero, const ArtifactPosition & slot, const bool onlyEquipped = false,
		const bool checkIgnored = false);
	bool askToDisassemble(const CGHeroInstance * hero, const ArtifactPosition & slot);

	void artifactRemoved();
	void artifactMoved();
	void bulkArtMovementStart(size_t totalNumOfArts, size_t possibleAssemblyNumOfArts);
	void artifactAssembled();
	void artifactDisassembled();
};
