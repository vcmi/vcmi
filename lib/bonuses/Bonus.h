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

VCMI_LIB_NAMESPACE_BEGIN

struct Bonus;
class IBonusBearer;
class CBonusSystemNode;
class ILimiter;
class IPropagator;
class IUpdater;
class BonusList;
class CSelector;

using TBonusSubtype = int32_t;
using TBonusListPtr = std::shared_ptr<BonusList>;
using TConstBonusListPtr = std::shared_ptr<const BonusList>;
using TLimiterPtr = std::shared_ptr<ILimiter>;
using TPropagatorPtr = std::shared_ptr<IPropagator>;
using TUpdaterPtr = std::shared_ptr<IUpdater>;

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

#define BONUS_TREE_DESERIALIZATION_FIX if(!h.saving && h.smartPointerSerialization) deserializationFix();

/// Struct for handling bonuses of several types. Can be transferred to any hero
struct DLL_LINKAGE Bonus : public std::enable_shared_from_this<Bonus>
{
	BonusDuration::Type duration = BonusDuration::PERMANENT; //uses BonusDuration values
	si16 turnsRemain = 0; //used if duration is N_TURNS, N_DAYS or ONE_WEEK

	BonusType type = BonusType::NONE; //uses BonusType values - says to what is this bonus - 1 byte
	TBonusSubtype subtype = -1; //-1 if not applicable - 4 bytes

	BonusSource source = BonusSource::OTHER; //source type" uses BonusSource values - what gave that bonus
	BonusSource targetSourceType;//Bonuses of what origin this amplifies, uses BonusSource values. Needed for PERCENT_TO_TARGET_TYPE.
	si32 val = 0;
	ui32 sid = 0; //source id: id of object/artifact/spell
	BonusValueType valType = BonusValueType::ADDITIVE_VALUE;
	std::string stacking; // bonuses with the same stacking value don't stack (e.g. Angel/Archangel morale bonus)

	CAddInfo additionalInfo;
	BonusLimitEffect effectRange = BonusLimitEffect::NO_LIMIT; //if not NO_LIMIT, bonus will be omitted by default

	TLimiterPtr limiter;
	TPropagatorPtr propagator;
	TUpdaterPtr updater;
	TUpdaterPtr propagationUpdater;

	std::string description;

	Bonus(BonusDuration::Type Duration, BonusType Type, BonusSource Src, si32 Val, ui32 ID, std::string Desc, si32 Subtype=-1);
	Bonus(BonusDuration::Type Duration, BonusType Type, BonusSource Src, si32 Val, ui32 ID, si32 Subtype=-1, BonusValueType ValType = BonusValueType::ADDITIVE_VALUE);
	Bonus() = default;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & duration;
		h & type;
		h & subtype;
		h & source;
		h & val;
		h & sid;
		h & description;
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
		return set.any();
	}
	static bool NTurns(const Bonus *hb)
	{
		auto set = hb->duration & BonusDuration::N_TURNS;
		return set.any();
	}
	static bool OneDay(const Bonus *hb)
	{
		auto set = hb->duration & BonusDuration::ONE_DAY;
		return set.any();
	}
	static bool OneWeek(const Bonus *hb)
	{
		auto set = hb->duration & BonusDuration::ONE_WEEK;
		return set.any();
	}
	static bool OneBattle(const Bonus *hb)
	{
		auto set = hb->duration & BonusDuration::ONE_BATTLE;
		return set.any();
	}
	static bool Permanent(const Bonus *hb)
	{
		auto set = hb->duration & BonusDuration::PERMANENT;
		return set.any();
	}
	static bool UntilGetsTurn(const Bonus *hb)
	{
		auto set = hb->duration & BonusDuration::STACK_GETS_TURN;
		return set.any();
	}
	static bool UntilAttack(const Bonus *hb)
	{
		auto set = hb->duration & BonusDuration::UNTIL_ATTACK;
		return set.any();
	}
	static bool UntilBeingAttacked(const Bonus *hb)
	{
		auto set = hb->duration & BonusDuration::UNTIL_BEING_ATTACKED;
		return set.any();
	}
	static bool UntilCommanderKilled(const Bonus *hb)
	{
		auto set = hb->duration & BonusDuration::COMMANDER_KILLED;
		return set.any();
	}
	inline bool operator == (const BonusType & cf) const
	{
		return type == cf;
	}
	inline void operator += (const ui32 Val) //no return
	{
		val += Val;
	}
	STRONG_INLINE static ui32 getSid32(ui32 high, ui32 low)
	{
		return (high << 16) + low;
	}

	STRONG_INLINE static ui32 getHighFromSid32(ui32 sid)
	{
		return sid >> 16;
	}

	STRONG_INLINE static ui32 getLowFromSid32(ui32 sid)
	{
		return sid & 0x0000FFFF;
	}

	std::string Description(std::optional<si32> customValue = {}) const;
	JsonNode toJsonNode() const;

	std::shared_ptr<Bonus> addLimiter(const TLimiterPtr & Limiter); //returns this for convenient chain-calls
	std::shared_ptr<Bonus> addPropagator(const TPropagatorPtr & Propagator); //returns this for convenient chain-calls
	std::shared_ptr<Bonus> addUpdater(const TUpdaterPtr & Updater); //returns this for convenient chain-calls
};

DLL_LINKAGE std::ostream & operator<<(std::ostream &out, const Bonus &bonus);

VCMI_LIB_NAMESPACE_END
