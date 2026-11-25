/*
 * ZonePlacementLog.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "float3.h"
#include "../GameConstants.h"
#include "../int3.h"

#include <map>
#include <memory>
#include <utility>
#include <vector>

namespace rmg
{
enum class EConnectionType;
}

namespace vstd
{
class CLoggerBase;
}

class Zone;

using TZoneVector = std::vector<std::pair<TRmgTemplateZoneId, std::shared_ptr<Zone>>>;
using TZoneMap = std::map<TRmgTemplateZoneId, std::shared_ptr<Zone>>;
using TForceVector = std::map<std::shared_ptr<Zone>, float3>;
using TZoneCenterMap = std::map<std::shared_ptr<Zone>, float3>;
using TDistanceVector = std::map<std::shared_ptr<Zone>, float>;
using TDistanceMap = std::map<int, std::map<int, size_t>>;

enum class ConnectionOutcome
{
	Direct,
	Gate,
	Monolith
};

/// Predicted realization of one template connection from a finished layout's geometry.
struct ConnectionPrediction
{
	int id = 0;
	TRmgTemplateZoneId zoneA = 0;
	TRmgTemplateZoneId zoneB = 0;
	rmg::EConnectionType type {};
	ConnectionOutcome predicted = ConnectionOutcome::Direct;
	float distance = 0.f;
	float reach = 0.f;
};

struct ConnectivityCounts
{
	int direct = 0;
	int gates = 0;
	int monoliths = 0;
};

inline ConnectivityCounts connectivityCounts(const std::vector<ConnectionPrediction> & predictions)
{
	ConnectivityCounts counts;
	for(const auto & prediction : predictions)
	{
		switch(prediction.predicted)
		{
			case ConnectionOutcome::Direct:
				++counts.direct;
				break;
			case ConnectionOutcome::Gate:
				++counts.gates;
				break;
			case ConnectionOutcome::Monolith:
				++counts.monoliths;
				break;
		}
	}
	return counts;
}

/// Formats zone placement events onto the rmg.zonePlacement logger.
class ZonePlacementLog
{
public:
	ZonePlacementLog();

	void placementStarted(int zoneCount, int levels, int width, int height, int attempts) const;
	void attemptStarted(int attempt, int attempts) const;

	void levelSplitAndPrescale(const TZoneMap & zones, const std::map<int, float> & prescaler) const;
	void initialGridLayout(const TZoneMap & zones) const;
	void relaxationIteration(float stiffness, const TDistanceVector & distances, const TDistanceVector & overlaps, bool improved) const;
	void misplacedRatio(float worstRatio) const;
	void zonesSwapped(TRmgTemplateZoneId firstId, const float3 & firstOldCenter, TRmgTemplateZoneId secondId, const float3 & secondOldCenter) const;
	void zoneMoved(TRmgTemplateZoneId zoneId, TRmgTemplateZoneId targetId, const float3 & direction, const float3 & oldCenter, const float3 & newCenter, bool toward) const;

	void attemptFinished(int attempt, int attempts, int score, const ConnectivityCounts & counts, float fitness, float totalDistance, float totalOverlap) const;
	void attemptBestLayout(const TZoneMap & zones, const TZoneCenterMap & solution) const;
	void winner(int attempt, int attempts, int score, const TZoneMap & zones, const std::vector<ConnectionPrediction> & predictions) const;
	void orientation(int rotation, bool flipH, bool flipV) const;
	void tileAssignment(int level, double bestBalanceError, const std::vector<std::shared_ptr<Zone>> & levelZones, const std::vector<double> & targetShare, const std::vector<double> & tilesWon, const std::vector<int> & vertexCount) const;
	void colouringFinished(const TZoneMap & zones) const;

	void nonParticipatingPlayer(int player, TRmgTemplateZoneId zoneId) const;
	void gridPlacementFailed(TRmgTemplateZoneId zoneId, int level, size_t gridSize) const;
	void gridCellChosen(TRmgTemplateZoneId zoneId, int level, const int3 & cell, double score) const;

private:
	void logLayoutTable(bool debug, const TZoneMap & zones, const TZoneCenterMap * centersOverride) const;

	vstd::CLoggerBase * logger;
	mutable int iteration = 0;
	mutable int currentAttempt = 0;
	mutable int attemptCount = 0;
};
