/*
* QuestArtifactPlacer, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once
#include "../Zone.h"
#include "../Functions.h"
#include "../../mapObjects/ObjectTemplate.h"

VCMI_LIB_NAMESPACE_BEGIN

class CRandomGenerator;

class QuestArtifactPlacer : public Modificator
{
public:
	MODIFICATOR(QuestArtifactPlacer);

	void process() override;
	void init() override;

	void addQuestArtZone(std::shared_ptr<Zone> otherZone);
	void findZonesForQuestArts();

	void addQuestArtifact(const ArtifactID& id);
	void rememberPotentialArtifactToReplace(CGObjectInstance* obj);
	std::vector<CGObjectInstance*> getPossibleArtifactsToReplace() const;
	void placeQuestArtifacts(CRandomGenerator & rand);
	void dropReplacedArtifact(CGObjectInstance* obj);

	size_t getMaxQuestArtifactCount() const;
	ArtifactID drawRandomArtifact();
	void addRandomArtifact(ArtifactID artid);

protected:

	std::vector<std::shared_ptr<Zone>> questArtZones; //artifacts required for Seer Huts will be placed here - or not if null
	std::vector<ArtifactID> questArtifactsToPlace;
	std::vector<CGObjectInstance*> artifactsToReplace; //Common artifacts which may be replaced by quest artifacts from other zones

	size_t maxQuestArtifacts;
	std::vector<ArtifactID> questArtifacts;
};

VCMI_LIB_NAMESPACE_END