/*
 * Limiters.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "Bonus.h"

#include "../battle/BattleHexArray.h"
#include "../serializer/Serializeable.h"
#include "../constants/Enumerations.h"

VCMI_LIB_NAMESPACE_BEGIN

class CCreature;

extern DLL_LINKAGE const std::map<std::string, TLimiterPtr> bonusLimiterMap;

struct BonusLimitationContext
{
	const Bonus & b;
	const CBonusSystemNode & node;
	const BonusList & alreadyAccepted;
	const BonusList & stillUndecided;
};

class DLL_LINKAGE ILimiter : public Serializeable
{
public:
	enum class EDecision : uint8_t {ACCEPT, DISCARD, NOT_SURE};

	virtual ~ILimiter() = default;

	virtual EDecision limit(const BonusLimitationContext &context) const; //0 - accept bonus; 1 - drop bonus; 2 - delay (drops eventually)
	virtual std::string toString() const;
	virtual JsonNode toJsonNode() const;
	virtual void acceptUpdater(IUpdater & visitor);

	template <typename Handler> void serialize(Handler &h)
	{
	}
};

class DLL_LINKAGE AggregateLimiter : public ILimiter
{
protected:
	virtual const std::string & getAggregator() const = 0;
	AggregateLimiter(std::vector<TLimiterPtr> limiters = {});
public:
	std::vector<TLimiterPtr> limiters;
	void add(const TLimiterPtr & limiter);
	JsonNode toJsonNode() const override;
	void acceptUpdater(IUpdater & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & static_cast<ILimiter&>(*this);
		h & limiters;
	}
};

class DLL_LINKAGE AllOfLimiter : public AggregateLimiter
{
protected:
	const std::string & getAggregator() const override;
public:
	AllOfLimiter(std::vector<TLimiterPtr> limiters = {});
	static const std::string aggregator;
	EDecision limit(const BonusLimitationContext & context) const override;
};

class DLL_LINKAGE AnyOfLimiter : public AggregateLimiter
{
protected:
	const std::string & getAggregator() const override;
public:
	AnyOfLimiter(std::vector<TLimiterPtr> limiters = {});
	static const std::string aggregator;
	EDecision limit(const BonusLimitationContext & context) const override;
};

class DLL_LINKAGE NoneOfLimiter : public AggregateLimiter
{
protected:
	const std::string & getAggregator() const override;
public:
	NoneOfLimiter(std::vector<TLimiterPtr> limiters = {});
	static const std::string aggregator;
	EDecision limit(const BonusLimitationContext & context) const override;
};

class DLL_LINKAGE CCreatureTypeLimiter : public ILimiter //affect only stacks of given creature (and optionally it's upgrades)
{
public:
	CreatureID creatureID;
	bool includeUpgrades = false;

	CCreatureTypeLimiter() = default;
	CCreatureTypeLimiter(const CCreature & creature_, bool IncludeUpgrades);
	void setCreature(const CreatureID & id);

	EDecision limit(const BonusLimitationContext &context) const override;
	std::string toString() const override;
	JsonNode toJsonNode() const override;
	void acceptUpdater(IUpdater & visitor) override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<ILimiter&>(*this);
		h & creatureID;
		h & includeUpgrades;
	}
};

class DLL_LINKAGE HasAnotherBonusLimiter : public ILimiter //applies only to nodes that have another bonus working
{
public:
	BonusType type;
	BonusSubtypeID subtype;
	BonusSource source = BonusSource::OTHER;
	BonusSourceID sid;
	bool isSubtypeRelevant; //check for subtype only if this is true
	bool isSourceRelevant; //check for bonus source only if this is true
	bool isSourceIDRelevant; //check for bonus source only if this is true

	HasAnotherBonusLimiter(BonusType bonus = BonusType::NONE);
	HasAnotherBonusLimiter(BonusType bonus, BonusSubtypeID _subtype);
	HasAnotherBonusLimiter(BonusType bonus, BonusSource src);
	HasAnotherBonusLimiter(BonusType bonus, BonusSubtypeID _subtype, BonusSource src);

	EDecision limit(const BonusLimitationContext &context) const override;
	std::string toString() const override;
	JsonNode toJsonNode() const override;
	void acceptUpdater(IUpdater & visitor) override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<ILimiter&>(*this);
		h & type;
		h & subtype;
		h & isSubtypeRelevant;
		h & source;
		h & isSourceRelevant;
		h & sid;
		h & isSourceIDRelevant;
	}
};

class DLL_LINKAGE CreatureTerrainLimiter : public ILimiter //applies only to creatures that are on specified terrain, default native terrain
{
public:
	TerrainId terrainType;
	CreatureTerrainLimiter();
	CreatureTerrainLimiter(TerrainId terrain);

	EDecision limit(const BonusLimitationContext &context) const override;
	std::string toString() const override;
	JsonNode toJsonNode() const override;
	void acceptUpdater(IUpdater & visitor) override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<ILimiter&>(*this);
		h & terrainType;
	}
};

class DLL_LINKAGE CreatureLevelLimiter : public ILimiter //applies only to creatures of given faction
{
public:
	uint32_t minLevel;
	uint32_t maxLevel;
	//accept all levels by default, accept creatures of minLevel <= creature->getLevel() < maxLevel
	CreatureLevelLimiter(uint32_t minLevel = std::numeric_limits<uint32_t>::min(), uint32_t maxLevel = std::numeric_limits<uint32_t>::max());

	EDecision limit(const BonusLimitationContext &context) const override;
	std::string toString() const override;
	JsonNode toJsonNode() const override;
	void acceptUpdater(IUpdater & visitor) override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<ILimiter&>(*this);
		h & minLevel;
		h & maxLevel;
	}
};

class DLL_LINKAGE FactionLimiter : public ILimiter //applies only to creatures of given faction
{
public:
	FactionID faction;
	FactionLimiter(FactionID faction = FactionID::DEFAULT);

	EDecision limit(const BonusLimitationContext &context) const override;
	std::string toString() const override;
	JsonNode toJsonNode() const override;
	void acceptUpdater(IUpdater & visitor) override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<ILimiter&>(*this);
		h & faction;
	}
};

class DLL_LINKAGE CreatureAlignmentLimiter : public ILimiter //applies only to creatures of given alignment
{
public:
	EAlignment alignment;
	CreatureAlignmentLimiter(EAlignment Alignment = EAlignment::NEUTRAL);

	EDecision limit(const BonusLimitationContext &context) const override;
	std::string toString() const override;
	JsonNode toJsonNode() const override;
	void acceptUpdater(IUpdater & visitor) override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<ILimiter&>(*this);
		h & alignment;
	}
};

class DLL_LINKAGE OppositeSideLimiter : public ILimiter //applies only to creatures of enemy army during combat
{
public:
	PlayerColor owner;
	OppositeSideLimiter(PlayerColor Owner = PlayerColor::CANNOT_DETERMINE);

	EDecision limit(const BonusLimitationContext &context) const override;
	void acceptUpdater(IUpdater & visitor) override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<ILimiter&>(*this);
		h & owner;
	}
};

class DLL_LINKAGE RankRangeLimiter : public ILimiter //applies to creatures with min <= Rank <= max
{
public:
	ui8 minRank;
	ui8 maxRank;

	RankRangeLimiter();
	RankRangeLimiter(ui8 Min, ui8 Max = 255);
	EDecision limit(const BonusLimitationContext &context) const override;
	void acceptUpdater(IUpdater & visitor) override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<ILimiter&>(*this);
		h & minRank;
		h & maxRank;
	}
};

class DLL_LINKAGE UnitOnHexLimiter : public ILimiter //works only on selected hexes
{
public:
	BattleHexArray applicableHexes;

	UnitOnHexLimiter(const BattleHexArray & applicableHexes = {});
	EDecision limit(const BonusLimitationContext &context) const override;
	JsonNode toJsonNode() const override;
	void acceptUpdater(IUpdater& visitor) override;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<ILimiter&>(*this);
		h & applicableHexes;
	}
};

VCMI_LIB_NAMESPACE_END
