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

void QuestArtifactPlacer::addQuestArtifact(const ArtifactID& id, ui32 desiredValue)
{
	logGlobal->trace("Need to place quest artifact %s (desired value %u)",
		LIBRARY->artifacts()->getById(id)->getNameTranslated(),
		desiredValue);
	RecursiveLock lock(externalAccessMutex);
	questArtifactsToPlace.push_back({id, desiredValue});
}

void QuestArtifactPlacer::removeQuestArtifact(const ArtifactID& id)
{
	logGlobal->trace("Will not try to place quest artifact %s", LIBRARY->artifacts()->getById(id)->getNameTranslated());
	RecursiveLock lock(externalAccessMutex);
	vstd::erase_if(questArtifactsToPlace, [&id](const QuestArtifactRequest& request)
	{
		return request.id == id;
	});
}

void QuestArtifactPlacer::rememberPotentialArtifactToReplace(CGObjectInstance* obj, ui32 value)
{
	RecursiveLock lock(externalAccessMutex);
	artifactsToReplace.push_back({obj, value});
}

std::vector<CGObjectInstance*> QuestArtifactPlacer::getPossibleArtifactsToReplace() const
{
	RecursiveLock lock(externalAccessMutex);
	std::vector<CGObjectInstance*> result;
	result.reserve(artifactsToReplace.size());
	for (const auto& candidate : artifactsToReplace)
	{
		result.push_back(candidate.object);
	}
	return result;
}

CGObjectInstance * QuestArtifactPlacer::drawObjectToReplace(ui32 desiredValue)
{
	RecursiveLock lock(externalAccessMutex);

	if (artifactsToReplace.empty())
	{
		return nullptr;
	}

	auto bestIt = artifactsToReplace.end();
	ui32 bestDiff = std::numeric_limits<ui32>::max();
	for (auto it = artifactsToReplace.begin(); it != artifactsToReplace.end(); ++it)
	{
		const ui32 value = it->value;
		auto diff = std::abs(static_cast<int>(value) - static_cast<int>(desiredValue));
		if (diff < bestDiff)
		{
			bestDiff = diff;
			bestIt = it;
			if (diff == 0)
				break;
		}
	}

	if (bestIt == artifactsToReplace.end())
	{
		return nullptr;
	}

	auto* ret = bestIt->object;
	artifactsToReplace.erase(bestIt);
	return ret;
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
	for (const auto & questRequest : questArtifactsToPlace)
	{
		RandomGeneratorUtil::randomShuffle(questArtZones, rand);
		for (auto zone : questArtZones)
		{
			auto* qap = zone->getModificator<QuestArtifactPlacer>();
			
			auto objectToReplace = qap->drawObjectToReplace(questRequest.desiredValue);
			if (!objectToReplace)
				continue;

			logGlobal->trace("Replacing %s at %s with the quest artifact %s (desired value %u)",
				objectToReplace->getObjectName(),
				objectToReplace->anchorPos().toString(),
				LIBRARY->artifacts()->getById(questRequest.id)->getNameTranslated(),
				questRequest.desiredValue);

			//Update appearance. Terrain is irrelevant.
			auto handler = LIBRARY->objtypeh->getHandlerFor(Obj::ARTIFACT, questRequest.id);
			auto newObj = handler->create(map.mapInstance->cb, nullptr);
			auto templates = handler->getTemplates();
			newObj->appearance  = templates.front();
			newObj->setAnchorPos(objectToReplace->anchorPos());
			mapProxy->insertObject(newObj);
			mapProxy->removeObject(objectToReplace);
			break;
		}
	}
}

// TODO: Unused?
void QuestArtifactPlacer::dropReplacedArtifact(const CGObjectInstance* obj)
{
	RecursiveLock lock(externalAccessMutex);
	vstd::erase_if(artifactsToReplace, [obj](const ReplacementCandidate& candidate)
	{
		return candidate.object == obj;
	});
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
