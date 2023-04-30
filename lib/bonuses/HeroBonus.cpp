/*
 * HeroBonus.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "HeroBonus.h"
#include "CBonusSystemNode.h"
#include "Limiters.h"
#include "Updaters.h"
#include "Propagators.h"

#include "../VCMI_Lib.h"
#include "../spells/CSpellHandler.h"
#include "../CCreatureHandler.h"
#include "../CCreatureSet.h"
#include "../CHeroHandler.h"
#include "../CTownHandler.h"
#include "../CGeneralTextHandler.h"
#include "../CSkillHandler.h"
#include "../CStack.h"
#include "../CArtHandler.h"
#include "../CModHandler.h"
#include "../TerrainHandler.h"
#include "../StringConstants.h"
#include "../battle/BattleInfo.h"

VCMI_LIB_NAMESPACE_BEGIN

#define BONUS_NAME(x) { #x, Bonus::x },
	const std::map<std::string, Bonus::BonusType> bonusNameMap = {
		BONUS_LIST
	};
#undef BONUS_NAME

#define BONUS_VALUE(x) { #x, Bonus::x },
	const std::map<std::string, Bonus::ValueType> bonusValueMap = { BONUS_VALUE_LIST };
#undef BONUS_VALUE

#define BONUS_SOURCE(x) { #x, Bonus::x },
	const std::map<std::string, Bonus::BonusSource> bonusSourceMap = { BONUS_SOURCE_LIST };
#undef BONUS_SOURCE

#define BONUS_ITEM(x) { #x, Bonus::x },

const std::map<std::string, ui16> bonusDurationMap =
{
	BONUS_ITEM(PERMANENT)
	BONUS_ITEM(ONE_BATTLE)
	BONUS_ITEM(ONE_DAY)
	BONUS_ITEM(ONE_WEEK)
	BONUS_ITEM(N_TURNS)
	BONUS_ITEM(N_DAYS)
	BONUS_ITEM(UNTIL_BEING_ATTACKED)
	BONUS_ITEM(UNTIL_ATTACK)
	BONUS_ITEM(STACK_GETS_TURN)
	BONUS_ITEM(COMMANDER_KILLED)
	{ "UNITL_BEING_ATTACKED", Bonus::UNTIL_BEING_ATTACKED }//typo, but used in some mods
};

const std::map<std::string, Bonus::LimitEffect> bonusLimitEffect =
{
	BONUS_ITEM(NO_LIMIT)
	BONUS_ITEM(ONLY_DISTANCE_FIGHT)
	BONUS_ITEM(ONLY_MELEE_FIGHT)
};

const std::set<std::string> deprecatedBonusSet = {
	"SECONDARY_SKILL_PREMY",
	"SECONDARY_SKILL_VAL2",
	"MAXED_SPELL",
	"LAND_MOVEMENT",
	"SEA_MOVEMENT",
	"SIGHT_RADIOUS",
	"NO_TYPE",
	"SPECIAL_SECONDARY_SKILL",
	"FULL_HP_REGENERATION",
	"KING1",
	"KING2",
	"KING3",
	"BLOCK_MORALE",
	"BLOCK_LUCK",
	"SELF_MORALE",
	"SELF_LUCK"
};

//This constructor should be placed here to avoid side effects
CAddInfo::CAddInfo() = default;

CAddInfo::CAddInfo(si32 value)
{
	if(value != CAddInfo::NONE)
		push_back(value);
}

bool CAddInfo::operator==(si32 value) const
{
	switch(size())
	{
	case 0:
		return value == CAddInfo::NONE;
	case 1:
		return operator[](0) == value;
	default:
		return false;
	}
}

bool CAddInfo::operator!=(si32 value) const
{
	return !operator==(value);
}

si32 & CAddInfo::operator[](size_type pos)
{
	if(pos >= size())
		resize(pos + 1, CAddInfo::NONE);
	return vector::operator[](pos);
}

si32 CAddInfo::operator[](size_type pos) const
{
	return pos < size() ? vector::operator[](pos) : CAddInfo::NONE;
}

std::string CAddInfo::toString() const
{
	return toJsonNode().toJson(true);
}

JsonNode CAddInfo::toJsonNode() const
{
	if(size() < 2)
	{
		return JsonUtils::intNode(operator[](0));
	}
	else
	{
		JsonNode node(JsonNode::JsonType::DATA_VECTOR);
		for(si32 value : *this)
			node.Vector().push_back(JsonUtils::intNode(value));
		return node;
	}
}

BonusList::BonusList(bool BelongsToTree) : belongsToTree(BelongsToTree)
{

}

BonusList::BonusList(const BonusList & bonusList): belongsToTree(false)
{
	bonuses.resize(bonusList.size());
	std::copy(bonusList.begin(), bonusList.end(), bonuses.begin());
}

BonusList::BonusList(BonusList && other) noexcept: belongsToTree(false)
{
	std::swap(belongsToTree, other.belongsToTree);
	std::swap(bonuses, other.bonuses);
}

BonusList& BonusList::operator=(const BonusList &bonusList)
{
	bonuses.resize(bonusList.size());
	std::copy(bonusList.begin(), bonusList.end(), bonuses.begin());
	belongsToTree = false;
	return *this;
}

void BonusList::changed() const
{
    if(belongsToTree)
		CBonusSystemNode::treeHasChanged();
}

void BonusList::stackBonuses()
{
	boost::sort(bonuses, [](const std::shared_ptr<Bonus> & b1, const std::shared_ptr<Bonus> & b2) -> bool
	{
		if(b1 == b2)
			return false;
#define COMPARE_ATT(ATT) if(b1->ATT != b2->ATT) return b1->ATT < b2->ATT
		COMPARE_ATT(stacking);
		COMPARE_ATT(type);
		COMPARE_ATT(subtype);
		COMPARE_ATT(valType);
#undef COMPARE_ATT
		return b1->val > b2->val;
	});
	// remove non-stacking
	size_t next = 1;
	while(next < bonuses.size())
	{
		bool remove = false;
		std::shared_ptr<Bonus> last = bonuses[next-1];
		std::shared_ptr<Bonus> current = bonuses[next];

		if(current->stacking.empty())
			remove = current == last;
		else if(current->stacking == "ALWAYS")
			remove = false;
		else
			remove = current->stacking == last->stacking
				&& current->type == last->type
				&& current->subtype == last->subtype
				&& current->valType == last->valType;

		if(remove)
			bonuses.erase(bonuses.begin() + next);
		else
			next++;
	}
}

int BonusList::totalValue() const
{
	struct BonusCollection
	{
		int base = 0;
		int percentToBase = 0;
		int percentToAll = 0;
		int additive = 0;
		int percentToSource = 0;
		int indepMin = std::numeric_limits<int>::max();
		int indepMax = std::numeric_limits<int>::min();
	};

	auto percent = [](int64_t base, int64_t percent) -> int {
		return static_cast<int>(std::clamp<int64_t>((base * (100 + percent)) / 100, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()));
	};
	std::array <BonusCollection, Bonus::BonusSource::NUM_BONUS_SOURCE> sources = {};
	BonusCollection any;
	bool hasIndepMax = false;
	bool hasIndepMin = false;

	for(const auto & b : bonuses)
	{
		switch(b->valType)
		{
		case Bonus::BASE_NUMBER:
			sources[b->source].base += b->val;
			break;
		case Bonus::PERCENT_TO_ALL:
			sources[b->source].percentToAll += b->val;
			break;
		case Bonus::PERCENT_TO_BASE:
			sources[b->source].percentToBase += b->val;
			break;
		case Bonus::PERCENT_TO_SOURCE:
			sources[b->source].percentToSource += b->val;
			break;
		case Bonus::PERCENT_TO_TARGET_TYPE:
			sources[b->targetSourceType].percentToSource += b->val;
			break;
		case Bonus::ADDITIVE_VALUE:
			sources[b->source].additive += b->val;
			break;
		case Bonus::INDEPENDENT_MAX:
			hasIndepMax = true;
			vstd::amax(sources[b->source].indepMax, b->val);
			break;
		case Bonus::INDEPENDENT_MIN:
			hasIndepMin = true;
			vstd::amin(sources[b->source].indepMin, b->val);
			break;
		}
	}
	for(const auto & src : sources)
	{
		any.base += percent(src.base, src.percentToSource);
		any.percentToBase += percent(src.percentToBase, src.percentToSource);
		any.percentToAll += percent(src.percentToAll, src.percentToSource);
		any.additive += percent(src.additive, src.percentToSource);
		if(hasIndepMin)
			vstd::amin(any.indepMin, percent(src.indepMin, src.percentToSource));
		if(hasIndepMax)
			vstd::amax(any.indepMax, percent(src.indepMax, src.percentToSource));
	}
	any.base = percent(any.base, any.percentToBase);
	any.base += any.additive;
	auto valFirst = percent(any.base ,any.percentToAll);

	if(hasIndepMin && hasIndepMax && any.indepMin < any.indepMax)
		any.indepMax = any.indepMin;

	const int notIndepBonuses = static_cast<int>(std::count_if(bonuses.cbegin(), bonuses.cend(), [](const std::shared_ptr<Bonus>& b)
	{
		return b->valType != Bonus::INDEPENDENT_MAX && b->valType != Bonus::INDEPENDENT_MIN;
	}));

	if(notIndepBonuses)
		return std::clamp(valFirst, any.indepMax, any.indepMin);
	
	return hasIndepMin ? any.indepMin : hasIndepMax ? any.indepMax : 0;
}

std::shared_ptr<Bonus> BonusList::getFirst(const CSelector &select)
{
	for (auto & b : bonuses)
	{
		if(select(b.get()))
			return b;
	}
	return nullptr;
}

std::shared_ptr<const Bonus> BonusList::getFirst(const CSelector &selector) const
{
	for(const auto & b : bonuses)
	{
		if(selector(b.get()))
			return b;
	}
	return nullptr;
}

void BonusList::getBonuses(BonusList & out, const CSelector &selector, const CSelector &limit) const
{
	out.reserve(bonuses.size());
	for(const auto & b : bonuses)
	{
		//add matching bonuses that matches limit predicate or have NO_LIMIT if no given predicate
		auto noFightLimit = b->effectRange == Bonus::NO_LIMIT;
		if(selector(b.get()) && ((!limit && noFightLimit) || ((bool)limit && limit(b.get()))))
			out.push_back(b);
	}
}

void BonusList::getAllBonuses(BonusList &out) const
{
	for(const auto & b : bonuses)
		out.push_back(b);
}

int BonusList::valOfBonuses(const CSelector &select) const
{
	BonusList ret;
	CSelector limit = nullptr;
	getBonuses(ret, select, limit);
	return ret.totalValue();
}

JsonNode BonusList::toJsonNode() const
{
	JsonNode node(JsonNode::JsonType::DATA_VECTOR);
	for(const std::shared_ptr<Bonus> & b : bonuses)
		node.Vector().push_back(b->toJsonNode());
	return node;
}

void BonusList::push_back(const std::shared_ptr<Bonus> & x)
{
	bonuses.push_back(x);
	changed();
}

BonusList::TInternalContainer::iterator BonusList::erase(const int position)
{
	changed();
	return bonuses.erase(bonuses.begin() + position);
}

void BonusList::clear()
{
	bonuses.clear();
	changed();
}

std::vector<BonusList *>::size_type BonusList::operator-=(const std::shared_ptr<Bonus> & i)
{
	auto itr = std::find(bonuses.begin(), bonuses.end(), i);
	if(itr == bonuses.end())
		return false;
	bonuses.erase(itr);
	changed();
	return true;
}

void BonusList::resize(BonusList::TInternalContainer::size_type sz, const std::shared_ptr<Bonus> & c)
{
	bonuses.resize(sz, c);
	changed();
}

void BonusList::reserve(TInternalContainer::size_type sz)
{
	bonuses.reserve(sz);
}

void BonusList::insert(BonusList::TInternalContainer::iterator position, BonusList::TInternalContainer::size_type n, const std::shared_ptr<Bonus> & x)
{
	bonuses.insert(position, n, x);
	changed();
}

std::string Bonus::Description(std::optional<si32> customValue) const
{
	std::ostringstream str;

	if(description.empty())
	{
		if(stacking.empty() || stacking == "ALWAYS")
		{
			switch(source)
			{
			case ARTIFACT:
				str << ArtifactID(sid).toArtifact(VLC->artifacts())->getNameTranslated();
				break;
			case SPELL_EFFECT:
				str << SpellID(sid).toSpell(VLC->spells())->getNameTranslated();
				break;
			case CREATURE_ABILITY:
				str << VLC->creh->objects[sid]->getNamePluralTranslated();
				break;
			case SECONDARY_SKILL:
				str << VLC->skillh->getByIndex(sid)->getNameTranslated();
				break;
			case HERO_SPECIAL:
				str << VLC->heroh->objects[sid]->getNameTranslated();
				break;
			default:
				//todo: handle all possible sources
				str << "Unknown";
				break;
			}
		}
		else
			str << stacking;
	}
	else
	{
		str << description;
	}

	if(auto value = customValue.value_or(val))
		str << " " << std::showpos << value;

	return str.str();
}

JsonNode subtypeToJson(Bonus::BonusType type, int subtype)
{
	switch(type)
	{
	case Bonus::PRIMARY_SKILL:
		return JsonUtils::stringNode("primSkill." + PrimarySkill::names[subtype]);
	case Bonus::SPECIAL_SPELL_LEV:
	case Bonus::SPECIFIC_SPELL_DAMAGE:
	case Bonus::SPELL:
	case Bonus::SPECIAL_PECULIAR_ENCHANT:
	case Bonus::SPECIAL_ADD_VALUE_ENCHANT:
	case Bonus::SPECIAL_FIXED_VALUE_ENCHANT:
		return JsonUtils::stringNode(CModHandler::makeFullIdentifier("", "spell", SpellID::encode(subtype)));
	case Bonus::IMPROVED_NECROMANCY:
	case Bonus::SPECIAL_UPGRADE:
		return JsonUtils::stringNode(CModHandler::makeFullIdentifier("", "creature", CreatureID::encode(subtype)));
	case Bonus::GENERATE_RESOURCE:
		return JsonUtils::stringNode("resource." + GameConstants::RESOURCE_NAMES[subtype]);
	default:
		return JsonUtils::intNode(subtype);
	}
}

JsonNode additionalInfoToJson(Bonus::BonusType type, CAddInfo addInfo)
{
	switch(type)
	{
	case Bonus::SPECIAL_UPGRADE:
		return JsonUtils::stringNode(CModHandler::makeFullIdentifier("", "creature", CreatureID::encode(addInfo[0])));
	default:
		return addInfo.toJsonNode();
	}
}

JsonNode durationToJson(ui16 duration)
{
	std::vector<std::string> durationNames;
	for(ui16 durBit = 1; durBit; durBit = durBit << 1)
	{
		if(duration & durBit)
			durationNames.push_back(vstd::findKey(bonusDurationMap, durBit));
	}
	if(durationNames.size() == 1)
	{
		return JsonUtils::stringNode(durationNames[0]);
	}
	else
	{
		JsonNode node(JsonNode::JsonType::DATA_VECTOR);
		for(const std::string & dur : durationNames)
			node.Vector().push_back(JsonUtils::stringNode(dur));
		return node;
	}
}

JsonNode Bonus::toJsonNode() const
{
	JsonNode root(JsonNode::JsonType::DATA_STRUCT);
	// only add values that might reasonably be found in config files
	root["type"].String() = vstd::findKey(bonusNameMap, type);
	if(subtype != -1)
		root["subtype"] = subtypeToJson(type, subtype);
	if(additionalInfo != CAddInfo::NONE)
		root["addInfo"] = additionalInfoToJson(type, additionalInfo);
	if(duration != 0)
	{
		JsonNode durationVec(JsonNode::JsonType::DATA_VECTOR);
		for(const auto & kv : bonusDurationMap)
		{
			if(duration & kv.second)
				durationVec.Vector().push_back(JsonUtils::stringNode(kv.first));
		}
		root["duration"] = durationVec;
	}
	if(turnsRemain != 0)
		root["turns"].Integer() = turnsRemain;
	if(source != OTHER)
		root["sourceType"].String() = vstd::findKey(bonusSourceMap, source);
	if(targetSourceType != OTHER)
		root["targetSourceType"].String() = vstd::findKey(bonusSourceMap, targetSourceType);
	if(sid != 0)
		root["sourceID"].Integer() = sid;
	if(val != 0)
		root["val"].Integer() = val;
	if(valType != ADDITIVE_VALUE)
		root["valueType"].String() = vstd::findKey(bonusValueMap, valType);
	if(!stacking.empty())
		root["stacking"].String() = stacking;
	if(!description.empty())
		root["description"].String() = description;
	if(effectRange != NO_LIMIT)
		root["effectRange"].String() = vstd::findKey(bonusLimitEffect, effectRange);
	if(duration != PERMANENT)
		root["duration"] = durationToJson(duration);
	if(turnsRemain)
		root["turns"].Integer() = turnsRemain;
	if(limiter)
		root["limiters"] = limiter->toJsonNode();
	if(updater)
		root["updater"] = updater->toJsonNode();
	if(propagator)
		root["propagator"].String() = vstd::findKey(bonusPropagatorMap, propagator);
	return root;
}

std::string Bonus::nameForBonus() const
{
	switch(type)
	{
	case Bonus::PRIMARY_SKILL:
		return PrimarySkill::names[subtype];
	case Bonus::SPECIAL_SPELL_LEV:
	case Bonus::SPECIFIC_SPELL_DAMAGE:
	case Bonus::SPELL:
	case Bonus::SPECIAL_PECULIAR_ENCHANT:
	case Bonus::SPECIAL_ADD_VALUE_ENCHANT:
	case Bonus::SPECIAL_FIXED_VALUE_ENCHANT:
		return VLC->spells()->getByIndex(subtype)->getJsonKey();
	case Bonus::SPECIAL_UPGRADE:
		return CreatureID::encode(subtype) + "2" + CreatureID::encode(additionalInfo[0]);
	case Bonus::GENERATE_RESOURCE:
		return GameConstants::RESOURCE_NAMES[subtype];
	case Bonus::STACKS_SPEED:
		return "speed";
	default:
		return vstd::findKey(bonusNameMap, type);
	}
}

Bonus::Bonus(Bonus::BonusDuration Duration, BonusType Type, BonusSource Src, si32 Val, ui32 ID, std::string Desc, si32 Subtype):
	duration(static_cast<ui16>(Duration)),
	type(Type),
	subtype(Subtype),
	source(Src),
	val(Val),
	sid(ID),
	description(std::move(Desc))
{
	boost::algorithm::trim(description);
	targetSourceType = OTHER;
}

Bonus::Bonus(Bonus::BonusDuration Duration, BonusType Type, BonusSource Src, si32 Val, ui32 ID, si32 Subtype, ValueType ValType):
	duration(static_cast<ui16>(Duration)),
	type(Type),
	subtype(Subtype),
	source(Src),
	val(Val),
	sid(ID),
	valType(ValType)
{
	turnsRemain = 0;
	effectRange = NO_LIMIT;
	targetSourceType = OTHER;
}

std::shared_ptr<Bonus> Bonus::addPropagator(const TPropagatorPtr & Propagator)
{
	propagator = Propagator;
	return this->shared_from_this();
}

namespace Selector
{
	DLL_LINKAGE CSelectFieldEqual<Bonus::BonusType> & type()
	{
		static CSelectFieldEqual<Bonus::BonusType> stype(&Bonus::type);
		return stype;
	}

	DLL_LINKAGE CSelectFieldEqual<TBonusSubtype> & subtype()
	{
		static CSelectFieldEqual<TBonusSubtype> ssubtype(&Bonus::subtype);
		return ssubtype;
	}

	DLL_LINKAGE CSelectFieldEqual<CAddInfo> & info()
	{
		static CSelectFieldEqual<CAddInfo> sinfo(&Bonus::additionalInfo);
		return sinfo;
	}

	DLL_LINKAGE CSelectFieldEqual<Bonus::BonusSource> & sourceType()
	{
		static CSelectFieldEqual<Bonus::BonusSource> ssourceType(&Bonus::source);
		return ssourceType;
	}

	DLL_LINKAGE CSelectFieldEqual<Bonus::BonusSource> & targetSourceType()
	{
		static CSelectFieldEqual<Bonus::BonusSource> ssourceType(&Bonus::targetSourceType);
		return ssourceType;
	}

	DLL_LINKAGE CSelectFieldEqual<Bonus::LimitEffect> & effectRange()
	{
		static CSelectFieldEqual<Bonus::LimitEffect> seffectRange(&Bonus::effectRange);
		return seffectRange;
	}

	DLL_LINKAGE CWillLastTurns turns;
	DLL_LINKAGE CWillLastDays days;

	CSelector DLL_LINKAGE typeSubtype(Bonus::BonusType Type, TBonusSubtype Subtype)
	{
		return type()(Type).And(subtype()(Subtype));
	}

	CSelector DLL_LINKAGE typeSubtypeInfo(Bonus::BonusType type, TBonusSubtype subtype, const CAddInfo & info)
	{
		return CSelectFieldEqual<Bonus::BonusType>(&Bonus::type)(type)
			.And(CSelectFieldEqual<TBonusSubtype>(&Bonus::subtype)(subtype))
			.And(CSelectFieldEqual<CAddInfo>(&Bonus::additionalInfo)(info));
	}

	CSelector DLL_LINKAGE source(Bonus::BonusSource source, ui32 sourceID)
	{
		return CSelectFieldEqual<Bonus::BonusSource>(&Bonus::source)(source)
			.And(CSelectFieldEqual<ui32>(&Bonus::sid)(sourceID));
	}

	CSelector DLL_LINKAGE sourceTypeSel(Bonus::BonusSource source)
	{
		return CSelectFieldEqual<Bonus::BonusSource>(&Bonus::source)(source);
	}

	CSelector DLL_LINKAGE valueType(Bonus::ValueType valType)
	{
		return CSelectFieldEqual<Bonus::ValueType>(&Bonus::valType)(valType);
	}

	DLL_LINKAGE CSelector all([](const Bonus * b){return true;});
	DLL_LINKAGE CSelector none([](const Bonus * b){return false;});
}

DLL_LINKAGE std::ostream & operator<<(std::ostream &out, const BonusList &bonusList)
{
	for (ui32 i = 0; i < bonusList.size(); i++)
	{
		const auto & b = bonusList[i];
		out << "Bonus " << i << "\n" << *b << std::endl;
	}
	return out;
}

DLL_LINKAGE std::ostream & operator<<(std::ostream &out, const Bonus &bonus)
{
	for(const auto & i : bonusNameMap)
	if(i.second == bonus.type)
		out << "\tType: " << i.first << " \t";

#define printField(field) out << "\t" #field ": " << (int)bonus.field << "\n"
	printField(val);
	printField(subtype);
	printField(duration);
	printField(source);
	printField(sid);
	if(bonus.additionalInfo != CAddInfo::NONE)
		out << "\taddInfo: " << bonus.additionalInfo.toString() << "\n";
	printField(turnsRemain);
	printField(valType);
	if(!bonus.stacking.empty())
		out << "\tstacking: \"" << bonus.stacking << "\"\n";
	printField(effectRange);
#undef printField

	if(bonus.limiter)
		out << "\tLimiter: " << bonus.limiter->toString() << "\n";
	if(bonus.updater)
		out << "\tUpdater: " << bonus.updater->toString() << "\n";

	return out;
}

std::shared_ptr<Bonus> Bonus::addLimiter(const TLimiterPtr & Limiter)
{
	if (limiter)
	{
		//If we already have limiter list, retrieve it
		auto limiterList = std::dynamic_pointer_cast<AllOfLimiter>(limiter);
		if(!limiterList)
		{
			//Create a new limiter list with old limiter and the new one will be pushed later
			limiterList = std::make_shared<AllOfLimiter>();
			limiterList->add(limiter);
			limiter = limiterList;
		}

		limiterList->add(Limiter);
	}
	else
	{
		limiter = Limiter;
	}
	return this->shared_from_this();
}

// Updaters

std::shared_ptr<Bonus> Bonus::addUpdater(const TUpdaterPtr & Updater)
{
	updater = Updater;
	return this->shared_from_this();
}

VCMI_LIB_NAMESPACE_END
