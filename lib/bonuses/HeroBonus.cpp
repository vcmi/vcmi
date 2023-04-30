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
#include "ILimiter.h"
#include "IUpdater.h"

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

#define FOREACH_PARENT(pname) 	TNodes lparents; getParents(lparents); for(CBonusSystemNode *pname : lparents)
#define FOREACH_RED_CHILD(pname) 	TNodes lchildren; getRedChildren(lchildren); for(CBonusSystemNode *pname : lchildren)

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

const std::map<std::string, TPropagatorPtr> bonusPropagatorMap =
{
	{"BATTLE_WIDE", std::make_shared<CPropagatorNodeType>(CBonusSystemNode::BATTLE)},
	{"VISITED_TOWN_AND_VISITOR", std::make_shared<CPropagatorNodeType>(CBonusSystemNode::TOWN_AND_VISITOR)},
	{"PLAYER_PROPAGATOR", std::make_shared<CPropagatorNodeType>(CBonusSystemNode::PLAYER)},
	{"HERO", std::make_shared<CPropagatorNodeType>(CBonusSystemNode::HERO)},
	{"TEAM_PROPAGATOR", std::make_shared<CPropagatorNodeType>(CBonusSystemNode::TEAM)}, //untested
	{"GLOBAL_EFFECT", std::make_shared<CPropagatorNodeType>(CBonusSystemNode::GLOBAL_EFFECTS)}
}; //untested

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

///CBonusProxy
CBonusProxy::CBonusProxy(const IBonusBearer * Target, CSelector Selector):
	bonusListCachedLast(0),
	target(Target),
	selector(std::move(Selector)),
	currentBonusListIndex(0)
{

}

CBonusProxy::CBonusProxy(const CBonusProxy & other):
	bonusListCachedLast(other.bonusListCachedLast),
	target(other.target),
	selector(other.selector),
	currentBonusListIndex(other.currentBonusListIndex)
{
	bonusList[currentBonusListIndex] = other.bonusList[currentBonusListIndex];
}

CBonusProxy::CBonusProxy(CBonusProxy && other) noexcept:
	bonusListCachedLast(0),
	target(other.target),
	currentBonusListIndex(0)
{
	std::swap(bonusListCachedLast, other.bonusListCachedLast);
	std::swap(selector, other.selector);
	std::swap(bonusList, other.bonusList);
	std::swap(currentBonusListIndex, other.currentBonusListIndex);
}

CBonusProxy & CBonusProxy::operator=(const CBonusProxy & other)
{
	boost::lock_guard<boost::mutex> lock(swapGuard);

	selector = other.selector;
	swapBonusList(other.bonusList[other.currentBonusListIndex]);
	bonusListCachedLast = other.bonusListCachedLast;

	return *this;
}

CBonusProxy & CBonusProxy::operator=(CBonusProxy && other) noexcept
{
	std::swap(bonusListCachedLast, other.bonusListCachedLast);
	std::swap(selector, other.selector);
	std::swap(bonusList, other.bonusList);
	std::swap(currentBonusListIndex, other.currentBonusListIndex);

	return *this;
}

void CBonusProxy::swapBonusList(TConstBonusListPtr other) const
{
	// The idea here is to avoid changing active bonusList while it can be read by a different thread.
	// Because such use of shared ptr is not thread safe
	// So to avoid this we change the second offline instance and swap active index
	auto newCurrent = 1 - currentBonusListIndex;
	bonusList[newCurrent] = std::move(other);
	currentBonusListIndex = newCurrent;
}

TConstBonusListPtr CBonusProxy::getBonusList() const
{
	auto needUpdateBonusList = [&]() -> bool
	{
		return target->getTreeVersion() != bonusListCachedLast || !bonusList[currentBonusListIndex];
	};

	// avoid locking if everything is up-to-date
	if(needUpdateBonusList())
	{
		boost::lock_guard<boost::mutex>lock(swapGuard);

		if(needUpdateBonusList())
		{
			//TODO: support limiters
			swapBonusList(target->getAllBonuses(selector, Selector::all));
			bonusListCachedLast = target->getTreeVersion();
		}
	}

	return bonusList[currentBonusListIndex];
}

const BonusList * CBonusProxy::operator->() const
{
	return getBonusList().get();
}

CTotalsProxy::CTotalsProxy(const IBonusBearer * Target, CSelector Selector, int InitialValue):
	CBonusProxy(Target, std::move(Selector)),
	initialValue(InitialValue),
	meleeCachedLast(0),
	meleeValue(0),
	rangedCachedLast(0),
	rangedValue(0)
{
}

CTotalsProxy::CTotalsProxy(const CTotalsProxy & other)
	: CBonusProxy(other),
	initialValue(other.initialValue),
	meleeCachedLast(other.meleeCachedLast),
	meleeValue(other.meleeValue),
	rangedCachedLast(other.rangedCachedLast),
	rangedValue(other.rangedValue)
{
}

int CTotalsProxy::getValue() const
{
	const auto treeVersion = target->getTreeVersion();

	if(treeVersion != valueCachedLast)
	{
		auto bonuses = getBonusList();

		value = initialValue + bonuses->totalValue();
		valueCachedLast = treeVersion;
	}
	return value;
}

int CTotalsProxy::getValueAndList(TConstBonusListPtr & outBonusList) const
{
	const auto treeVersion = target->getTreeVersion();
	outBonusList = getBonusList();

	if(treeVersion != valueCachedLast)
	{
		value = initialValue + outBonusList->totalValue();
		valueCachedLast = treeVersion;
	}
	return value;
}

int CTotalsProxy::getMeleeValue() const
{
	static const auto limit = Selector::effectRange()(Bonus::NO_LIMIT).Or(Selector::effectRange()(Bonus::ONLY_MELEE_FIGHT));

	const auto treeVersion = target->getTreeVersion();

	if(treeVersion != meleeCachedLast)
	{
		auto bonuses = target->getBonuses(selector, limit);
		meleeValue = initialValue + bonuses->totalValue();
		meleeCachedLast = treeVersion;
	}

	return meleeValue;
}

int CTotalsProxy::getRangedValue() const
{
	static const auto limit = Selector::effectRange()(Bonus::NO_LIMIT).Or(Selector::effectRange()(Bonus::ONLY_DISTANCE_FIGHT));

	const auto treeVersion = target->getTreeVersion();

	if(treeVersion != rangedCachedLast)
	{
		auto bonuses = target->getBonuses(selector, limit);
		rangedValue = initialValue + bonuses->totalValue();
		rangedCachedLast = treeVersion;
	}

	return rangedValue;
}

///CCheckProxy
CCheckProxy::CCheckProxy(const IBonusBearer * Target, CSelector Selector):
	target(Target),
	selector(std::move(Selector)),
	cachedLast(0),
	hasBonus(false)
{
}

//This constructor should be placed here to avoid side effects
CCheckProxy::CCheckProxy(const CCheckProxy & other) = default;

bool CCheckProxy::getHasBonus() const
{
	const auto treeVersion = target->getTreeVersion();

	if(treeVersion != cachedLast)
	{
		hasBonus = target->hasBonus(selector);
		cachedLast = treeVersion;
	}

	return hasBonus;
}

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

std::atomic<int64_t> CBonusSystemNode::treeChanged(1);
constexpr bool CBonusSystemNode::cachingEnabled = true;

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

int IBonusBearer::valOfBonuses(Bonus::BonusType type, const CSelector &selector) const
{
	return valOfBonuses(Selector::type()(type).And(selector));
}

int IBonusBearer::valOfBonuses(Bonus::BonusType type, int subtype) const
{
	//This part is performance-critical
	std::string cachingStr = "type_" + std::to_string(static_cast<int>(type)) + "_" + std::to_string(subtype);

	CSelector s = Selector::type()(type);
	if(subtype != -1)
		s = s.And(Selector::subtype()(subtype));

	return valOfBonuses(s, cachingStr);
}

int IBonusBearer::valOfBonuses(const CSelector &selector, const std::string &cachingStr) const
{
	CSelector limit = nullptr;
	TConstBonusListPtr hlp = getAllBonuses(selector, limit, nullptr, cachingStr);
	return hlp->totalValue();
}
bool IBonusBearer::hasBonus(const CSelector &selector, const std::string &cachingStr) const
{
	//TODO: We don't need to count all bonuses and could break on first matching
	return getBonuses(selector, cachingStr)->size() > 0;
}

bool IBonusBearer::hasBonus(const CSelector &selector, const CSelector &limit, const std::string &cachingStr) const
{
	return getBonuses(selector, limit, cachingStr)->size() > 0;
}

bool IBonusBearer::hasBonusOfType(Bonus::BonusType type, int subtype) const
{
	//This part is performance-ciritcal
	std::string cachingStr = "type_" + std::to_string(static_cast<int>(type)) + "_" + std::to_string(subtype);

	CSelector s = Selector::type()(type);
	if(subtype != -1)
		s = s.And(Selector::subtype()(subtype));

	return hasBonus(s, cachingStr);
}

TConstBonusListPtr IBonusBearer::getBonuses(const CSelector &selector, const std::string &cachingStr) const
{
	return getAllBonuses(selector, nullptr, nullptr, cachingStr);
}

TConstBonusListPtr IBonusBearer::getBonuses(const CSelector &selector, const CSelector &limit, const std::string &cachingStr) const
{
	return getAllBonuses(selector, limit, nullptr, cachingStr);
}

bool IBonusBearer::hasBonusFrom(Bonus::BonusSource source, ui32 sourceID) const
{
	boost::format fmt("source_%did_%d");
	fmt % static_cast<int>(source) % sourceID;

	return hasBonus(Selector::source(source,sourceID), fmt.str());
}
std::shared_ptr<const Bonus> IBonusBearer::getBonus(const CSelector &selector) const
{
	auto bonuses = getAllBonuses(selector, Selector::all);
	return bonuses->getFirst(Selector::all);
}

PlayerColor CBonusSystemNode::retrieveNodeOwner(const CBonusSystemNode * node)
{
	return node ? node->getOwner() : PlayerColor::CANNOT_DETERMINE;
}

std::shared_ptr<Bonus> CBonusSystemNode::getBonusLocalFirst(const CSelector & selector)
{
	auto ret = bonuses.getFirst(selector);
	if(ret)
		return ret;

	FOREACH_PARENT(pname)
	{
		ret = pname->getBonusLocalFirst(selector);
		if (ret)
			return ret;
	}

	return nullptr;
}

std::shared_ptr<const Bonus> CBonusSystemNode::getBonusLocalFirst(const CSelector & selector) const
{
	return (const_cast<CBonusSystemNode*>(this))->getBonusLocalFirst(selector);
}

void CBonusSystemNode::getParents(TCNodes & out) const /*retrieves list of parent nodes (nodes to inherit bonuses from) */
{
	for(const auto & elem : parents)
	{
		const CBonusSystemNode *parent = elem;
		out.insert(parent);
	}
}

void CBonusSystemNode::getParents(TNodes &out)
{
	for (auto & elem : parents)
	{
		const CBonusSystemNode *parent = elem;
		out.insert(const_cast<CBonusSystemNode*>(parent));
	}
}

void CBonusSystemNode::getAllParents(TCNodes & out) const //retrieves list of parent nodes (nodes to inherit bonuses from)
{
	for(auto * parent : parents)
	{
		out.insert(parent);
		parent->getAllParents(out);
	}
}

void CBonusSystemNode::getAllBonusesRec(BonusList &out, const CSelector & selector) const
{
	//out has been reserved sufficient capacity at getAllBonuses() call

	BonusList beforeUpdate;
	TCNodes lparents;
	getAllParents(lparents);

	if(!lparents.empty())
	{
		//estimate on how many bonuses are missing yet - must be positive
		beforeUpdate.reserve(std::max(out.capacity() - out.size(), bonuses.size()));
	}
	else
	{
		beforeUpdate.reserve(bonuses.size()); //at most all local bonuses
	}

	for(const auto * parent : lparents)
	{
		parent->getAllBonusesRec(beforeUpdate, selector);
	}
	bonuses.getAllBonuses(beforeUpdate);

	for(const auto & b : beforeUpdate)
	{
		//We should not run updaters on non-selected bonuses
		auto updated = selector(b.get()) && b->updater
			? getUpdatedBonus(b, b->updater)
			: b;

		//do not add bonus with updater
		bool bonusExists = false;
		for(const auto & bonus : out)
		{
			if (bonus == updated)
				bonusExists = true;
			if (bonus->updater && bonus->updater == updated->updater)
				bonusExists = true;
		}

		if (!bonusExists)
			out.push_back(updated);
	}
}

TConstBonusListPtr CBonusSystemNode::getAllBonuses(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root, const std::string &cachingStr) const
{
	bool limitOnUs = (!root || root == this); //caching won't work when we want to limit bonuses against an external node
	if (CBonusSystemNode::cachingEnabled && limitOnUs)
	{
		// Exclusive access for one thread
		boost::lock_guard<boost::mutex> lock(sync);

		// If the bonus system tree changes(state of a single node or the relations to each other) then
		// cache all bonus objects. Selector objects doesn't matter.
		if (cachedLast != treeChanged)
		{
			BonusList allBonuses;
			allBonuses.reserve(cachedBonuses.capacity()); //we assume we'll get about the same number of bonuses

			cachedBonuses.clear();
			cachedRequests.clear();

			getAllBonusesRec(allBonuses, Selector::all);
			limitBonuses(allBonuses, cachedBonuses);
			cachedBonuses.stackBonuses();

			cachedLast = treeChanged;
		}

		// If a bonus system request comes with a caching string then look up in the map if there are any
		// pre-calculated bonus results. Limiters can't be cached so they have to be calculated.
		if(!cachingStr.empty())
		{
			auto it = cachedRequests.find(cachingStr);
			if(it != cachedRequests.end())
			{
				//Cached list contains bonuses for our query with applied limiters
				return it->second;
			}
		}

		//We still don't have the bonuses (didn't returned them from cache)
		//Perform bonus selection
		auto ret = std::make_shared<BonusList>();
		cachedBonuses.getBonuses(*ret, selector, limit);

		// Save the results in the cache
		if(!cachingStr.empty())
			cachedRequests[cachingStr] = ret;

		return ret;
	}
	else
	{
		return getAllBonusesWithoutCaching(selector, limit, root);
	}
}

TConstBonusListPtr CBonusSystemNode::getAllBonusesWithoutCaching(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root) const
{
	auto ret = std::make_shared<BonusList>();

	// Get bonus results without caching enabled.
	BonusList beforeLimiting;
	BonusList afterLimiting;
	getAllBonusesRec(beforeLimiting, selector);

	if(!root || root == this)
	{
		limitBonuses(beforeLimiting, afterLimiting);
	}
	else if(root)
	{
		//We want to limit our query against an external node. We get all its bonuses,
		// add the ones we're considering and see if they're cut out by limiters
		BonusList rootBonuses;
		BonusList limitedRootBonuses;
		getAllBonusesRec(rootBonuses, selector);

		for(const auto & b : beforeLimiting)
			rootBonuses.push_back(b);

		root->limitBonuses(rootBonuses, limitedRootBonuses);

		for(const auto & b : beforeLimiting)
			if(vstd::contains(limitedRootBonuses, b))
				afterLimiting.push_back(b);

	}
	afterLimiting.getBonuses(*ret, selector, limit);
	ret->stackBonuses();
	return ret;
}

std::shared_ptr<Bonus> CBonusSystemNode::getUpdatedBonus(const std::shared_ptr<Bonus> & b, const TUpdaterPtr & updater) const
{
	assert(updater);
	return updater->createUpdatedBonus(b, * this);
}

CBonusSystemNode::CBonusSystemNode()
	:CBonusSystemNode(false)
{
}

CBonusSystemNode::CBonusSystemNode(bool isHypotetic):
	bonuses(true),
	exportedBonuses(true),
	nodeType(UNKNOWN),
	cachedLast(0),
	isHypotheticNode(isHypotetic)
{
}

CBonusSystemNode::CBonusSystemNode(ENodeTypes NodeType):
	bonuses(true),
	exportedBonuses(true),
	nodeType(NodeType),
	cachedLast(0),
	isHypotheticNode(false)
{
}

CBonusSystemNode::CBonusSystemNode(CBonusSystemNode && other) noexcept:
	bonuses(std::move(other.bonuses)),
	exportedBonuses(std::move(other.exportedBonuses)),
	nodeType(other.nodeType),
	description(other.description),
	cachedLast(0),
	isHypotheticNode(other.isHypotheticNode)
{
	std::swap(parents, other.parents);
	std::swap(children, other.children);

	//fixing bonus tree without recalculation

	if(!isHypothetic())
	{
		for(CBonusSystemNode * n : parents)
		{
			n->children -= &other;
			n->children.push_back(this);
		}
	}

	for(CBonusSystemNode * n : children)
	{
		n->parents -= &other;
		n->parents.push_back(this);
	}

	//cache ignored

	//cachedBonuses
	//cachedRequests
}

CBonusSystemNode::~CBonusSystemNode()
{
	detachFromAll();

	if(!children.empty())
	{
		while(!children.empty())
			children.front()->detachFrom(*this);
	}
}

void CBonusSystemNode::attachTo(CBonusSystemNode & parent)
{
	assert(!vstd::contains(parents, &parent));
	parents.push_back(&parent);

	if(!isHypothetic())
	{
		if(parent.actsAsBonusSourceOnly())
			parent.newRedDescendant(*this);
		else
			newRedDescendant(parent);

		parent.newChildAttached(*this);
	}

	CBonusSystemNode::treeHasChanged();
}

void CBonusSystemNode::detachFrom(CBonusSystemNode & parent)
{
	assert(vstd::contains(parents, &parent));

	if(!isHypothetic())
	{
		if(parent.actsAsBonusSourceOnly())
			parent.removedRedDescendant(*this);
		else
			removedRedDescendant(parent);
	}

	if (vstd::contains(parents, &parent))
	{
		parents -= &parent;
	}
	else
	{
		logBonus->error("Error on Detach. Node %s (nodeType=%d) has not parent %s (nodeType=%d)"
			, nodeShortInfo(), nodeType, parent.nodeShortInfo(), parent.nodeType);
	}

	if(!isHypothetic())
	{
		parent.childDetached(*this);
	}
	CBonusSystemNode::treeHasChanged();
}

void CBonusSystemNode::removeBonusesRecursive(const CSelector & s)
{
	removeBonuses(s);
	for(CBonusSystemNode * child : children)
		child->removeBonusesRecursive(s);
}

void CBonusSystemNode::reduceBonusDurations(const CSelector &s)
{
	BonusList bl;
	exportedBonuses.getBonuses(bl, s, Selector::all);
	for(const auto & b : bl)
	{
		b->turnsRemain--;
		if(b->turnsRemain <= 0)
			removeBonus(b);
	}

	for(CBonusSystemNode *child : children)
		child->reduceBonusDurations(s);
}

void CBonusSystemNode::addNewBonus(const std::shared_ptr<Bonus>& b)
{
	//turnsRemain shouldn't be zero for following durations
	if(Bonus::NTurns(b.get()) || Bonus::NDays(b.get()) || Bonus::OneWeek(b.get()))
	{
		assert(b->turnsRemain);
	}

	assert(!vstd::contains(exportedBonuses, b));
	exportedBonuses.push_back(b);
	exportBonus(b);
	CBonusSystemNode::treeHasChanged();
}

void CBonusSystemNode::accumulateBonus(const std::shared_ptr<Bonus>& b)
{
	auto bonus = exportedBonuses.getFirst(Selector::typeSubtype(b->type, b->subtype)); //only local bonuses are interesting //TODO: what about value type?
	if(bonus)
		bonus->val += b->val;
	else
		addNewBonus(std::make_shared<Bonus>(*b)); //duplicate needed, original may get destroyed
}

void CBonusSystemNode::removeBonus(const std::shared_ptr<Bonus>& b)
{
	exportedBonuses -= b;
	if(b->propagator)
		unpropagateBonus(b);
	else
		bonuses -= b;
	CBonusSystemNode::treeHasChanged();
}

void CBonusSystemNode::removeBonuses(const CSelector & selector)
{
	BonusList toRemove;
	exportedBonuses.getBonuses(toRemove, selector, Selector::all);
	for(const auto & bonus : toRemove)
		removeBonus(bonus);
}

bool CBonusSystemNode::actsAsBonusSourceOnly() const
{
	switch(nodeType)
	{
	case CREATURE:
	case ARTIFACT:
	case ARTIFACT_INSTANCE:
		return true;
	default:
		return false;
	}
}

void CBonusSystemNode::propagateBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & source)
{
	if(b->propagator->shouldBeAttached(this))
	{
		auto propagated = b->propagationUpdater 
			? source.getUpdatedBonus(b, b->propagationUpdater)
			: b;
		bonuses.push_back(propagated);
		logBonus->trace("#$# %s #propagated to# %s",  propagated->Description(), nodeName());
	}

	FOREACH_RED_CHILD(child)
		child->propagateBonus(b, source);
}

void CBonusSystemNode::unpropagateBonus(const std::shared_ptr<Bonus> & b)
{
	if(b->propagator->shouldBeAttached(this))
	{
		bonuses -= b;
		logBonus->trace("#$# %s #is no longer propagated to# %s",  b->Description(), nodeName());
	}

	FOREACH_RED_CHILD(child)
		child->unpropagateBonus(b);
}

void CBonusSystemNode::newChildAttached(CBonusSystemNode & child)
{
	assert(!vstd::contains(children, &child));
	children.push_back(&child);
}

void CBonusSystemNode::childDetached(CBonusSystemNode & child)
{
	if(vstd::contains(children, &child))
		children -= &child;
	else
	{
		logBonus->error("Error on Detach. Node %s (nodeType=%d) is not a child of %s (nodeType=%d)"
			, child.nodeShortInfo(), child.nodeType, nodeShortInfo(), nodeType);
	}
}

void CBonusSystemNode::detachFromAll()
{
	while(!parents.empty())
		detachFrom(*parents.front());
}

bool CBonusSystemNode::isIndependentNode() const
{
	return parents.empty() && children.empty();
}

std::string CBonusSystemNode::nodeName() const
{
	return !description.empty()
		? description
		: std::string("Bonus system node of type ") + typeid(*this).name();
}

std::string CBonusSystemNode::nodeShortInfo() const
{
	std::ostringstream str;
	str << "'" << typeid(* this).name() << "'";
	description.length() > 0 
		? str << " (" << description << ")"
		: str << " (no description)";
	return str.str();
}

void CBonusSystemNode::deserializationFix()
{
	exportBonuses();

}

void CBonusSystemNode::getRedParents(TNodes & out)
{
	FOREACH_PARENT(pname)
	{
		if(pname->actsAsBonusSourceOnly())
		{
			out.insert(pname);
		}
	}

	if(!actsAsBonusSourceOnly())
	{
		for(CBonusSystemNode *child : children)
		{
			out.insert(child);
		}
	}
}

void CBonusSystemNode::getRedChildren(TNodes &out)
{
	FOREACH_PARENT(pname)
	{
		if(!pname->actsAsBonusSourceOnly())
		{
			out.insert(pname);
		}
	}

	if(actsAsBonusSourceOnly())
	{
		for(CBonusSystemNode *child : children)
		{
			out.insert(child);
		}
	}
}

void CBonusSystemNode::newRedDescendant(CBonusSystemNode & descendant)
{
	for(const auto & b : exportedBonuses)
	{
		if(b->propagator)
			descendant.propagateBonus(b, *this);
	}
	TNodes redParents;
	getRedAncestors(redParents); //get all red parents recursively

	for(auto * parent : redParents)
	{
		for(const auto & b : parent->exportedBonuses)
		{
			if(b->propagator)
				descendant.propagateBonus(b, *this);
		}
	}
}

void CBonusSystemNode::removedRedDescendant(CBonusSystemNode & descendant)
{
	for(const auto & b : exportedBonuses)
		if(b->propagator)
			descendant.unpropagateBonus(b);

	TNodes redParents;
	getRedAncestors(redParents); //get all red parents recursively

	for(auto * parent : redParents)
	{
		for(const auto & b : parent->exportedBonuses)
			if(b->propagator)
				descendant.unpropagateBonus(b);
	}
}

void CBonusSystemNode::getRedAncestors(TNodes &out)
{
	getRedParents(out);

	TNodes redParents; 
	getRedParents(redParents);

	for(CBonusSystemNode * parent : redParents)
		parent->getRedAncestors(out);
}

void CBonusSystemNode::exportBonus(const std::shared_ptr<Bonus> & b)
{
	if(b->propagator)
		propagateBonus(b, *this);
	else
		bonuses.push_back(b);

	CBonusSystemNode::treeHasChanged();
}

void CBonusSystemNode::exportBonuses()
{
	for(const auto & b : exportedBonuses)
		exportBonus(b);
}

CBonusSystemNode::ENodeTypes CBonusSystemNode::getNodeType() const
{
	return nodeType;
}

const BonusList& CBonusSystemNode::getBonusList() const
{
	return bonuses;
}

const TNodesVector& CBonusSystemNode::getParentNodes() const
{
	return parents;
}

const TNodesVector& CBonusSystemNode::getChildrenNodes() const
{
	return children;
}

void CBonusSystemNode::setNodeType(CBonusSystemNode::ENodeTypes type)
{
	nodeType = type;
}

BonusList & CBonusSystemNode::getExportedBonusList()
{
	return exportedBonuses;
}

const BonusList & CBonusSystemNode::getExportedBonusList() const
{
	return exportedBonuses;
}

const std::string& CBonusSystemNode::getDescription() const
{
	return description;
}

void CBonusSystemNode::setDescription(const std::string &description)
{
	this->description = description;
}

void CBonusSystemNode::limitBonuses(const BonusList &allBonuses, BonusList &out) const
{
	assert(&allBonuses != &out); //todo should it work in-place?

	BonusList undecided = allBonuses;
	BonusList & accepted = out;

	while(true)
	{
		int undecidedCount = static_cast<int>(undecided.size());
		for(int i = 0; i < undecided.size(); i++)
		{
			auto b = undecided[i];
			BonusLimitationContext context = {*b, *this, out, undecided};
			auto decision = b->limiter ? b->limiter->limit(context) : ILimiter::EDecision::ACCEPT; //bonuses without limiters will be accepted by default
			if(decision == ILimiter::EDecision::DISCARD)
			{
				undecided.erase(i);
				i--; continue;
			}
			else if(decision == ILimiter::EDecision::ACCEPT)
			{
				accepted.push_back(b);
				undecided.erase(i);
				i--; continue;
			}
			else
				assert(decision == ILimiter::EDecision::NOT_SURE);
		}

		if(undecided.size() == undecidedCount) //we haven't moved a single bonus -> limiters reached a stable state
			return;
	}
}

TBonusListPtr CBonusSystemNode::limitBonuses(const BonusList &allBonuses) const
{
	auto ret = std::make_shared<BonusList>();
	limitBonuses(allBonuses, *ret);
	return ret;
}

void CBonusSystemNode::treeHasChanged()
{
	treeChanged++;
}

int64_t CBonusSystemNode::getTreeVersion() const
{
	return treeChanged;
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

BonusParams::BonusParams(std::string deprecatedTypeStr, std::string deprecatedSubtypeStr, int deprecatedSubtype):
	isConverted(true)
{
	if(deprecatedTypeStr == "SECONDARY_SKILL_PREMY" || deprecatedTypeStr == "SPECIAL_SECONDARY_SKILL")
	{
		if(deprecatedSubtype == SecondarySkill::PATHFINDING || deprecatedSubtypeStr == "skill.pathfinding")
			type = Bonus::ROUGH_TERRAIN_DISCOUNT;
		else if(deprecatedSubtype == SecondarySkill::DIPLOMACY || deprecatedSubtypeStr == "skill.diplomacy")
			type = Bonus::WANDERING_CREATURES_JOIN_BONUS;
		else if(deprecatedSubtype == SecondarySkill::WISDOM || deprecatedSubtypeStr == "skill.wisdom")
			type = Bonus::MAX_LEARNABLE_SPELL_LEVEL;
		else if(deprecatedSubtype == SecondarySkill::MYSTICISM || deprecatedSubtypeStr == "skill.mysticism")
			type = Bonus::MANA_REGENERATION;
		else if(deprecatedSubtype == SecondarySkill::NECROMANCY || deprecatedSubtypeStr == "skill.necromancy")
			type = Bonus::UNDEAD_RAISE_PERCENTAGE;
		else if(deprecatedSubtype == SecondarySkill::LEARNING || deprecatedSubtypeStr == "skill.learning")
			type = Bonus::HERO_EXPERIENCE_GAIN_PERCENT;
		else if(deprecatedSubtype == SecondarySkill::RESISTANCE || deprecatedSubtypeStr == "skill.resistance")
			type = Bonus::MAGIC_RESISTANCE;
		else if(deprecatedSubtype == SecondarySkill::EAGLE_EYE || deprecatedSubtypeStr == "skill.eagleEye")
			type = Bonus::LEARN_BATTLE_SPELL_CHANCE;
		else if(deprecatedSubtype == SecondarySkill::SCOUTING || deprecatedSubtypeStr == "skill.scouting")
			type = Bonus::SIGHT_RADIUS;
		else if(deprecatedSubtype == SecondarySkill::INTELLIGENCE || deprecatedSubtypeStr == "skill.intelligence")
		{
			type = Bonus::MANA_PER_KNOWLEDGE;
			valueType = Bonus::PERCENT_TO_BASE;
			valueTypeRelevant = true;
		}
		else if(deprecatedSubtype == SecondarySkill::SORCERY || deprecatedSubtypeStr == "skill.sorcery")
			type = Bonus::SPELL_DAMAGE;
		else if(deprecatedSubtype == SecondarySkill::SCHOLAR || deprecatedSubtypeStr == "skill.scholar")
			type = Bonus::LEARN_MEETING_SPELL_LIMIT;
		else if(deprecatedSubtype == SecondarySkill::ARCHERY|| deprecatedSubtypeStr == "skill.archery")
		{
			subtype = 1;
			subtypeRelevant = true;
			type = Bonus::PERCENTAGE_DAMAGE_BOOST;
		}
		else if(deprecatedSubtype == SecondarySkill::OFFENCE || deprecatedSubtypeStr == "skill.offence")
		{
			subtype = 0;
			subtypeRelevant = true;
			type = Bonus::PERCENTAGE_DAMAGE_BOOST;
		}
		else if(deprecatedSubtype == SecondarySkill::ARMORER || deprecatedSubtypeStr == "skill.armorer")
		{
			subtype = -1;
			subtypeRelevant = true;
			type = Bonus::GENERAL_DAMAGE_REDUCTION;
		}
		else if(deprecatedSubtype == SecondarySkill::NAVIGATION || deprecatedSubtypeStr == "skill.navigation")
		{
			subtype = 0;
			subtypeRelevant = true;
			valueType = Bonus::PERCENT_TO_BASE;
			valueTypeRelevant = true;
			type = Bonus::MOVEMENT;
		}
		else if(deprecatedSubtype == SecondarySkill::LOGISTICS || deprecatedSubtypeStr == "skill.logistics")
		{
			subtype = 1;
			subtypeRelevant = true;
			valueType = Bonus::PERCENT_TO_BASE;
			valueTypeRelevant = true;
			type = Bonus::MOVEMENT;
		}
		else if(deprecatedSubtype == SecondarySkill::ESTATES || deprecatedSubtypeStr == "skill.estates")
		{
			type = Bonus::GENERATE_RESOURCE;
			subtype = GameResID(EGameResID::GOLD);
			subtypeRelevant = true;
		}
		else if(deprecatedSubtype == SecondarySkill::AIR_MAGIC || deprecatedSubtypeStr == "skill.airMagic")
		{
			type = Bonus::MAGIC_SCHOOL_SKILL;
			subtypeRelevant = true;
			subtype = 4;
		}
		else if(deprecatedSubtype == SecondarySkill::WATER_MAGIC || deprecatedSubtypeStr == "skill.waterMagic")
		{
			type = Bonus::MAGIC_SCHOOL_SKILL;
			subtypeRelevant = true;
			subtype = 1;
		}
		else if(deprecatedSubtype == SecondarySkill::FIRE_MAGIC || deprecatedSubtypeStr == "skill.fireMagic")
		{
			type = Bonus::MAGIC_SCHOOL_SKILL;
			subtypeRelevant = true;
			subtype = 2;
		}
		else if(deprecatedSubtype == SecondarySkill::EARTH_MAGIC || deprecatedSubtypeStr == "skill.earthMagic")
		{
			type = Bonus::MAGIC_SCHOOL_SKILL;
			subtypeRelevant = true;
			subtype = 8;
		}
		else if (deprecatedSubtype == SecondarySkill::ARTILLERY || deprecatedSubtypeStr == "skill.artillery")
		{
			type = Bonus::BONUS_DAMAGE_CHANCE;
			subtypeRelevant = true;
			subtypeStr = "core:creature.ballista";
		}
		else if (deprecatedSubtype == SecondarySkill::FIRST_AID || deprecatedSubtypeStr == "skill.firstAid")
		{
			type = Bonus::SPECIFIC_SPELL_POWER;
			subtypeRelevant = true;
			subtypeStr = "core:spell.firstAid";
		}
		else if (deprecatedSubtype == SecondarySkill::BALLISTICS || deprecatedSubtypeStr == "skill.ballistics")
		{
			type = Bonus::CATAPULT_EXTRA_SHOTS;
			subtypeRelevant = true;
			subtypeStr = "core:spell.catapultShot";
		}
		else
			isConverted = false;
	}
	else if (deprecatedTypeStr == "SECONDARY_SKILL_VAL2")
	{
		if(deprecatedSubtype == SecondarySkill::EAGLE_EYE || deprecatedSubtypeStr == "skill.eagleEye")
			type = Bonus::LEARN_BATTLE_SPELL_LEVEL_LIMIT;
		else if (deprecatedSubtype == SecondarySkill::ARTILLERY || deprecatedSubtypeStr == "skill.artillery")
		{
			type = Bonus::HERO_GRANTS_ATTACKS;
			subtypeRelevant = true;
			subtypeStr = "core:creature.ballista";
		}
		else
			isConverted = false;
	}
	else if (deprecatedTypeStr == "SEA_MOVEMENT")
	{
		subtype = 0;
		subtypeRelevant = true;
		valueType = Bonus::ADDITIVE_VALUE;
		valueTypeRelevant = true;
		type = Bonus::MOVEMENT;
	}
	else if (deprecatedTypeStr == "LAND_MOVEMENT")
	{
		subtype = 1;
		subtypeRelevant = true;
		valueType = Bonus::ADDITIVE_VALUE;
		valueTypeRelevant = true;
		type = Bonus::MOVEMENT;
	}
	else if (deprecatedTypeStr == "MAXED_SPELL")
	{
		type = Bonus::SPELL;
		subtypeStr = deprecatedSubtypeStr;
		subtypeRelevant = true;
		valueType = Bonus::INDEPENDENT_MAX;
		valueTypeRelevant = true;
		val = 3;
		valRelevant = true;
	}
	else if (deprecatedTypeStr == "FULL_HP_REGENERATION")
	{
		type = Bonus::HP_REGENERATION;
		val = 100000; //very high value to always chose stack health
		valRelevant = true;
	}
	else if (deprecatedTypeStr == "KING1")
	{
		type = Bonus::KING;
		val = 0;
		valRelevant = true;
	}
	else if (deprecatedTypeStr == "KING2")
	{
		type = Bonus::KING;
		val = 2;
		valRelevant = true;
	}
	else if (deprecatedTypeStr == "KING3")
	{
		type = Bonus::KING;
		val = 3;
		valRelevant = true;
	}
	else if (deprecatedTypeStr == "SIGHT_RADIOUS")
		type = Bonus::SIGHT_RADIUS;
	else if (deprecatedTypeStr == "SELF_MORALE")
	{
		type = Bonus::MORALE;
		val = 1;
		valRelevant = true;
		valueType = Bonus::INDEPENDENT_MAX;
		valueTypeRelevant = true;
	}
	else if (deprecatedTypeStr == "SELF_LUCK")
	{
		type = Bonus::LUCK;
		val = 1;
		valRelevant = true;
		valueType = Bonus::INDEPENDENT_MAX;
		valueTypeRelevant = true;
	}
	else
		isConverted = false;
}

const JsonNode & BonusParams::toJson()
{
	assert(isConverted);
	if(ret.isNull())
	{
		ret["type"].String() = vstd::findKey(bonusNameMap, type);
		if(subtypeRelevant && !subtypeStr.empty())
			ret["subtype"].String() = subtypeStr;
		else if(subtypeRelevant)
			ret["subtype"].Integer() = subtype;
		if(valueTypeRelevant)
			ret["valueType"].String() = vstd::findKey(bonusValueMap, valueType);
		if(valRelevant)
			ret["val"].Float() = val;
		if(targetTypeRelevant)
			ret["targetSourceType"].String() = vstd::findKey(bonusSourceMap, targetType);
		jsonCreated = true;
	}
	return ret;
};

CSelector BonusParams::toSelector()
{
	assert(isConverted);
	if(subtypeRelevant && !subtypeStr.empty())
		JsonUtils::resolveIdentifier(subtype, toJson(), "subtype");

	auto ret = Selector::type()(type);
	if(subtypeRelevant)
		ret = ret.And(Selector::subtype()(subtype));
	if(valueTypeRelevant)
		ret = ret.And(Selector::valueType(valueType));
	if(targetTypeRelevant)
		ret = ret.And(Selector::targetSourceType()(targetType));
	return ret;
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

bool IPropagator::shouldBeAttached(CBonusSystemNode *dest)
{
	return false;
}

CBonusSystemNode::ENodeTypes IPropagator::getPropagatorType() const
{
	return CBonusSystemNode::ENodeTypes::NONE;
}

CPropagatorNodeType::CPropagatorNodeType(CBonusSystemNode::ENodeTypes NodeType)
	: nodeType(NodeType)
{
}

CBonusSystemNode::ENodeTypes CPropagatorNodeType::getPropagatorType() const
{
	return nodeType;
}

bool CPropagatorNodeType::shouldBeAttached(CBonusSystemNode *dest)
{
	return nodeType == dest->getNodeType();
}

// Updaters

std::shared_ptr<Bonus> Bonus::addUpdater(const TUpdaterPtr & Updater)
{
	updater = Updater;
	return this->shared_from_this();
}

VCMI_LIB_NAMESPACE_END
