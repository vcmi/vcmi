/*
 * Updaters.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "Bonus.h"
#include "../serializer/Serializeable.h"

VCMI_LIB_NAMESPACE_BEGIN

class AggregateLimiter;
class CCreatureTypeLimiter;
class HasAnotherBonusLimiter;
class CreatureTerrainLimiter;
class CreatureLevelLimiter;
class FactionLimiter;
class CreatureAlignmentLimiter;
class OppositeSideLimiter;
class RankRangeLimiter;
class UnitOnHexLimiter;

// observers for updating bonuses based on certain events (e.g. hero gaining level)

class DLL_LINKAGE IUpdater : public Serializeable
{
public:
	virtual ~IUpdater() = default;

	virtual std::shared_ptr<Bonus> createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context);
	virtual std::string toString() const;
	virtual JsonNode toJsonNode() const;

	virtual void visitLimiter(AggregateLimiter & limiter);
	virtual void visitLimiter(CCreatureTypeLimiter& limiter);
	virtual void visitLimiter(HasAnotherBonusLimiter & limiter);
	virtual void visitLimiter(CreatureTerrainLimiter & limiter);
	virtual void visitLimiter(CreatureLevelLimiter & limiter);
	virtual void visitLimiter(FactionLimiter & limiter);
	virtual void visitLimiter(CreatureAlignmentLimiter & limiter) ;
	virtual void visitLimiter(OppositeSideLimiter & limiter);
	virtual void visitLimiter(RankRangeLimiter & limiter);
	virtual void visitLimiter(UnitOnHexLimiter & limiter);

	template <typename Handler> void serialize(Handler & h)
	{
	}
};

class DLL_LINKAGE GrowsWithLevelUpdater : public IUpdater
{
public:
	int valPer20 = 0;
	int stepSize = 1;

	GrowsWithLevelUpdater() = default;
	GrowsWithLevelUpdater(int valPer20, int stepSize = 1);

	template <typename Handler> void serialize(Handler & h)
	{
		h & static_cast<IUpdater &>(*this);
		h & valPer20;
		h & stepSize;
	}

	std::shared_ptr<Bonus> createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context) override;
	std::string toString() const override;
	JsonNode toJsonNode() const override;
};

class DLL_LINKAGE TimesHeroLevelUpdater : public IUpdater
{
public:
	template <typename Handler> void serialize(Handler & h)
	{
		h & static_cast<IUpdater &>(*this);
	}

	std::shared_ptr<Bonus> createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context) override;
	std::string toString() const override;
	JsonNode toJsonNode() const override;
};

class DLL_LINKAGE TimesStackLevelUpdater : public IUpdater
{
	std::shared_ptr<Bonus> apply(const std::shared_ptr<Bonus> & b, int level) const;
public:
	template <typename Handler> void serialize(Handler & h)
	{
		h & static_cast<IUpdater &>(*this);
	}

	std::shared_ptr<Bonus> createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context) override;
	std::string toString() const override;
	JsonNode toJsonNode() const override;
};

class DLL_LINKAGE DivideStackLevelUpdater : public IUpdater
{
	std::shared_ptr<Bonus> apply(const std::shared_ptr<Bonus> & b, int level) const;
public:
	template <typename Handler> void serialize(Handler & h)
	{
		h & static_cast<IUpdater &>(*this);
	}

	std::shared_ptr<Bonus> createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context) override;
	std::string toString() const override;
	JsonNode toJsonNode() const override;
};

class DLL_LINKAGE TimesHeroLevelDivideStackLevelUpdater : public TimesHeroLevelUpdater
{
	std::shared_ptr<DivideStackLevelUpdater> divideStackLevel;
public:
	template <typename Handler> void serialize(Handler & h)
	{
		h & static_cast<TimesHeroLevelUpdater &>(*this);
		h & divideStackLevel;
	}

	TimesHeroLevelDivideStackLevelUpdater()
		: divideStackLevel(std::make_shared<DivideStackLevelUpdater>())
	{}

	std::shared_ptr<Bonus> createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context) override;
	std::string toString() const override;
	JsonNode toJsonNode() const override;
};

class DLL_LINKAGE OwnerUpdater : public IUpdater
{
protected:
	PlayerColor owner;

public:
	template <typename Handler> void serialize(Handler& h)
	{
		h & static_cast<IUpdater &>(*this);
	}

	std::shared_ptr<Bonus> createUpdatedBonus(const std::shared_ptr<Bonus>& b, const CBonusSystemNode& context) override;
	std::string toString() const override;
	JsonNode toJsonNode() const override;
	void visitLimiter(OppositeSideLimiter& limiter) override;
};

VCMI_LIB_NAMESPACE_END
