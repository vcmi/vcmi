/*
 * ZonePlacementLog.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "ZonePlacementLog.h"

#include "CRmgTemplate.h"
#include "Zone.h"
#include "../logging/CLogger.h"

namespace
{
const char * zoneTypeName(ETemplateZoneType type)
{
	switch(type)
	{
		case ETemplateZoneType::PLAYER_START:
			return "playerStart";
		case ETemplateZoneType::CPU_START:
			return "cpuStart";
		case ETemplateZoneType::TREASURE:
			return "treasure";
		case ETemplateZoneType::JUNCTION:
			return "junction";
		case ETemplateZoneType::WATER:
			return "water";
		case ETemplateZoneType::SEALED:
			return "sealed";
	}
	return "?";
}

const char * connectionTypeName(rmg::EConnectionType type)
{
	switch(type)
	{
		case rmg::EConnectionType::GUARDED:
			return "guarded";
		case rmg::EConnectionType::FICTIVE:
			return "fictive";
		case rmg::EConnectionType::REPULSIVE:
			return "repulsive";
		case rmg::EConnectionType::WIDE:
			return "wide";
		case rmg::EConnectionType::FORCE_PORTAL:
			return "forcePortal";
	}
	return "?";
}

const char * outcomeName(ConnectionOutcome outcome)
{
	switch(outcome)
	{
		case ConnectionOutcome::Direct:
			return "direct";
		case ConnectionOutcome::Gate:
			return "gate";
		case ConnectionOutcome::Monolith:
			return "monolith";
	}
	return "?";
}

std::string formatCenter(const float3 & center)
{
	return boost::str(boost::format("(%.4f %.4f %d)") % center.x % center.y % center.z);
}

std::string ownerLabel(const std::shared_ptr<Zone> & zone)
{
	if(auto owner = zone->getOwner())
		return std::to_string(*owner);
	return "-";
}
}

ZonePlacementLog::ZonePlacementLog()
	: logger(CLogger::getLogger(CLoggerDomain("rmg.zonePlacement")))
{
}

void ZonePlacementLog::placementStarted(int zoneCount, int levels, int width, int height, int attempts) const
{
	logger->info("Starting zone placement: %d zones, %d levels, %dx%d, %d attempts", zoneCount, levels, width, height, attempts);
}

void ZonePlacementLog::attemptStarted(int attempt, int attempts) const
{
	currentAttempt = attempt;
	attemptCount = attempts;
	iteration = 0;
}

void ZonePlacementLog::levelSplitAndPrescale(const TZoneMap & zones, const std::map<int, float> & prescaler) const
{
	if(!logger->isDebugEnabled())
		return;

	for(const auto & [level, scale] : prescaler)
		logger->debug("Level %d prescaler %.4f", level, scale);

	for(const auto & zone : zones)
		logger->debug("Zone %d -> level %d", zone.first, static_cast<int>(zone.second->getCenter().z));
}

void ZonePlacementLog::initialGridLayout(const TZoneMap & zones) const
{
	if(!logger->isDebugEnabled())
		return;
	logger->debug("Initial grid layout");
	logLayoutTable(true, zones, nullptr);
}

void ZonePlacementLog::relaxationIteration(float stiffness, const TDistanceVector & distances, const TDistanceVector & overlaps, bool improved) const
{
	if(!logger->isTraceEnabled())
		return;

	++iteration;
	float totalDistance = 0;
	float totalOverlap = 0;
	for(const auto & zone : distances)
	{
		totalDistance += zone.second;
		totalOverlap += overlaps.at(zone.first);
	}

	logger->trace("Relaxation attempt %d/%d iteration %d stiffness %.6f distance %.4f overlap %.4f improved %s",
		currentAttempt, attemptCount, iteration, stiffness, totalDistance, totalOverlap, improved ? "yes" : "no");
}

void ZonePlacementLog::misplacedRatio(float worstRatio) const
{
	if(!logger->isTraceEnabled())
		return;
	logger->trace("Worst misplacement/movement ratio: %3.2f", worstRatio);
}

void ZonePlacementLog::zonesSwapped(TRmgTemplateZoneId firstId, const float3 & firstOldCenter, TRmgTemplateZoneId secondId, const float3 & secondOldCenter) const
{
	if(!logger->isTraceEnabled())
		return;
	logger->trace("Swapping misplaced zones %d %s and %d %s", firstId, formatCenter(firstOldCenter), secondId, formatCenter(secondOldCenter));
}

void ZonePlacementLog::zoneMoved(TRmgTemplateZoneId zoneId, TRmgTemplateZoneId targetId, const float3 & direction, const float3 & oldCenter, const float3 & newCenter, bool toward) const
{
	if(!logger->isTraceEnabled())
		return;
	logger->trace("Move zone %d %s %s %d, direction %s, %s -> %s",
		zoneId,
		formatCenter(oldCenter),
		toward ? "towards" : "away from",
		targetId,
		formatCenter(direction),
		formatCenter(oldCenter),
		formatCenter(newCenter));
}

void ZonePlacementLog::attemptFinished(int attempt, int attempts, int score, const ConnectivityCounts & counts, float fitness, float totalDistance, float totalOverlap, float crowding) const
{
	logger->info("Zone placement attempt %d/%d: score %d (direct %d, gates %d, monoliths %d); fitness %2.4f (distance %2.4f overlap %2.4f crowding %2.4f)",
		attempt, attempts, score, counts.direct, counts.gates, counts.monoliths, fitness, totalDistance, totalOverlap, crowding);
}

void ZonePlacementLog::playerZonesArranged(const std::vector<TRmgTemplateZoneId> & group, const std::vector<std::pair<TRmgTemplateZoneId, std::pair<float3, float3>>> & moves) const
{
	if(!logger->isDebugEnabled())
		return;

	std::string ids;
	for(size_t i = 0; i < group.size(); ++i)
	{
		if(i)
			ids += " ";
		ids += std::to_string(group[i]);
	}
	logger->debug("Arranged interchangeable player zones: %s", ids);
	for(const auto & move : moves)
		logger->debug("  zone %d %s -> %s", move.first, formatCenter(move.second.first), formatCenter(move.second.second));
}

void ZonePlacementLog::attemptBestLayout(const TZoneMap & zones, const TZoneCenterMap & solution) const
{
	if(!logger->isDebugEnabled())
		return;
	logger->debug("Attempt %d best layout", currentAttempt);
	logLayoutTable(true, zones, &solution);
}

void ZonePlacementLog::winner(int attempt, int attempts, int score, const TZoneMap & zones, const std::vector<ConnectionPrediction> & predictions) const
{
	logger->info("Best zone placement: attempt %d/%d score %d", attempt, attempts, score);
	logger->info("Winning layout: id type owner level size center");
	logLayoutTable(false, zones, nullptr);

	for(const auto & prediction : predictions)
	{
		logger->info("Connection %d zones %d-%d %s: predicted %s, distance %.4f vs reach %.4f",
			prediction.id,
			prediction.zoneA,
			prediction.zoneB,
			connectionTypeName(prediction.type),
			outcomeName(prediction.predicted),
			prediction.distance,
			prediction.reach);
	}
}

void ZonePlacementLog::orientation(int rotation, bool flipH, bool flipV) const
{
	if(!logger->isDebugEnabled())
		return;
	logger->debug("Orientation rotation %d flipH %s flipV %s", rotation, flipH ? "true" : "false", flipV ? "true" : "false");
}

void ZonePlacementLog::tileAssignment(int level, double bestBalanceError, const std::vector<std::shared_ptr<Zone>> & levelZones, const std::vector<double> & targetShare, const std::vector<double> & tilesWon, const std::vector<int> & vertexCount) const
{
	if(!logger->isDebugEnabled())
		return;

	logger->debug("Level %d tile assignment, best balance error %.6f", level, bestBalanceError);
	for(size_t i = 0; i < levelZones.size(); ++i)
	{
		logger->debug("Zone %d: target share %.4f, tiles %d, Penrose vertices %d",
			levelZones[i]->getId(),
			targetShare[i],
			static_cast<int>(tilesWon[i]),
			vertexCount[i]);
	}
}

void ZonePlacementLog::colouringFinished(const TZoneMap & zones) const
{
	logger->info("Finished zone colouring");
	for(const auto & zone : zones)
	{
		logger->info("Zone %d: %d tiles, pos %s",
			zone.first,
			static_cast<int>(zone.second->area()->getTiles().size()),
			zone.second->getPos().toString());
	}
}

void ZonePlacementLog::nonParticipatingPlayer(int player, TRmgTemplateZoneId zoneId) const
{
	logger->trace("Player %d (starting zone %d) does not participate in game", player, zoneId);
}

void ZonePlacementLog::gridPlacementFailed(TRmgTemplateZoneId zoneId, int level, size_t gridSize) const
{
	logger->warn("Could not find place for zone %d on level %d grid size %d", zoneId, level, gridSize);
}

void ZonePlacementLog::gridCellChosen(TRmgTemplateZoneId zoneId, int level, const int3 & cell, double score) const
{
	if(!logger->isTraceEnabled())
		return;
	logger->trace("placeOnGrid: zone %d level %d grid %s score %.6g", zoneId, level, cell.toString(), score);
}

void ZonePlacementLog::logLayoutTable(bool debug, const TZoneMap & zones, const TZoneCenterMap * centersOverride) const
{
	if(debug && !logger->isDebugEnabled())
		return;

	const auto level = debug ? ELogLevel::DEBUG : ELogLevel::INFO;
	for(const auto & zone : zones)
	{
		float3 center = zone.second->getCenter();
		if(centersOverride)
		{
			auto it = centersOverride->find(zone.second);
			if(it != centersOverride->end())
				center = it->second;
		}

		logger->log(level, "%d %s %s %d %d %s",
			zone.first,
			zoneTypeName(zone.second->getType()),
			ownerLabel(zone.second),
			static_cast<int>(center.z),
			zone.second->getSize(),
			formatCenter(center));
	}
}
