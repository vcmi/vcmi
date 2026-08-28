/*
 * ZonePlacementConfig.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

/// Zone placement tuning, loaded from randomMap.json
struct ZonePlacementConfig
{
	int attempts = 0;         // number of layouts to relax, best connection score wins
	int scoreDirect = 0;      // weight of a connection predicted to become a direct passage
	int scoreGate = 0;        // weight of a connection predicted to become a subterranean gate
	int scoreMonolith = 0;    // weight of a connection predicted to fall back to a monolith
	int maxGateDistance = 0;  // max distance between the two ends of a subterranean gate (0 = gates must share a column)
	float playerRepulsion = 0.f; // strength of the preference for keeping player starts apart (0 = off)
};
