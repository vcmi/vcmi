/*
 * Info.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../json/JsonNode.h"
#include "../mapObjectConstructors/IObjectInfo.h"

VCMI_LIB_NAMESPACE_BEGIN

class MetaString;
class IGameInfoCallback;
class IGameRandomizer;

namespace Rewardable
{
struct Limiter;
using LimitersList = std::vector<std::shared_ptr<Rewardable::Limiter>>;
struct Reward;
struct Configuration;
struct Variables;
struct VisitInfo;
struct ResetInfo;
enum class EEventType : uint8_t;

class DLL_LINKAGE Info : public IObjectInfo
{
	JsonNode parameters;
	std::string objectTextID;

	void replaceTextPlaceholders(MetaString & target, const Variables & variables) const;
	void replaceTextPlaceholders(MetaString & target, const Variables & variables, const VisitInfo & info) const;

	void configureVariables(Rewardable::Configuration & object, IGameRandomizer & gameRandomizer, IGameInfoCallback * cb, const JsonNode & source) const;
	void configureRewards(Rewardable::Configuration & object, IGameRandomizer & gameRandomizer, IGameInfoCallback * cb, const JsonNode & source, Rewardable::EEventType mode, const std::string & textPrefix) const;

	void configureLimiter(Rewardable::Configuration & object, IGameRandomizer & gameRandomizer, IGameInfoCallback * cb, Rewardable::Limiter & limiter, const JsonNode & source) const;
	Rewardable::LimitersList configureSublimiters(Rewardable::Configuration & object, IGameRandomizer & gameRandomizer, IGameInfoCallback * cb, const JsonNode & source) const;

	void configureReward(Rewardable::Configuration & object, IGameRandomizer & gameRandomizer, IGameInfoCallback * cb, Rewardable::Reward & info, const JsonNode & source) const;
	void configureResetInfo(Rewardable::Configuration & object, IGameRandomizer & gameRandomizer, Rewardable::ResetInfo & info, const JsonNode & source) const;
public:
	const JsonNode & getParameters() const;

	bool givesResources() const override;

	bool givesExperience() const override;
	bool givesMana() const override;
	bool givesMovement() const override;

	bool givesPrimarySkills() const override;
	bool givesSecondarySkills() const override;

	bool givesArtifacts() const override;
	bool givesCreatures() const override;
	bool givesSpells() const override;

	bool givesBonuses() const override;

	bool hasGuards() const override;

	void configureObject(Rewardable::Configuration & object, IGameRandomizer & gameRandomizer, IGameInfoCallback * cb) const;

	void init(const JsonNode & objectConfig, const std::string & objectTextID);

	template <typename Handler> void serialize(Handler &h)
	{
		h & parameters;
	}
};

}

VCMI_LIB_NAMESPACE_END
