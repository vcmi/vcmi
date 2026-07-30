/*
 * ConnectionReport.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "ConnectionReport.h"

#include "CRmgTemplate.h"
#include "../logging/CLogger.h"

namespace rmg
{

void ConnectionReport::noteDirectFailure(const ZoneConnection & connection, DirectFailure reason, int sharedBorderTiles, GridRelation gridRelation)
{
	if(reason == DirectFailure::NONE)
		return;

	std::lock_guard lock(mutex);
	auto & record = records[connection.getId()];
	record.zoneA = connection.getZoneA();
	record.zoneB = connection.getZoneB();
	record.failure = reason;
	record.sharedBorderTiles = sharedBorderTiles;
	record.gridRelation = gridRelation;
}

void ConnectionReport::noteGateFailure(const ZoneConnection & connection, GateFailure reason)
{
	std::lock_guard lock(mutex);
	auto & record = records[connection.getId()];
	record.zoneA = connection.getZoneA();
	record.zoneB = connection.getZoneB();
	record.gateFailure = reason;
}

const char * ConnectionReport::describeGateFailure(GateFailure reason)
{
	switch(reason)
	{
		case GateFailure::SAME_UNDERGROUND_STATE:
			return "zones on different levels but both surface or both underground - gate not applicable";
		case GateFailure::NO_AREA_OVERLAP:
			return "zone footprints never overlap in XY - placement/tiling never aligned them vertically";
		case GateFailure::OVERLAP_CONSUMED:
			return "footprints overlap in XY, but the overlap was used up before the gate step";
		case GateFailure::GATE_PLACEMENT_FAILED:
			return "footprints overlap, but no valid guarded gate + connecting path could be placed";
		default:
			return "unknown gate failure";
	}
}

void ConnectionReport::noteResolution(const ZoneConnection & connection, Resolution resolution)
{
	std::lock_guard lock(mutex);
	auto & record = records[connection.getId()];
	record.zoneA = connection.getZoneA();
	record.zoneB = connection.getZoneB();
	record.resolution = resolution;
	record.resolved = true;
}

ConnectionReport::Totals ConnectionReport::totals() const
{
	std::lock_guard lock(mutex);
	Totals t;
	t.total = static_cast<int>(records.size());
	for(const auto & [id, record] : records)
	{
		switch(record.resolution)
		{
			case Resolution::DIRECT: t.direct++; break;
			case Resolution::WIDE: t.wide++; break;
			case Resolution::WATER: t.water++; break;
			case Resolution::SUBTERRANEAN_GATE: t.subterraneanGates++; break;
			case Resolution::MONOLITH:
				t.monoliths++;
				if(record.gateFailure != GateFailure::NONE)
				{
					t.crossLevelMonoliths++;
					break;
				}
				//same-level monolith: attribute it to a pipeline stage
				switch(record.failure)
				{
					case DirectFailure::TERRAIN_PROHIBITED: t.terrain++; break;
					case DirectFailure::NO_VALID_GUARD: t.noGuard++; break;
					case DirectFailure::NOT_ADJACENT:
						switch(record.gridRelation)
						{
							case GridRelation::ORTHOGONAL: t.naOrthogonal++; break;
							case GridRelation::DIAGONAL: t.naDiagonal++; break;
							case GridRelation::DISTANT: t.naDistant++; break;
							default: t.naOther++; break;
						}
						break;
					default: t.naOther++; break;
				}
				break;
			default: break;
		}
	}
	return t;
}

void ConnectionReport::dump() const
{
	// Turns the recorded failure + placement geometry into a single diagnosis of the monolith
	auto explain = [](const Record & r) -> std::string
	{
		//Cross-level connections degrade because a subterranean gate could not be placed
		if(r.gateFailure != GateFailure::NONE)
			return describeGateFailure(r.gateFailure);

		if(r.failure == DirectFailure::TERRAIN_PROHIBITED)
			return "terrain transition prohibited between the two zones";

		switch(r.gridRelation)
		{
			case GridRelation::DIAGONAL:
				return "zones are diagonal grid neighbours - only corner contact, direct connection rarely possible (initial placement)";
			case GridRelation::DISTANT:
				return "initial grid placement never put zones adjacent (placed far apart)";
			case GridRelation::DIFFERENT_LEVEL:
				return "zones on different levels and no subterranean gate could be placed";
			case GridRelation::ORTHOGONAL:
				if(r.sharedBorderTiles == 0)
					return "placed as adjacent grid neighbours, but relaxation/tiling tore them apart (no shared border)";
				return (boost::format("adjacent (shared border %d tiles) but no tile could host a guarded passage - border too narrow or blocked by a third zone") % r.sharedBorderTiles).str();
			default: // UNKNOWN grid relation - fall back to the raw border measurement
				if(r.sharedBorderTiles == 0)
					return "zones share no border (cause could not be attributed to a placement stage)";
				return (boost::format("shared border %d tiles but no valid guard tile") % r.sharedBorderTiles).str();
		}
	};

	std::lock_guard lock(mutex);

	std::map<Resolution, int> totals;
	std::map<GateFailure, int> gateTotals;
	for(const auto & [id, record] : records)
	{
		totals[record.resolution]++;
		if(record.resolution == Resolution::MONOLITH && record.gateFailure != GateFailure::NONE)
			gateTotals[record.gateFailure]++;
	}

	logGlobal->info("=== RMG connection report ===");
	logGlobal->info("Total connections: %d", static_cast<int>(records.size()));
	logGlobal->info("  direct passages:     %d", totals[Resolution::DIRECT]);
	logGlobal->info("  wide passages:       %d", totals[Resolution::WIDE]);
	logGlobal->info("  water routes:        %d", totals[Resolution::WATER]);
	logGlobal->info("  subterranean gates:  %d", totals[Resolution::SUBTERRANEAN_GATE]);
	logGlobal->info("  monoliths (bad):     %d", totals[Resolution::MONOLITH]);
	logGlobal->info("  intended portals:    %d", totals[Resolution::PORTAL_INTENDED]);
	logGlobal->info("  virtual (no passage):%d", totals[Resolution::VIRTUAL]);

	int crossLevelMonoliths = 0;
	for(const auto & [reason, count] : gateTotals)
		crossLevelMonoliths += count;
	if(crossLevelMonoliths > 0)
	{
		logGlobal->info("Of those monoliths, %d are cross-level (gate could not be placed):", crossLevelMonoliths);
		logGlobal->info("  gate not applicable (same level state): %d", gateTotals[GateFailure::SAME_UNDERGROUND_STATE]);
		logGlobal->info("  footprints never overlapped in XY:      %d", gateTotals[GateFailure::NO_AREA_OVERLAP]);
		logGlobal->info("  overlap consumed before gate step:      %d", gateTotals[GateFailure::OVERLAP_CONSUMED]);
		logGlobal->info("  gate/path placement failed:             %d", gateTotals[GateFailure::GATE_PLACEMENT_FAILED]);
	}

	// Per-connection detail for degraded links only - these are what we want to reduce
	for(const auto & [id, record] : records)
	{
		if(record.resolution == Resolution::MONOLITH)
			logGlobal->info("Monolith between zones %d and %d: %s", record.zoneA, record.zoneB, explain(record));
	}

	// Unresolved records mean a connection was never classified - a wiring bug worth surfacing
	for(const auto & [id, record] : records)
	{
		if(!record.resolved)
			logGlobal->warn("Connection %d (zones %d-%d) was never resolved", id, record.zoneA, record.zoneB);
	}
}

}
