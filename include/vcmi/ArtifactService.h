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

#include "EntityService.h"

VCMI_LIB_NAMESPACE_BEGIN

class ArtifactID;
class Artifact;

class DLL_LINKAGE ArtifactService : public EntityServiceT<ArtifactID, Artifact>
{
public:
};

VCMI_LIB_NAMESPACE_END
