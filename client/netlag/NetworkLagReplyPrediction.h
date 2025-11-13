/*
 * NetworkLagReplyPrediction.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "NetworkLagPredictedPack.h"

#include "../../lib/constants/EntityIdentifiers.h"

/// Class that contains data for a single pack that client sent to server
class NetworkLagReplyPrediction
{
public:
	/// Sender that sent this reply. Generally - this should be only local client color
	PlayerColor senderID;

	/// Unique request ID that was written into sent pack
	uint32_t requestID;

	/// One or more packs that client expects to receive from server as a reply
	std::vector<NetworkLagPredictedPack> writtenPacks;
};
