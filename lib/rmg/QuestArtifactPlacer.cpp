/*
* QuestArtifact.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/


#include "StdInc.h"
#include "QuestArtifactPlacer.h"
#include "CMapGenerator.h"
#include "RmgMap.h"
#include "TreasurePlacer.h"
#include "CZonePlacer.h"
#include "../VCMI_Lib.h"
#include "../mapObjects/CObjectHandler.h"
#include "../mapObjects/CommonConstructors.h"
#include "../mapObjects/MapObjects.h" 

void QuestArtifactPlacer::process()
{
	findZonesForQuestArts();
	placeQuestArtifacts(&generator.rand);
}

void QuestArtifactPlacer::init()
{
	DEPENDENCY_ALL(TreasurePlacer);
}

void QuestArtifactPlacer::addQuestArtZone(std::shared_ptr<Zone> otherZone)
{
	questArtZones.push_back(otherZone);
}

void QuestArtifactPlacer::addQuestArtifact(const ArtifactID& id)
{
	logGlobal->info("Need to place quest artifact artifact %s", VLC->artifacts()->getById(id)->getNameTranslated());
	questArtifactsToPlace.emplace_back(id);
}

void QuestArtifactPlacer::rememberPotentialArtifactToReplace(CGObjectInstance* obj)
{
	artifactsToReplace.push_back(obj);
}

std::vector<CGObjectInstance*> QuestArtifactPlacer::getPossibleArtifactsToReplace() const
{
	return artifactsToReplace;
}

void QuestArtifactPlacer::findZonesForQuestArts()
{
	const auto& distances = generator.getZonePlacer()->getDistanceMap().at(zone.getId());
	for (auto const& connectedZone : distances)
	{
		// Choose zones that are 1 or 2 connections away
		if (vstd::iswithin(connectedZone.second, 1, 2))
		{
			addQuestArtZone(map.getZones().at(connectedZone.first));
		}
	}

	logGlobal->info("Number of nearby zones suitable for quest artifacts: %d", questArtZones.size());
}

void QuestArtifactPlacer::placeQuestArtifacts(CRandomGenerator * rand)
{
	for (const auto & artifactToPlace : questArtifactsToPlace)
	{
		RandomGeneratorUtil::randomShuffle(questArtZones, *rand);
		for (auto zone : questArtZones)
		{
			auto* qap = zone->getModificator<QuestArtifactPlacer>();
			std::vector<CGObjectInstance *> artifactsToReplace = qap->getPossibleArtifactsToReplace();
			if (artifactsToReplace.empty())
				continue;

			auto artifactToReplace = *RandomGeneratorUtil::nextItem(artifactsToReplace, *rand);
			logGlobal->info("Replacing %s at %s with the quest artifact %s",
				artifactToReplace->getObjectName(),
				artifactToReplace->getPosition().toString(),
				VLC->artifacts()->getById(artifactToPlace)->getNameTranslated());
			artifactToReplace->ID = Obj::ARTIFACT;
			artifactToReplace->subID = artifactToPlace;

			//Update appearance. Terrain is irrelevant.
			auto handler = VLC->objtypeh->getHandlerFor(Obj::ARTIFACT, artifactToPlace);
			auto templates = handler->getTemplates();
			artifactToReplace->appearance = templates.front();
			//FIXME: Instance name is still "randomArtifact"

			for (auto z : map.getZones())
			{
				//Every qap has its OWN collection of artifacts
				auto * localQap = zone->getModificator<QuestArtifactPlacer>();
				if (localQap)
				{
					localQap->dropReplacedArtifact(artifactToReplace);
				}
			}
			break;
		}
	}
}

void QuestArtifactPlacer::dropReplacedArtifact(CGObjectInstance* obj)
{
	boost::remove(artifactsToReplace, obj);
}

size_t QuestArtifactPlacer::getMaxQuestArtifactCount() const
{
	return questArtifacts.size();
}

ArtifactID QuestArtifactPlacer::drawRandomArtifact()
{
	if (!questArtifacts.empty())
	{
		ArtifactID ret = questArtifacts.back();
		questArtifacts.pop_back();
		RandomGeneratorUtil::randomShuffle(questArtifacts, generator.rand);
		return ret;
	}
	else
	{
		throw rmgException("No quest artifacts left for this zone!");
	}
}

void QuestArtifactPlacer::addRandomArtifact(ArtifactID artid)
{
	questArtifacts.push_back(artid);
}
