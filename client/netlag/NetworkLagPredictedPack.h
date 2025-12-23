/*
 * NetworkLagPredictedPack.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/networkPacks/NetPacksBase.h"
#include "../../lib/serializer/CSerializer.h"

/// Class that contains data for a single pack that client expects to receive from server
class NetworkLagPredictedPack final : public IBinaryWriter
{
public:
	/// Serialized data of predicted pack, for comparison with version from server
	std::vector<std::byte> predictedPackData;

	/// List of packs that must be applied on client in case of failed prediction
	std::vector<std::unique_ptr<CPackForClient>> rollbackPacks;

	int write(const std::byte * data, unsigned size) final
	{
		predictedPackData.insert(predictedPackData.end(), data, data + size);
		return size;
	}
};
