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

class CGHeroInstance;

enum class EArtifactClass;

namespace vstd
{
class RNG;
}

/// Provides source of random rolls for game entities
/// Instance of this interface only exists on server
class DLL_LINKAGE IGameRandomizer : boost::noncopyable
{
public:
	virtual ~IGameRandomizer() = default;

	virtual ArtifactID rollArtifact() = 0;
	virtual ArtifactID rollArtifact(EArtifactClass type) = 0;
	virtual ArtifactID rollArtifact(std::set<ArtifactID> filtered) = 0;

	virtual std::vector<ArtifactID> rollMarketArtifactSet() = 0;

	virtual CreatureID rollCreature() = 0;
	virtual CreatureID rollCreature(int tier) = 0;

	virtual PrimarySkill rollPrimarySkillForLevelup(const CGHeroInstance * hero) = 0;
	virtual SecondarySkill rollSecondarySkillForLevelup(const CGHeroInstance * hero, const std::set<SecondarySkill> & candidates) = 0;

	virtual vstd::RNG & getDefault() = 0;
};

VCMI_LIB_NAMESPACE_END
