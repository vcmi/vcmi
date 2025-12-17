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

class QuestArtifactPlacer : public Modificator
{
public:
	MODIFICATOR(QuestArtifactPlacer);

	void process() override;
	void init() override;

	void addQuestArtZone(std::shared_ptr<Zone> otherZone);
	void findZonesForQuestArts();

	void addQuestArtifact(const ArtifactID& id, ui32 desiredValue);
	void removeQuestArtifact(const ArtifactID& id);
	void rememberPotentialArtifactToReplace(CGObjectInstance* obj, ui32 value);
	CGObjectInstance * drawObjectToReplace(ui32 desiredValue);
	std::vector<CGObjectInstance*> getPossibleArtifactsToReplace() const;
	void placeQuestArtifacts(vstd::RNG & rand);
	void dropReplacedArtifact(const CGObjectInstance* obj);

	size_t getMaxQuestArtifactCount() const;
	[[nodiscard]] ArtifactID drawRandomArtifact();
	void addRandomArtifact(const ArtifactID & artid);

protected:
	struct QuestArtifactRequest
	{
		ArtifactID id;
		ui32 desiredValue;
	};

	struct ReplacementCandidate
	{
		CGObjectInstance* object;
		ui32 value;
	};

	std::vector<std::shared_ptr<Zone>> questArtZones; //artifacts required for Seer Huts will be placed here - or not if null
	std::vector<QuestArtifactRequest> questArtifactsToPlace;
	std::vector<ReplacementCandidate> artifactsToReplace; //Common artifacts which may be replaced by quest artifacts from other zones

	size_t maxQuestArtifacts;
	std::vector<ArtifactID> questArtifacts;
};

VCMI_LIB_NAMESPACE_END
