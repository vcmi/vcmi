/*
 * ArtifactService.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

class ArtifactID;
class Artifact;

class DLL_LINKAGE ArtifactService
{
public:
	virtual ~ArtifactService() = default;
	virtual const Artifact * getArtifact(const ArtifactID & artifactID) const = 0;
};
