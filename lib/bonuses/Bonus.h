/*
 * Bonus.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "BonusEnum.h"
#include "BonusCustomTypes.h"
#include "../constants/VariantIdentifier.h"
#include "../constants/EntityIdentifiers.h"
#include "../serializer/Serializeable.h"
#include "../texts/MetaString.h"
#include "../filesystem/ResourcePath.h"

VCMI_LIB_NAMESPACE_BEGIN

struct Bonus;
class IBonusBearer;
class CBonusSystemNode;
class ILimiter;
class IPropagator;
class IUpdater;
class BonusList;
class CSelector;
class IGameInfoCallback;

using BonusSubtypeID = VariantIdentifier<BonusCustomSubtype, SpellID, CreatureID, PrimarySkill, TerrainId, GameResID, SpellSchool>;
using BonusSourceID = VariantIdentifier<BonusCustomSource, SpellID, CreatureID, ArtifactID, CampaignScenarioID, SecondarySkill, HeroTypeID, Obj, ObjectInstanceID, BuildingTypeUniqueID, BattleField>;
using TBonusListPtr = std::shared_ptr<BonusList>;
using TConstBonusListPtr = std::shared_ptr<const BonusList>;
using TLimiterPtr = std::shared_ptr<const ILimiter>;
using TPropagatorPtr = std::shared_ptr<const IPropagator>;
using TUpdaterPtr = std::shared_ptr<const IUpdater>;

class DLL_LINKAGE CAddInfo : public std::vector<si32>
{
public:
	enum { NONE = -1 };

	CAddInfo();
	CAddInfo(si32 value);

	bool operator==(si32 value) const;
	bool operator!=(si32 value) const;

	si32 & operator[](size_type pos);
	si32 operator[](size_type pos) const;

	std::string toString() const;
	JsonNode toJsonNode() const;
};

/// Struct for handling bonuses of several types. Can be transferred to any hero
struct DLL_LINKAGE Bonus : public std::enable_shared_from_this<Bonus>, public Serializeable
{
	BonusDuration::Type duration = BonusDuration::PERMANENT; //uses BonusDuration values - 2 bytes
	si32 val = 0;
	si16 turnsRemain = 0; //used if duration is N_TURNS, N_DAYS or ONE_WEEK

	BonusValueType valType = BonusValueType::ADDITIVE_VALUE; // 1 byte
	BonusSource source = BonusSource::OTHER; //source type" uses BonusSource values - what gave that bonus - 1 byte
	BonusSource targetSourceType = BonusSource::OTHER;//Bonuses of what origin this amplifies, uses BonusSource values. Needed for PERCENT_TO_TARGET_TYPE. - 1 byte
	BonusLimitEffect effectRange = BonusLimitEffect::NO_LIMIT; // 1 byte
	BonusType type = BonusType::NONE; //uses BonusType values - says to what is this bonus - 2 bytes

	BonusSubtypeID subtype;
	BonusSourceID sid; //source id: id of object/artifact/spell
	std::string stacking; // bonuses with the same stacking value don't stack (e.g. Angel/Archangel morale bonus)

	CAddInfo additionalInfo;

	TLimiterPtr limiter;
	TPropagatorPtr propagator;
	TUpdaterPtr updater;
	TUpdaterPtr propagationUpdater;

	ImagePath customIconPath;
	MetaString description;
	PlayerColor bonusOwner = PlayerColor::CANNOT_DETERMINE;

	Bonus(BonusDuration::Type Duration, BonusType Type, BonusSource Src, si32 Val, BonusSourceID sourceID);
	Bonus(BonusDuration::Type Duration, BonusType Type, BonusSource Src, si32 Val, BonusSourceID sourceID, BonusSubtypeID subtype);
	Bonus(BonusDuration::Type Duration, BonusType Type, BonusSource Src, si32 Val, BonusSourceID sourceID, BonusSubtypeID subtype, BonusValueType ValType);
	Bonus() = default;

	template <typename Handler> void serialize(Handler &h)
	{
		h & duration;
		h & type;
		h & subtype;
		h & source;
		h & val;
		h & sid;
		h & description;
		if (h.hasFeature(Handler::Version::CUSTOM_BONUS_ICONS))
			h & customIconPath;
		h & additionalInfo;
		h & turnsRemain;
		h & valType;
		h & stacking;
		h & effectRange;
		h & limiter;
		h & propagator;
		h & updater;
		h & propagationUpdater;
		h & targetSourceType;
	}

	template <typename Ptr>
	static bool compareByAdditionalInfo(const Ptr& a, const Ptr& b)
	{
		return a->additionalInfo < b->additionalInfo;
	}
	static bool NDays(const Bonus *hb)
	{
		auto set = hb->duration & BonusDuration::N_DAYS;
		return set != 0;
	}
	static bool NTurns(const Bonus *hb)
	{
		auto set = hb->duration & BonusDuration::N_TURNS;
		return set != 0;
	}
	static bool OneDay(const Bonus *hb)
	{
		auto set = hb->duration & BonusDuration::ONE_DAY;
		return set != 0;
	}
	static bool OneWeek(const Bonus *hb)
	{
		auto set = hb->duration & BonusDuration::ONE_WEEK;
		return set != 0;
	}
	static bool OneBattle(const Bonus *hb)
	{
		auto set = hb->duration & BonusDuration::ONE_BATTLE;
		return set != 0;
	}
	static bool Permanent(const Bonus *hb)
	{
		auto set = hb->duration & BonusDuration::PERMANENT;
		return set != 0;
	}
	static bool UntilGetsTurn(const Bonus *hb)
	{
		auto set = hb->duration & BonusDuration::STACK_GETS_TURN;
		return set != 0;
	}
	static bool UntilAttack(const Bonus *hb)
	{
		auto set = hb->duration & BonusDuration::UNTIL_ATTACK;
		return set != 0;
	}
	static bool UntilBeingAttacked(const Bonus *hb)
	{
		auto set = hb->duration & BonusDuration::UNTIL_BEING_ATTACKED;
		return set != 0;
	}
	static bool UntilCommanderKilled(const Bonus *hb)
	{
		auto set = hb->duration & BonusDuration::COMMANDER_KILLED;
		return set != 0;
	}
	static bool UntilOwnAttack(const Bonus *hb)
	{
		auto set = hb->duration & BonusDuration::UNTIL_OWN_ATTACK;
		return set != 0;
	}
	inline bool operator == (const BonusType & cf) const
	{
		return type == cf;
	}
	inline void operator += (const ui32 Val) //no return
	{
		val += Val;
	}

	std::string Description(const IGameInfoCallback * cb, std::optional<si32> customValue = {}) const;
	JsonNode toJsonNode() const;

	std::shared_ptr<Bonus> addLimiter(const TLimiterPtr & Limiter); //returns this for convenient chain-calls
	std::shared_ptr<Bonus> addPropagator(const TPropagatorPtr & Propagator); //returns this for convenient chain-calls
	std::shared_ptr<Bonus> addUpdater(const TUpdaterPtr & Updater); //returns this for convenient chain-calls
};

DLL_LINKAGE std::ostream & operator<<(std::ostream &out, const Bonus &bonus);

VCMI_LIB_NAMESPACE_END
