/*
 * CArtifactFittingSet.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CArtifactFittingSet.h"

VCMI_LIB_NAMESPACE_BEGIN

CArtifactFittingSet::CArtifactFittingSet(IGameCallback *cb, ArtBearer bearer)
	: CArtifactSet(cb)
	, GameCallbackHolder(cb)
	, bearer(bearer)
{
}

CArtifactFittingSet::CArtifactFittingSet(const CArtifactSet & artSet)
	: CArtifactFittingSet(artSet.getCallback(), artSet.bearerType())
{
	artifactsWorn = artSet.artifactsWorn;
	artifactsInBackpack = artSet.artifactsInBackpack;
	artifactsTransitionPos = artSet.artifactsTransitionPos;
}

ArtBearer CArtifactFittingSet::bearerType() const
{
	return this->bearer;
}

VCMI_LIB_NAMESPACE_END
