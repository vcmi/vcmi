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
#include "../CMapGenerator.h"
#include "../RmgMap.h"
#include "TreasurePlacer.h"
#include "../CZonePlacer.h"
#include "../../GameLibrary.h"
#include "../../entities/artifact/CArtHandler.h"
#include "../../mapObjectConstructors/AObjectTypeHandler.h"
#include "../../mapObjectConstructors/CObjectClassesHandler.h"
#include "../../mapObjects/MapObjects.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

void QuestArtifactPlacer::process()
{
	findZonesForQuestArts();
	placeQuestArtifacts(zone.getRand());
}

void QuestArtifactPlacer::init()
{
	DEPENDENCY_ALL(TreasurePlacer);
}

void QuestArtifactPlacer::addQuestArtZone(std::shared_ptr<Zone> otherZone)
{
	RecursiveLock lock(externalAccessMutex);
	questArtZones.push_back(otherZone);
}

void QuestArtifactPlacer::addQuestArtifact(const ArtifactID& id)
{
	logGlobal->trace("Need to place quest artifact %s", LIBRARY->artifacts()->getById(id)->getNameTranslated());
	RecursiveLock lock(externalAccessMutex);
	questArtifactsToPlace.emplace_back(id);
}

void QuestArtifactPlacer::removeQuestArtifact(const ArtifactID& id)
{
	logGlobal->trace("Will not try to place quest artifact %s", LIBRARY->artifacts()->getById(id)->getNameTranslated());
	RecursiveLock lock(externalAccessMutex);
	vstd::erase_if_present(questArtifactsToPlace, id);
}

void QuestArtifactPlacer::rememberPotentialArtifactToReplace(CGObjectInstance* obj)
{
	RecursiveLock lock(externalAccessMutex);
	artifactsToReplace.push_back(obj);
}

std::vector<CGObjectInstance*> QuestArtifactPlacer::getPossibleArtifactsToReplace() const
{
	RecursiveLock lock(externalAccessMutex);
	return artifactsToReplace;
}

CGObjectInstance * QuestArtifactPlacer::drawObjectToReplace()
{
	RecursiveLock lock(externalAccessMutex);

	if (artifactsToReplace.empty())
	{
		return nullptr;
	}
	else
	{
		auto ret = *RandomGeneratorUtil::nextItem(artifactsToReplace, zone.getRand());
		vstd::erase_if_present(artifactsToReplace, ret);
		return ret;
	}
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

	logGlobal->trace("Number of nearby zones suitable for quest artifacts: %d", questArtZones.size());
}

void QuestArtifactPlacer::placeQuestArtifacts(vstd::RNG & rand)
{
	for (const auto & artifactToPlace : questArtifactsToPlace)
	{
		RandomGeneratorUtil::randomShuffle(questArtZones, rand);
		for (auto zone : questArtZones)
		{
			auto* qap = zone->getModificator<QuestArtifactPlacer>();
			
			auto objectToReplace = qap->drawObjectToReplace();
			if (!objectToReplace)
				continue;

			logGlobal->trace("Replacing %s at %s with the quest artifact %s",
				objectToReplace->getObjectName(),
				objectToReplace->anchorPos().toString(),
				LIBRARY->artifacts()->getById(artifactToPlace)->getNameTranslated());

			//Update appearance. Terrain is irrelevant.
			auto handler = LIBRARY->objtypeh->getHandlerFor(Obj::ARTIFACT, artifactToPlace);
			auto newObj = handler->create(map.mapInstance->cb, nullptr);
			auto templates = handler->getTemplates();
			//artifactToReplace->appearance = templates.front();
			newObj->appearance  = templates.front();
			newObj->setAnchorPos(objectToReplace->anchorPos());
			mapProxy->insertObject(newObj);
			mapProxy->removeObject(objectToReplace);
			break;
		}
	}
}

// TODO: Unused?
void QuestArtifactPlacer::dropReplacedArtifact(CGObjectInstance* obj)
{
	RecursiveLock lock(externalAccessMutex);
	boost::remove(artifactsToReplace, obj);
}

size_t QuestArtifactPlacer::getMaxQuestArtifactCount() const
{
	RecursiveLock lock(externalAccessMutex);
	return questArtifacts.size();
}

ArtifactID QuestArtifactPlacer::drawRandomArtifact()
{
	RecursiveLock lock(externalAccessMutex);
	if (!questArtifacts.empty())
	{
		RandomGeneratorUtil::randomShuffle(questArtifacts, zone.getRand());
		ArtifactID ret = questArtifacts.back();
		questArtifacts.pop_back();
		generator.banQuestArt(ret);
		return ret;
	}
	else
	{
		throw rmgException("No quest artifacts left for this zone!");
	}
}

void QuestArtifactPlacer::addRandomArtifact(const ArtifactID & artid)
{
	RecursiveLock lock(externalAccessMutex);
	questArtifacts.push_back(artid);
	generator.unbanQuestArt(artid);
}

VCMI_LIB_NAMESPACE_END
