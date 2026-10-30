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
#include "../mapObjects/army/CStackBasicDescriptor.h"

class MetaString;
class IGameInfoCallback;
class IGameRandomizer;
class TextIdentifier;
class LibraryLoadContext;

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

	void configureReward(Rewardable::Configuration & object, IGameRandomizer & gameRandomizer, IGameInfoCallback * cb, Rewardable::Reward & info, const JsonNode & source, const TextIdentifier & textIdentifier) const;
	void configureResetInfo(Rewardable::Configuration & object, IGameRandomizer & gameRandomizer, Rewardable::ResetInfo & info, const JsonNode & source) const;
public:
	using CreatureReward = std::vector<CStackBasicDescriptor>;
	using CreatureRewardVariants = std::vector<CreatureReward>;

	const JsonNode & getParameters() const;

	bool givesResources() const override;

	bool givesExperience() const override;
	bool givesMana() const override;
	bool givesMovement() const override;

	bool givesPrimarySkills() const override;
	bool givesSecondarySkills() const override;

	bool givesArtifacts() const override;
	bool givesCreatures() const override;
	bool givesOnlyCreatures() const override;

	/// Empty outer vector means possibilities cannot be determined.
	/// Empty inner vector means this particular reward tier gives no creatures.
	/// Non-empty inner vector contains creature stacks granted by that tier.
	CreatureRewardVariants getPossibleCreatureRewards() const;
	bool hasOnlyBuiltinCreatureRewards() const;
	bool hasOnlyBuiltinRewards() const;

	bool givesSpells() const override;

	bool givesBonuses() const override;

	bool hasGuards() const override;

	/// Returns the minimum and maximum total guard strength across all configured reward tiers.
	/// Returns std::nullopt when there are no guards or their range cannot be evaluated statically.
	std::optional<std::pair<uint64_t, uint64_t>> getCreatureBankGuardRange() const;

	void configureObject(Rewardable::Configuration & object, IGameRandomizer & gameRandomizer, IGameInfoCallback * cb) const;

	void init(LibraryLoadContext & context, const JsonNode & objectConfig, const std::string & objectTextID);

	template <typename Handler> void serialize(Handler &h)
	{
		h & parameters;
	}
};

}
