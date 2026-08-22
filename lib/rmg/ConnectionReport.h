/*
 * ConnectionReport.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../GameConstants.h"

namespace rmg
{
class ZoneConnection;

/// Accumulates how every inter-zone connection was finally realized, so RMG can report
/// how many links degraded to teleporters instead of direct passages, and why.
/// Thread-safe: modificators fill it concurrently, dump() is called once afterwards.
class DLL_LINKAGE ConnectionReport
{
public:
	enum class Resolution
	{
		DIRECT,            // guarded direct passage - the desired outcome
		WIDE,              // wide open shared border
		WATER,             // routed across the water zone
		SUBTERRANEAN_GATE, // cross-level gate - acceptable, reported separately
		MONOLITH,          // teleporter fallback - the outcome we want to minimize
		PORTAL_INTENDED,   // template asked for a portal (FORCE_PORTAL or self-connection)
		VIRTUAL            // fictive/repulsive - no passage was ever needed
	};

	/// Why a direct land passage could not be built (only meaningful for MONOLITH).
	enum class DirectFailure
	{
		NONE,
		NOT_ADJACENT,       // zones share no usable border
		TERRAIN_PROHIBITED, // terrain transition banned between the two zones
		NO_VALID_GUARD      // border exists, but no tile could host a guarded passage
	};

	/// Where the initial placement grid put the two zones relative to each other.
	/// This is a snapshot of discrete placement, so it separates a placement cause
	/// (zones never put adjacent) from a tiling cause (adjacent cells, torn apart later).
	enum class GridRelation
	{
		UNKNOWN,          // one of the zones has no grid cell (e.g. water)
		ORTHOGONAL,       // side-by-side cells - placement intended a shared edge
		DIAGONAL,         // corner-touching cells - direct connection rarely possible
		DISTANT,          // more than one cell apart - placement never abutted them
		DIFFERENT_LEVEL   // cells on different levels
	};

	/// Why a cross-level connection could not become a subterranean gate and fell back to a monolith.
	enum class GateFailure
	{
		NONE,
		SAME_UNDERGROUND_STATE, // zones on different levels but both surface or both underground - gate never applies
		NO_AREA_OVERLAP,        // zone footprints never overlap in XY, so no tile could host a gate on both levels
		OVERLAP_CONSUMED,       // footprints do overlap, but the overlap was used up before the gate step
		GATE_PLACEMENT_FAILED   // overlap existed, but no valid guarded gate + path could be placed
	};

	void noteDirectFailure(const ZoneConnection & connection, DirectFailure reason, int sharedBorderTiles, GridRelation gridRelation);
	void noteGateFailure(const ZoneConnection & connection, GateFailure reason);
	void noteResolution(const ZoneConnection & connection, Resolution resolution);

	static const char * describeGateFailure(GateFailure reason);

	/// Aggregate counts per resolution, for programmatic reporting (e.g. benchmarks).
	struct Totals
	{
		int total = 0;
		int direct = 0;
		int wide = 0;
		int water = 0;
		int subterraneanGates = 0;
		int monoliths = 0;
		int crossLevelMonoliths = 0; // subset of monoliths caused by a failed subterranean gate
		// Same-level monolith cause breakdown (monoliths with no gate failure). These partition the
		// non-cross-level monoliths and tell us which pipeline stage is at fault.
		int naDistant = 0;    // NOT_ADJACENT, grid cells were never abutted (placement)
		int naOrthogonal = 0; // NOT_ADJACENT, grid neighbours torn apart by relaxation/tiling
		int naDiagonal = 0;   // NOT_ADJACENT, only corner contact on the grid
		int naOther = 0;      // NOT_ADJACENT with unknown grid relation, or no direct test recorded
		int noGuard = 0;      // NO_VALID_GUARD, border exists but no tile could host a guarded passage
		int terrain = 0;      // TERRAIN_PROHIBITED
	};
	Totals totals() const;

	/// Logs the accumulated summary plus one line per degraded connection.
	void dump() const;

private:
	struct Record
	{
		TRmgTemplateZoneId zoneA = 0;
		TRmgTemplateZoneId zoneB = 0;
		Resolution resolution = Resolution::VIRTUAL;
		DirectFailure failure = DirectFailure::NONE;
		GridRelation gridRelation = GridRelation::UNKNOWN;
		GateFailure gateFailure = GateFailure::NONE;
		int sharedBorderTiles = 0;
		bool resolved = false;
	};

	mutable std::mutex mutex;
	std::map<int, Record> records; // keyed by ZoneConnection::getId()
};
}
