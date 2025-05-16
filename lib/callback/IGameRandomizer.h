/*
 * IGameRandomizer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN

enum class EArtifactClass;

namespace vstd
{
	class RNG;
}

/// Provides source of random rolls for game entities
/// Instance of this interface only exists on server
class IGameRandomizer : boost::noncopyable
{
public:
	virtual ~IGameRandomizer();

	virtual ArtifactID rollArtifact() = 0;
	virtual ArtifactID rollArtifact(EArtifactClass type) = 0;
	virtual ArtifactID rollArtifact(std::set<ArtifactID> filtered) = 0;

	virtual std::vector<ArtifactID> rollMarketArtifactSet() = 0;

	virtual CreatureID rollCreature() = 0;
	virtual CreatureID rollCreature(int tier) = 0;

	virtual HeroTypeID rollHero(PlayerColor player, FactionID faction) = 0;

	virtual std::string rollTownName(FactionID faction) = 0;

	virtual vstd::RNG & getDefault() = 0;
};

VCMI_LIB_NAMESPACE_END
