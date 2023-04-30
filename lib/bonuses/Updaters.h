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

#include "HeroBonus.h"

VCMI_LIB_NAMESPACE_BEGIN

extern DLL_LINKAGE const std::map<std::string, TUpdaterPtr> bonusUpdaterMap;

// observers for updating bonuses based on certain events (e.g. hero gaining level)

class DLL_LINKAGE IUpdater
{
public:
	virtual ~IUpdater() = default;

	virtual std::shared_ptr<Bonus> createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context) const;
	virtual std::string toString() const;
	virtual JsonNode toJsonNode() const;

	template <typename Handler> void serialize(Handler & h, const int version)
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

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<IUpdater &>(*this);
		h & valPer20;
		h & stepSize;
	}

	std::shared_ptr<Bonus> createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context) const override;
	virtual std::string toString() const override;
	virtual JsonNode toJsonNode() const override;
};

class DLL_LINKAGE TimesHeroLevelUpdater : public IUpdater
{
public:
	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<IUpdater &>(*this);
	}

	std::shared_ptr<Bonus> createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context) const override;
	virtual std::string toString() const override;
	virtual JsonNode toJsonNode() const override;
};

class DLL_LINKAGE TimesStackLevelUpdater : public IUpdater
{
public:
	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<IUpdater &>(*this);
	}

	std::shared_ptr<Bonus> createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context) const override;
	virtual std::string toString() const override;
	virtual JsonNode toJsonNode() const override;
};

class DLL_LINKAGE ArmyMovementUpdater : public IUpdater
{
public:
	si32 base;
	si32 divider;
	si32 multiplier;
	si32 max;
	ArmyMovementUpdater();
	ArmyMovementUpdater(int base, int divider, int multiplier, int max);
	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<IUpdater &>(*this);
		h & base;
		h & divider;
		h & multiplier;
		h & max;
	}

	std::shared_ptr<Bonus> createUpdatedBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & context) const override;
	virtual std::string toString() const override;
	virtual JsonNode toJsonNode() const override;
};

class DLL_LINKAGE OwnerUpdater : public IUpdater
{
public:
	template <typename Handler> void serialize(Handler& h, const int version)
	{
		h & static_cast<IUpdater &>(*this);
	}

	std::shared_ptr<Bonus> createUpdatedBonus(const std::shared_ptr<Bonus>& b, const CBonusSystemNode& context) const override;
	virtual std::string toString() const override;
	virtual JsonNode toJsonNode() const override;
};

VCMI_LIB_NAMESPACE_END