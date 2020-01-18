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

#include "VCMI_Lib.h"
#include "spells/CSpellHandler.h"
#include "CCreatureHandler.h"
#include "CCreatureSet.h"
#include "CHeroHandler.h"
#include "CTownHandler.h"
#include "CGeneralTextHandler.h"
#include "CSkillHandler.h"
#include "CStack.h"
#include "CArtHandler.h"
#include "StringConstants.h"
#include "battle/BattleInfo.h"

#define FOREACH_PARENT(pname) 	TNodes lparents; getParents(lparents); for(CBonusSystemNode *pname : lparents)
#define FOREACH_CPARENT(pname) 	TCNodes lparents; getParents(lparents); for(const CBonusSystemNode *pname : lparents)
#define FOREACH_RED_CHILD(pname) 	TNodes lchildren; getRedChildren(lchildren); for(CBonusSystemNode *pname : lchildren)
#define FOREACH_RED_PARENT(pname) 	TNodes lparents; getRedParents(lparents); for(CBonusSystemNode *pname : lparents)

#define BONUS_NAME(x) { #x, Bonus::x },
	const std::map<std::string, Bonus::BonusType> bonusNameMap = { BONUS_LIST };
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
	BONUS_ITEM(ONLY_ENEMY_ARMY)
};

const std::map<std::string, TLimiterPtr> bonusLimiterMap =
{
	{"SHOOTER_ONLY", std::make_shared<HasAnotherBonusLimiter>(Bonus::SHOOTER)},
	{"DRAGON_NATURE", std::make_shared<HasAnotherBonusLimiter>(Bonus::DRAGON_NATURE)},
	{"IS_UNDEAD", std::make_shared<HasAnotherBonusLimiter>(Bonus::UNDEAD)},
	{"CREATURE_NATIVE_TERRAIN", std::make_shared<CreatureTerrainLimiter>()}
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

const std::map<std::string, TUpdaterPtr> bonusUpdaterMap =
{
	{"TIMES_HERO_LEVEL", std::make_shared<TimesHeroLevelUpdater>()},
	{"TIMES_STACK_LEVEL", std::make_shared<TimesStackLevelUpdater>()}
};

///CBonusProxy
CBonusProxy::CBonusProxy(const IBonusBearer * Target, CSelector Selector)
	: cachedLast(0),
	target(Target),
	selector(Selector),
	data()
{

}

CBonusProxy::CBonusProxy(const CBonusProxy & other)
	: cachedLast(other.cachedLast),
	target(other.target),
	selector(other.selector),
	data(other.data)
{

}

CBonusProxy::CBonusProxy(CBonusProxy && other)
	: cachedLast(0),
	target(other.target),
	selector(),
	data()
{
	std::swap(cachedLast, other.cachedLast);
	std::swap(selector, other.selector);
	std::swap(data, other.data);
}

CBonusProxy & CBonusProxy::operator=(const CBonusProxy & other)
{
	cachedLast = other.cachedLast;
	selector = other.selector;
	data = other.data;
	return *this;
}

CBonusProxy & CBonusProxy::operator=(CBonusProxy && other)
{
	std::swap(cachedLast, other.cachedLast);
	std::swap(selector, other.selector);
	std::swap(data, other.data);
	return *this;
}

TBonusListPtr CBonusProxy::get() const
{
	if(target->getTreeVersion() != cachedLast || !data)
	{
		//TODO: support limiters
		data = target->getAllBonuses(selector, Selector::all);
		cachedLast = target->getTreeVersion();
	}
	return data;
}

const BonusList * CBonusProxy::operator->() const
{
	return get().get();
}

CAddInfo::CAddInfo()
{
}

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

std::atomic<int32_t> CBonusSystemNode::treeChanged(1);
const bool CBonusSystemNode::cachingEnabled = true;

BonusList::BonusList(bool BelongsToTree) : belongsToTree(BelongsToTree)
{

}

BonusList::BonusList(const BonusList &bonusList)
{
	bonuses.resize(bonusList.size());
	std::copy(bonusList.begin(), bonusList.end(), bonuses.begin());
	belongsToTree = false;
}

BonusList::BonusList(BonusList&& other):
	belongsToTree(false)
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

void BonusList::changed()
{
    if(belongsToTree)
		CBonusSystemNode::treeHasChanged();
}

void BonusList::stackBonuses()
{
	boost::sort(bonuses, [](std::shared_ptr<Bonus> b1, std::shared_ptr<Bonus> b2) -> bool
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
		bool remove;
		const std::shared_ptr<Bonus> last = bonuses[next-1];
		const std::shared_ptr<Bonus> current = bonuses[next];

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
	int base = 0;
	int percentToBase = 0;
	int percentToAll = 0;
	int additive = 0;
	int indepMax = 0;
	bool hasIndepMax = false;
	int indepMin = 0;
	bool hasIndepMin = false;

	for(std::shared_ptr<Bonus> b : bonuses)
	{
		switch(b->valType)
		{
		case Bonus::BASE_NUMBER:
			base += b->val;
			break;
		case Bonus::PERCENT_TO_ALL:
			percentToAll += b->val;
			break;
		case Bonus::PERCENT_TO_BASE:
			percentToBase += b->val;
			break;
		case Bonus::ADDITIVE_VALUE:
			additive += b->val;
			break;
		case Bonus::INDEPENDENT_MAX:
			if (!hasIndepMax)
			{
				indepMax = b->val;
				hasIndepMax = true;
			}
			else
			{
				vstd::amax(indepMax, b->val);
			}

			break;
		case Bonus::INDEPENDENT_MIN:
			if (!hasIndepMin)
			{
				indepMin = b->val;
				hasIndepMin = true;
			}
			else
			{
				vstd::amin(indepMin, b->val);
			}

			break;
		}
	}
	int modifiedBase = base + (base * percentToBase) / 100;
	modifiedBase += additive;
	int valFirst = (modifiedBase * (100 + percentToAll)) / 100;

	if(hasIndepMin && hasIndepMax)
		assert(indepMin < indepMax);

	const int notIndepBonuses = boost::count_if(bonuses, [](const std::shared_ptr<Bonus>& b)
	{
		return b->valType != Bonus::INDEPENDENT_MAX && b->valType != Bonus::INDEPENDENT_MIN;
	});

	if (hasIndepMax)
	{
		if(notIndepBonuses)
			vstd::amax(valFirst, indepMax);
		else
			valFirst = indepMax;
	}
	if (hasIndepMin)
	{
		if(notIndepBonuses)
			vstd::amin(valFirst, indepMin);
		else
			valFirst = indepMin;
	}

	return valFirst;
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

const std::shared_ptr<Bonus> BonusList::getFirst(const CSelector &selector) const
{
	for (auto & b : bonuses)
	{
		if(selector(b.get()))
			return b;
	}
	return nullptr;
}

void BonusList::getBonuses(BonusList & out, const CSelector &selector) const
{
	getBonuses(out, selector, nullptr);
}

void BonusList::getBonuses(BonusList & out, const CSelector &selector, const CSelector &limit) const
{
	for (auto & b : bonuses)
	{
		//add matching bonuses that matches limit predicate or have NO_LIMIT if no given predicate
		if(selector(b.get()) && ((!limit && b->effectRange == Bonus::NO_LIMIT) || ((bool)limit && limit(b.get()))))
			out.push_back(b);
	}
}

void BonusList::getAllBonuses(BonusList &out) const
{
	for(auto & b : bonuses)
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
	for(std::shared_ptr<Bonus> b : bonuses)
		node.Vector().push_back(b->toJsonNode());
	return node;
}

void BonusList::push_back(std::shared_ptr<Bonus> x)
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

std::vector<BonusList*>::size_type BonusList::operator-=(std::shared_ptr<Bonus> const &i)
{
	auto itr = std::find(bonuses.begin(), bonuses.end(), i);
	if(itr == bonuses.end())
		return false;
	bonuses.erase(itr);
	changed();
	return true;
}

void BonusList::resize(BonusList::TInternalContainer::size_type sz, std::shared_ptr<Bonus> c )
{
	bonuses.resize(sz, c);
	changed();
}

void BonusList::insert(BonusList::TInternalContainer::iterator position, BonusList::TInternalContainer::size_type n, std::shared_ptr<Bonus> const &x)
{
	bonuses.insert(position, n, x);
	changed();
}

int IBonusBearer::valOfBonuses(Bonus::BonusType type, const CSelector &selector) const
{
	return valOfBonuses(Selector::type(type).And(selector));
}

int IBonusBearer::valOfBonuses(Bonus::BonusType type, int subtype) const
{
	boost::format fmt("type_%ds_%d");
	fmt % (int)type % subtype;

	CSelector s = Selector::type(type);
	if(subtype != -1)
		s = s.And(Selector::subtype(subtype));

	return valOfBonuses(s, fmt.str());
}

int IBonusBearer::valOfBonuses(const CSelector &selector, const std::string &cachingStr) const
{
	CSelector limit = nullptr;
	TBonusListPtr hlp = getAllBonuses(selector, limit, nullptr, cachingStr);
	return hlp->totalValue();
}
bool IBonusBearer::hasBonus(const CSelector &selector, const std::string &cachingStr) const
{
	return getBonuses(selector, cachingStr)->size() > 0;
}

bool IBonusBearer::hasBonus(const CSelector &selector, const CSelector &limit, const std::string &cachingStr) const
{
	return getBonuses(selector, limit, cachingStr)->size() > 0;
}

bool IBonusBearer::hasBonusOfType(Bonus::BonusType type, int subtype) const
{
	boost::format fmt("type_%ds_%d");
	fmt % (int)type % subtype;

	CSelector s = Selector::type(type);
	if(subtype != -1)
		s = s.And(Selector::subtype(subtype));

	return hasBonus(s, fmt.str());
}

const TBonusListPtr IBonusBearer::getBonuses(const CSelector &selector, const std::string &cachingStr) const
{
	return getAllBonuses(selector, nullptr, nullptr, cachingStr);
}

const TBonusListPtr IBonusBearer::getBonuses(const CSelector &selector, const CSelector &limit, const std::string &cachingStr) const
{
	return getAllBonuses(selector, limit, nullptr, cachingStr);
}

bool IBonusBearer::hasBonusFrom(Bonus::BonusSource source, ui32 sourceID) const
{
	boost::format fmt("source_%did_%d");
	fmt % (int)source % sourceID;

	return hasBonus(Selector::source(source,sourceID), fmt.str());
}

int IBonusBearer::MoraleVal() const
{
	if(hasBonusOfType(Bonus::NON_LIVING) || hasBonusOfType(Bonus::UNDEAD) ||
		hasBonusOfType(Bonus::NO_MORALE) || hasBonusOfType(Bonus::SIEGE_WEAPON))
		return 0;

	int ret = valOfBonuses(Bonus::MORALE);

	if(hasBonusOfType(Bonus::SELF_MORALE)) //eg. minotaur
		vstd::amax(ret, +1);

	return vstd::abetween(ret, -3, +3);
}

int IBonusBearer::LuckVal() const
{
	if(hasBonusOfType(Bonus::NO_LUCK))
		return 0;

	int ret = valOfBonuses(Bonus::LUCK);

	if(hasBonusOfType(Bonus::SELF_LUCK)) //eg. halfling
		vstd::amax(ret, +1);

	return vstd::abetween(ret, -3, +3);
}

ui32 IBonusBearer::MaxHealth() const
{
	const std::string cachingStr = "type_STACK_HEALTH";
	static const auto selector = Selector::type(Bonus::STACK_HEALTH);
	auto value = valOfBonuses(selector, cachingStr);
	return std::max(1, value); //never 0
}

int IBonusBearer::getAttack(bool ranged) const
{
	const std::string cachingStr = "type_PRIMARY_SKILLs_ATTACK";

	static const auto selector = Selector::typeSubtype(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK);

	return getBonuses(selector, nullptr, cachingStr)->totalValue();
}

int IBonusBearer::getDefence(bool ranged) const
{
	const std::string cachingStr = "type_PRIMARY_SKILLs_DEFENSE";

	static const auto selector = Selector::typeSubtype(Bonus::PRIMARY_SKILL, PrimarySkill::DEFENSE);

	return getBonuses(selector, nullptr, cachingStr)->totalValue();
}

int IBonusBearer::getMinDamage(bool ranged) const
{
	const std::string cachingStr = "type_CREATURE_DAMAGEs_0Otype_CREATURE_DAMAGEs_1";
	static const auto selector = Selector::typeSubtype(Bonus::CREATURE_DAMAGE, 0).Or(Selector::typeSubtype(Bonus::CREATURE_DAMAGE, 1));
	return valOfBonuses(selector, cachingStr);
}

int IBonusBearer::getMaxDamage(bool ranged) const
{
	const std::string cachingStr = "type_CREATURE_DAMAGEs_0Otype_CREATURE_DAMAGEs_2";
	static const auto selector = Selector::typeSubtype(Bonus::CREATURE_DAMAGE, 0).Or(Selector::typeSubtype(Bonus::CREATURE_DAMAGE, 2));
	return valOfBonuses(selector, cachingStr);
}

si32 IBonusBearer::manaLimit() const
{
	return si32(getPrimSkillLevel(PrimarySkill::KNOWLEDGE)
		* (100.0 + valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, SecondarySkill::INTELLIGENCE))
		/ 10.0);
}

int IBonusBearer::getPrimSkillLevel(PrimarySkill::PrimarySkill id) const
{
	static const CSelector selectorAllSkills = Selector::type(Bonus::PRIMARY_SKILL);
	static const std::string keyAllSkills = "type_PRIMARY_SKILL";

	auto allSkills = getBonuses(selectorAllSkills, keyAllSkills);

	int ret = allSkills->valOfBonuses(Selector::subtype(id));

	vstd::amax(ret, id/2); //minimal value is 0 for attack and defense and 1 for spell power and knowledge
	return ret;
}

si32 IBonusBearer::magicResistance() const
{
	return valOfBonuses(Bonus::MAGIC_RESISTANCE);
}

ui32 IBonusBearer::Speed(int turn, bool useBind) const
{
	//war machines cannot move
	if(hasBonus(Selector::type(Bonus::SIEGE_WEAPON).And(Selector::turns(turn))))
	{
		return 0;
	}
	//bind effect check - doesn't influence stack initiative
	if(useBind && hasBonus(Selector::type(Bonus::BIND_EFFECT).And(Selector::turns(turn))))
	{
		return 0;
	}

	return valOfBonuses(Selector::type(Bonus::STACKS_SPEED).And(Selector::turns(turn)));
}

bool IBonusBearer::isLiving() const //TODO: theoreticaly there exists "LIVING" bonus in stack experience documentation
{
	static const std::string cachingStr = "IBonusBearer::isLiving";
	static const CSelector selector = Selector::type(Bonus::UNDEAD)
		.Or(Selector::type(Bonus::NON_LIVING))
		.Or(Selector::type(Bonus::GARGOYLE))
		.Or(Selector::type(Bonus::SIEGE_WEAPON));

	return !hasBonus(selector, cachingStr);
}

const std::shared_ptr<Bonus> IBonusBearer::getBonus(const CSelector &selector) const
{
	auto bonuses = getAllBonuses(selector, Selector::all);
	return bonuses->getFirst(Selector::all);
}

std::shared_ptr<Bonus> CBonusSystemNode::getBonusLocalFirst(const CSelector &selector)
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

const std::shared_ptr<Bonus> CBonusSystemNode::getBonusLocalFirst( const CSelector &selector ) const
{
	return (const_cast<CBonusSystemNode*>(this))->getBonusLocalFirst(selector);
}

void CBonusSystemNode::getParents(TCNodes & out) const /*retrieves list of parent nodes (nodes to inherit bonuses from) */
{
	for (auto & elem : parents)
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

void CBonusSystemNode::getBonusesRec(BonusList &out, const CSelector &selector, const CSelector &limit) const
{
	BonusList beforeUpdate;
	FOREACH_CPARENT(p)
	{
		p->getBonusesRec(beforeUpdate, selector, limit);
	}
	bonuses.getBonuses(beforeUpdate, selector, limit);

	for(auto b : beforeUpdate)
		out.push_back(update(b));
}

void CBonusSystemNode::getAllBonusesRec(BonusList &out) const
{
	BonusList beforeUpdate;
	FOREACH_CPARENT(p)
	{
		p->getAllBonusesRec(beforeUpdate);
	}
	bonuses.getAllBonuses(beforeUpdate);

	for(auto b : beforeUpdate)
		out.push_back(update(b));
}

const TBonusListPtr CBonusSystemNode::getAllBonuses(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root, const std::string &cachingStr) const
{
	bool limitOnUs = (!root || root == this); //caching won't work when we want to limit bonuses against an external node
	if (CBonusSystemNode::cachingEnabled && limitOnUs)
	{
		// Exclusive access for one thread
		static boost::mutex m;
		boost::mutex::scoped_lock lock(m);

		// If the bonus system tree changes(state of a single node or the relations to each other) then
		// cache all bonus objects. Selector objects doesn't matter.
		if (cachedLast != treeChanged)
		{
			cachedBonuses.clear();
			cachedRequests.clear();

			BonusList allBonuses;
			getAllBonusesRec(allBonuses);
			limitBonuses(allBonuses, cachedBonuses);
			cachedBonuses.stackBonuses();

			cachedLast = treeChanged;
		}

		// If a bonus system request comes with a caching string then look up in the map if there are any
		// pre-calculated bonus results. Limiters can't be cached so they have to be calculated.
		if (cachingStr != "")
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
		if(cachingStr != "")
			cachedRequests[cachingStr] = ret;

		return ret;
	}
	else
	{
		return getAllBonusesWithoutCaching(selector, limit, root);
	}
}

const TBonusListPtr CBonusSystemNode::getAllBonusesWithoutCaching(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root) const
{
	auto ret = std::make_shared<BonusList>();

	// Get bonus results without caching enabled.
	BonusList beforeLimiting, afterLimiting;
	getAllBonusesRec(beforeLimiting);

	if(!root || root == this)
	{
		limitBonuses(beforeLimiting, afterLimiting);
	}
	else if(root)
	{
		//We want to limit our query against an external node. We get all its bonuses,
		// add the ones we're considering and see if they're cut out by limiters
		BonusList rootBonuses, limitedRootBonuses;
		getAllBonusesRec(rootBonuses);

		for(auto b : beforeLimiting)
			rootBonuses.push_back(b);

		root->limitBonuses(rootBonuses, limitedRootBonuses);

		for(auto b : beforeLimiting)
			if(vstd::contains(limitedRootBonuses, b))
				afterLimiting.push_back(b);

	}
	afterLimiting.getBonuses(*ret, selector, limit);
	ret->stackBonuses();
	return ret;
}

const std::shared_ptr<Bonus> CBonusSystemNode::update(const std::shared_ptr<Bonus> b) const
{
	if(b->updater)
		return b->updater->update(b, *this);
	return b;
}

CBonusSystemNode::CBonusSystemNode()
	: bonuses(true),
	exportedBonuses(true),
	nodeType(UNKNOWN),
	cachedLast(0)
{
}

CBonusSystemNode::CBonusSystemNode(ENodeTypes NodeType)
	: bonuses(true),
	exportedBonuses(true),
	nodeType(NodeType),
	cachedLast(0)
{
}

CBonusSystemNode::CBonusSystemNode(CBonusSystemNode && other):
	bonuses(std::move(other.bonuses)),
	exportedBonuses(std::move(other.exportedBonuses)),
	nodeType(other.nodeType),
	description(other.description),
	cachedLast(0)
{
	std::swap(parents, other.parents);
	std::swap(children, other.children);

	//fixing bonus tree without recalculation

	for(CBonusSystemNode * n : parents)
	{
		n->children -= &other;
		n->children.push_back(this);
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

	if(children.size())
	{
		while(children.size())
			children.front()->detachFrom(this);
	}
}

void CBonusSystemNode::attachTo(CBonusSystemNode *parent)
{
	assert(!vstd::contains(parents, parent));
	parents.push_back(parent);

	if(parent->actsAsBonusSourceOnly())
		parent->newRedDescendant(this);
	else
		newRedDescendant(parent);

	parent->newChildAttached(this);
	CBonusSystemNode::treeHasChanged();
}

void CBonusSystemNode::detachFrom(CBonusSystemNode *parent)
{
	assert(vstd::contains(parents, parent));

	if(parent->actsAsBonusSourceOnly())
		parent->removedRedDescendant(this);
	else
		removedRedDescendant(parent);

	parents -= parent;
	parent->childDetached(this);
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
	for(auto b : bl)
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
	for(auto bonus : toRemove)
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

void CBonusSystemNode::propagateBonus(std::shared_ptr<Bonus> b)
{
	if(b->propagator->shouldBeAttached(this))
	{
		bonuses.push_back(b);
		logBonus->trace("#$# %s #propagated to# %s",  b->Description(), nodeName());
	}

	FOREACH_RED_CHILD(child)
		child->propagateBonus(b);
}

void CBonusSystemNode::unpropagateBonus(std::shared_ptr<Bonus> b)
{
	if(b->propagator->shouldBeAttached(this))
	{
		bonuses -= b;
		logBonus->trace("#$# %s #is no longer propagated to# %s",  b->Description(), nodeName());
	}

	FOREACH_RED_CHILD(child)
		child->unpropagateBonus(b);
}

void CBonusSystemNode::newChildAttached(CBonusSystemNode *child)
{
	assert(!vstd::contains(children, child));
	children.push_back(child);
}

void CBonusSystemNode::childDetached(CBonusSystemNode *child)
{
	if (vstd::contains(children, child))
		children -= child;
	else
	{
		logBonus->error("Error! %s #cannot be detached from# %s", child->nodeName(), nodeName());
		throw std::runtime_error("internal error");
	}

}

void CBonusSystemNode::detachFromAll()
{
	while(parents.size())
		detachFrom(parents.front());
}

bool CBonusSystemNode::isIndependentNode() const
{
	return parents.empty() && children.empty();
}

std::string CBonusSystemNode::nodeName() const
{
	return description.size()
		? description
		: std::string("Bonus system node of type ") + typeid(*this).name();
}

void CBonusSystemNode::deserializationFix()
{
	exportBonuses();

}

void CBonusSystemNode::getRedParents(TNodes &out)
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

void CBonusSystemNode::newRedDescendant(CBonusSystemNode *descendant)
{
	for(auto b : exportedBonuses)
		if(b->propagator)
			descendant->propagateBonus(b);

	FOREACH_RED_PARENT(parent)
		parent->newRedDescendant(descendant);
}

void CBonusSystemNode::removedRedDescendant(CBonusSystemNode *descendant)
{
	for(auto b : exportedBonuses)
		if(b->propagator)
			descendant->unpropagateBonus(b);

	FOREACH_RED_PARENT(parent)
		parent->removedRedDescendant(descendant);
}

void CBonusSystemNode::getRedAncestors(TNodes &out)
{
	getRedParents(out);
	FOREACH_RED_PARENT(p)
		p->getRedAncestors(out);
}

void CBonusSystemNode::getRedDescendants(TNodes &out)
{
	getRedChildren(out);
	FOREACH_RED_CHILD(c)
		c->getRedChildren(out);
}

void CBonusSystemNode::exportBonus(std::shared_ptr<Bonus> b)
{
	if(b->propagator)
		propagateBonus(b);
	else
		bonuses.push_back(b);

	CBonusSystemNode::treeHasChanged();
}

void CBonusSystemNode::exportBonuses()
{
	for(auto b : exportedBonuses)
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

BonusList& CBonusSystemNode::getExportedBonusList()
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

	BonusList undecided = allBonuses,
		&accepted = out;

	while(true)
	{
		int undecidedCount = undecided.size();
		for(int i = 0; i < undecided.size(); i++)
		{
			auto b = undecided[i];
			BonusLimitationContext context = {b, *this, out, undecided};
			int decision = b->limiter ? b->limiter->limit(context) : ILimiter::ACCEPT; //bonuses without limiters will be accepted by default
			if(decision == ILimiter::DISCARD)
			{
				undecided.erase(i);
				i--; continue;
			}
			else if(decision == ILimiter::ACCEPT)
			{
				accepted.push_back(b);
				undecided.erase(i);
				i--; continue;
			}
			else
				assert(decision == ILimiter::NOT_SURE);
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
	int64_t ret = treeChanged;
	return ret << 32;
}

int NBonus::valOf(const CBonusSystemNode *obj, Bonus::BonusType type, int subtype)
{
	if(obj)
		return obj->valOfBonuses(type, subtype);
	return 0;
}

bool NBonus::hasOfType(const CBonusSystemNode *obj, Bonus::BonusType type, int subtype)
{
	if(obj)
		return obj->hasBonusOfType(type, subtype);
	return false;
}

std::string Bonus::Description() const
{
	std::ostringstream str;

	if(description.empty())
	{
		if(stacking.empty() || stacking == "ALWAYS")
		{
			switch(source)
			{
			case ARTIFACT:
				str << VLC->arth->artifacts[sid]->Name();
				break;
			case SPELL_EFFECT:
				str << SpellID(sid).toSpell()->name;
				break;
			case CREATURE_ABILITY:
				str << VLC->creh->creatures[sid]->namePl;
				break;
			case SECONDARY_SKILL:
				str << VLC->skillh->skillName(sid);
				break;
			case HERO_SPECIAL:
				str << VLC->heroh->heroes[sid]->name;
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

	if(val != 0)
		str << " " << std::showpos << val;

	return str.str();
}

JsonNode subtypeToJson(Bonus::BonusType type, int subtype)
{
	switch(type)
	{
	case Bonus::PRIMARY_SKILL:
		return JsonUtils::stringNode("primSkill." + PrimarySkill::names[subtype]);
	case Bonus::SECONDARY_SKILL_PREMY:
		return JsonUtils::stringNode(CSkillHandler::encodeSkillWithType(subtype));
	case Bonus::SPECIAL_SPELL_LEV:
	case Bonus::SPECIFIC_SPELL_DAMAGE:
	case Bonus::SPECIAL_BLESS_DAMAGE:
	case Bonus::MAXED_SPELL:
	case Bonus::SPECIAL_PECULIAR_ENCHANT:
		return JsonUtils::stringNode("spell." + (*VLC->spellh)[SpellID::ESpellID(subtype)]->identifier);
	case Bonus::IMPROVED_NECROMANCY:
	case Bonus::SPECIAL_UPGRADE:
		return JsonUtils::stringNode("creature." + CreatureID::encode(subtype));
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
		return JsonUtils::stringNode("creature." + CreatureID::encode(addInfo[0]));
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
		for(std::string dur : durationNames)
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
	if(val != 0)
		root["val"].Integer() = val;
	if(valType != ADDITIVE_VALUE)
		root["valueType"].String() = vstd::findKey(bonusValueMap, valType);
	if(stacking != "")
		root["stacking"].String() = stacking;
	if(description != "")
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
	case Bonus::SECONDARY_SKILL_PREMY:
		return CSkillHandler::encodeSkill(subtype);
	case Bonus::SPECIAL_SPELL_LEV:
	case Bonus::SPECIFIC_SPELL_DAMAGE:
	case Bonus::SPECIAL_BLESS_DAMAGE:
	case Bonus::MAXED_SPELL:
	case Bonus::SPECIAL_PECULIAR_ENCHANT:
		return (*VLC->spellh)[SpellID::ESpellID(subtype)]->identifier;
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

Bonus::Bonus(ui16 Dur, BonusType Type, BonusSource Src, si32 Val, ui32 ID, std::string Desc, si32 Subtype)
	: duration(Dur), type(Type), subtype(Subtype), source(Src), val(Val), sid(ID), description(Desc)
{
	turnsRemain = 0;
	valType = ADDITIVE_VALUE;
	effectRange = NO_LIMIT;
	boost::algorithm::trim(description);
}

Bonus::Bonus(ui16 Dur, BonusType Type, BonusSource Src, si32 Val, ui32 ID, si32 Subtype, ValueType ValType)
	: duration(Dur), type(Type), subtype(Subtype), source(Src), val(Val), sid(ID), valType(ValType)
{
	turnsRemain = 0;
	effectRange = NO_LIMIT;
}

Bonus::Bonus()
{
	duration = PERMANENT;
	turnsRemain = 0;
	type = NONE;
	subtype = -1;

	valType = ADDITIVE_VALUE;
	effectRange = NO_LIMIT;
	val = 0;
	source = OTHER;
	sid = 0;
}

std::shared_ptr<Bonus> Bonus::addPropagator(TPropagatorPtr Propagator)
{
	propagator = Propagator;
	return this->shared_from_this();
}

namespace Selector
{
	DLL_LINKAGE CSelectFieldEqual<Bonus::BonusType> type(&Bonus::type);
	DLL_LINKAGE CSelectFieldEqual<TBonusSubtype> subtype(&Bonus::subtype);
	DLL_LINKAGE CSelectFieldEqual<CAddInfo> info(&Bonus::additionalInfo);
	DLL_LINKAGE CSelectFieldEqual<Bonus::BonusSource> sourceType(&Bonus::source);
	DLL_LINKAGE CSelectFieldEqual<Bonus::LimitEffect> effectRange(&Bonus::effectRange);
	DLL_LINKAGE CWillLastTurns turns;
	DLL_LINKAGE CWillLastDays days;

	CSelector DLL_LINKAGE typeSubtype(Bonus::BonusType Type, TBonusSubtype Subtype)
	{
		return type(Type).And(subtype(Subtype));
	}

	CSelector DLL_LINKAGE typeSubtypeInfo(Bonus::BonusType type, TBonusSubtype subtype, CAddInfo info)
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

	bool DLL_LINKAGE matchesType(const CSelector &sel, Bonus::BonusType type)
	{
		Bonus dummy;
		dummy.type = type;
		return sel(&dummy);
	}

	bool DLL_LINKAGE matchesTypeSubtype(const CSelector &sel, Bonus::BonusType type, TBonusSubtype subtype)
	{
		Bonus dummy;
		dummy.type = type;
		dummy.subtype = subtype;
		return sel(&dummy);
	}
}

const CStack * retrieveStackBattle(const CBonusSystemNode * node)
{
	switch(node->getNodeType())
	{
	case CBonusSystemNode::STACK_BATTLE:
		return static_cast<const CStack*>(node);
	default:
		return nullptr;
	}
}

const CStackInstance * retrieveStackInstance(const CBonusSystemNode * node)
{
	switch(node->getNodeType())
	{
	case CBonusSystemNode::STACK_INSTANCE:
		return (static_cast<const CStackInstance *>(node));
	case CBonusSystemNode::STACK_BATTLE:
		return (static_cast<const CStack*>(node))->base;
	default:
		return nullptr;
	}
}

const CCreature * retrieveCreature(const CBonusSystemNode *node)
{
	switch(node->getNodeType())
	{
	case CBonusSystemNode::CREATURE:
		return (static_cast<const CCreature *>(node));
	case CBonusSystemNode::STACK_BATTLE:
		return (static_cast<const CStack*>(node))->type;
	default:
		const CStackInstance * csi = retrieveStackInstance(node);
		if(csi)
			return csi->type;
		return nullptr;
	}
}

DLL_LINKAGE std::ostream & operator<<(std::ostream &out, const BonusList &bonusList)
{
	for (ui32 i = 0; i < bonusList.size(); i++)
	{
		auto b = bonusList[i];
		out << "Bonus " << i << "\n" << *b << std::endl;
	}
	return out;
}

DLL_LINKAGE std::ostream & operator<<(std::ostream &out, const Bonus &bonus)
{
	for(auto i = bonusNameMap.cbegin(); i != bonusNameMap.cend(); i++)
		if(i->second == bonus.type)
			out << "\tType: " << i->first << " \t";

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

std::shared_ptr<Bonus> Bonus::addLimiter(TLimiterPtr Limiter)
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

ILimiter::~ILimiter()
{
}

int ILimiter::limit(const BonusLimitationContext &context) const /*return true to drop the bonus */
{
	return false;
}

std::string ILimiter::toString() const
{
	return typeid(*this).name();
}

JsonNode ILimiter::toJsonNode() const
{
	JsonNode root(JsonNode::JsonType::DATA_STRUCT);
	root["type"].String() = toString();
	return root;
}

int CCreatureTypeLimiter::limit(const BonusLimitationContext &context) const
{
	const CCreature *c = retrieveCreature(&context.node);
	if(!c)
		return true;
	return c != creature   &&   (!includeUpgrades || !creature->isMyUpgrade(c));
	//drop bonus if it's not our creature and (we don`t check upgrades or its not our upgrade)
}

CCreatureTypeLimiter::CCreatureTypeLimiter(const CCreature &Creature, bool IncludeUpgrades)
	:creature(&Creature), includeUpgrades(IncludeUpgrades)
{
}

CCreatureTypeLimiter::CCreatureTypeLimiter()
{
	creature = nullptr;
	includeUpgrades = false;
}

void CCreatureTypeLimiter::setCreature (CreatureID id)
{
	creature = VLC->creh->creatures[id];
}

std::string CCreatureTypeLimiter::toString() const
{
	boost::format fmt("CCreatureTypeLimiter(creature=%s, includeUpgrades=%s)");
	fmt % creature->identifier % (includeUpgrades ? "true" : "false");
	return fmt.str();
}

JsonNode CCreatureTypeLimiter::toJsonNode() const
{
	JsonNode root(JsonNode::JsonType::DATA_STRUCT);

	root["type"].String() = "CREATURE_TYPE_LIMITER";
	root["parameters"].Vector().push_back(JsonUtils::stringNode(creature->identifier));
	root["parameters"].Vector().push_back(JsonUtils::boolNode(includeUpgrades));

	return root;
}

HasAnotherBonusLimiter::HasAnotherBonusLimiter( Bonus::BonusType bonus )
	: type(bonus), subtype(0), isSubtypeRelevant(false)
{
}

HasAnotherBonusLimiter::HasAnotherBonusLimiter( Bonus::BonusType bonus, TBonusSubtype _subtype )
	: type(bonus), subtype(_subtype), isSubtypeRelevant(true)
{
}

int HasAnotherBonusLimiter::limit(const BonusLimitationContext &context) const
{
	CSelector mySelector = isSubtypeRelevant
							? Selector::typeSubtype(type, subtype)
							: Selector::type(type);

	//if we have a bonus of required type accepted, limiter should accept also this bonus
	if(context.alreadyAccepted.getFirst(mySelector))
		return ACCEPT;

	//if there are no matching bonuses pending, we can (and must) reject right away
	if(!context.stillUndecided.getFirst(mySelector))
		return DISCARD;

	//do not accept for now but it may change if more bonuses gets included
	return NOT_SURE;
}

std::string HasAnotherBonusLimiter::toString() const
{
	std::string typeName = vstd::findKey(bonusNameMap, type);
	if(isSubtypeRelevant)
	{
		boost::format fmt("HasAnotherBonusLimiter(type=%s, subtype=%d)");
		fmt % typeName % subtype;
		return fmt.str();
	}
	else
	{
		boost::format fmt("HasAnotherBonusLimiter(type=%s)");
		fmt % typeName;
		return fmt.str();
	}
}

JsonNode HasAnotherBonusLimiter::toJsonNode() const
{
	JsonNode root(JsonNode::JsonType::DATA_STRUCT);
	std::string typeName = vstd::findKey(bonusNameMap, type);

	root["type"].String() = "HAS_ANOTHER_BONUS_LIMITER";
	root["parameters"].Vector().push_back(JsonUtils::stringNode(typeName));
	if(isSubtypeRelevant)
		root["parameters"].Vector().push_back(JsonUtils::intNode(subtype));

	return root;
}

IPropagator::~IPropagator()
{

}

bool IPropagator::shouldBeAttached(CBonusSystemNode *dest)
{
	return false;
}

CPropagatorNodeType::CPropagatorNodeType()
	:nodeType(0)
{

}

CPropagatorNodeType::CPropagatorNodeType(int NodeType)
	: nodeType(NodeType)
{
}

bool CPropagatorNodeType::shouldBeAttached(CBonusSystemNode *dest)
{
	return nodeType == dest->getNodeType();
}

CreatureTerrainLimiter::CreatureTerrainLimiter(int TerrainType)
	: terrainType(TerrainType)
{
}

CreatureTerrainLimiter::CreatureTerrainLimiter()
	: terrainType(-1)
{

}

int CreatureTerrainLimiter::limit(const BonusLimitationContext &context) const
{
	const CStack *stack = retrieveStackBattle(&context.node);
	if(stack)
	{
		if(terrainType == -1)//terrainType not specified = native
			return !stack->isOnNativeTerrain();
		return !stack->isOnTerrain(terrainType);
	}
	return true;
	//TODO neutral creatues
}

std::string CreatureTerrainLimiter::toString() const
{
	boost::format fmt("CreatureTerrainLimiter(terrainType=%s)");
	fmt % (terrainType >= 0 ? GameConstants::TERRAIN_NAMES[terrainType] : "native");
	return fmt.str();
}

JsonNode CreatureTerrainLimiter::toJsonNode() const
{
	JsonNode root(JsonNode::JsonType::DATA_STRUCT);

	root["type"].String() = "CREATURE_TERRAIN_LIMITER";
	if(terrainType >= 0)
		root["parameters"].Vector().push_back(JsonUtils::stringNode(GameConstants::TERRAIN_NAMES[terrainType]));

	return root;
}

CreatureFactionLimiter::CreatureFactionLimiter(int Faction)
	: faction(Faction)
{
}

CreatureFactionLimiter::CreatureFactionLimiter()
	: faction(-1)
{
}

int CreatureFactionLimiter::limit(const BonusLimitationContext &context) const
{
	const CCreature *c = retrieveCreature(&context.node);
	return !c || c->faction != faction; //drop bonus for non-creatures or non-native residents
}

std::string CreatureFactionLimiter::toString() const
{
	boost::format fmt("CreatureFactionLimiter(faction=%s)");
	fmt %  VLC->townh->factions[faction]->identifier;
	return fmt.str();
}

JsonNode CreatureFactionLimiter::toJsonNode() const
{
	JsonNode root(JsonNode::JsonType::DATA_STRUCT);

	root["type"].String() = "CREATURE_FACTION_LIMITER";
	root["parameters"].Vector().push_back(JsonUtils::stringNode(VLC->townh->factions[faction]->identifier));

	return root;
}

CreatureAlignmentLimiter::CreatureAlignmentLimiter()
	: alignment(-1)
{
}

CreatureAlignmentLimiter::CreatureAlignmentLimiter(si8 Alignment)
	: alignment(Alignment)
{
}

int CreatureAlignmentLimiter::limit(const BonusLimitationContext &context) const
{
	const CCreature *c = retrieveCreature(&context.node);
	if(!c)
		return true;
	switch(alignment)
	{
	case EAlignment::GOOD:
		return !c->isGood(); //if not good -> return true (drop bonus)
	case EAlignment::NEUTRAL:
		return c->isEvil() || c->isGood();
	case EAlignment::EVIL:
		return !c->isEvil();
	default:
		logBonus->warn("Warning: illegal alignment in limiter!");
		return true;
	}
}

std::string CreatureAlignmentLimiter::toString() const
{
	boost::format fmt("CreatureAlignmentLimiter(alignment=%s)");
	fmt % EAlignment::names[alignment];
	return fmt.str();
}

JsonNode CreatureAlignmentLimiter::toJsonNode() const
{
	JsonNode root(JsonNode::JsonType::DATA_STRUCT);

	root["type"].String() = "CREATURE_ALIGNMENT_LIMITER";
	root["parameters"].Vector().push_back(JsonUtils::stringNode(EAlignment::names[alignment]));

	return root;
}

RankRangeLimiter::RankRangeLimiter(ui8 Min, ui8 Max)
	:minRank(Min), maxRank(Max)
{
}

RankRangeLimiter::RankRangeLimiter()
{
	minRank = maxRank = -1;
}

int RankRangeLimiter::limit(const BonusLimitationContext &context) const
{
	const CStackInstance * csi = retrieveStackInstance(&context.node);
	if(csi)
	{
		if (csi->getNodeType() == CBonusSystemNode::COMMANDER) //no stack exp bonuses for commander creatures
			return true;
		return csi->getExpRank() < minRank || csi->getExpRank() > maxRank;
	}
	return true;
}

int StackOwnerLimiter::limit(const BonusLimitationContext &context) const
{
	const CStack * s = retrieveStackBattle(&context.node);
	if(s)
		return s->owner != owner;

	const CStackInstance * csi = retrieveStackInstance(&context.node);
	if(csi && csi->armyObj)
		return csi->armyObj->tempOwner != owner;
	return true;
}

StackOwnerLimiter::StackOwnerLimiter()
	: owner(-1)
{
}

StackOwnerLimiter::StackOwnerLimiter(PlayerColor Owner)
	: owner(Owner)
{
}

// Aggregate/Boolean Limiters

void AggregateLimiter::add(TLimiterPtr limiter)
{
	if(limiter)
		limiters.push_back(limiter);
}

JsonNode AggregateLimiter::toJsonNode() const
{
	JsonNode result(JsonNode::JsonType::DATA_VECTOR);
	result.Vector().push_back(JsonUtils::stringNode(getAggregator()));
	for(auto l : limiters)
		result.Vector().push_back(l->toJsonNode());
	return result;
}

const std::string AllOfLimiter::aggregator = "allOf";
const std::string & AllOfLimiter::getAggregator() const
{
	return aggregator;
}

int AllOfLimiter::limit(const BonusLimitationContext & context) const
{
	bool wasntSure = false;

	for(auto limiter : limiters)
	{
		auto result = limiter->limit(context);
		if(result == ILimiter::DISCARD)
			return result;
		if(result == ILimiter::NOT_SURE)
			wasntSure = true;
	}

	return wasntSure ? ILimiter::NOT_SURE : ILimiter::ACCEPT;
}

const std::string AnyOfLimiter::aggregator = "anyOf";
const std::string & AnyOfLimiter::getAggregator() const
{
	return aggregator;
}

int AnyOfLimiter::limit(const BonusLimitationContext & context) const
{
	bool wasntSure = false;

	for(auto limiter : limiters)
	{
		auto result = limiter->limit(context);
		if(result == ILimiter::ACCEPT)
			return result;
		if(result == ILimiter::NOT_SURE)
			wasntSure = true;
	}

	return wasntSure ? ILimiter::NOT_SURE : ILimiter::DISCARD;
}

const std::string NoneOfLimiter::aggregator = "noneOf";
const std::string & NoneOfLimiter::getAggregator() const
{
	return aggregator;
}

int NoneOfLimiter::limit(const BonusLimitationContext & context) const
{
	bool wasntSure = false;

	for(auto limiter : limiters)
	{
		auto result = limiter->limit(context);
		if(result == ILimiter::ACCEPT)
			return ILimiter::DISCARD;
		if(result == ILimiter::NOT_SURE)
			wasntSure = true;
	}

	return wasntSure ? ILimiter::NOT_SURE : ILimiter::ACCEPT;
}

// Updaters

std::shared_ptr<Bonus> Bonus::addUpdater(TUpdaterPtr Updater)
{
	updater = Updater;
	return this->shared_from_this();
}

IUpdater::~IUpdater()
{
}

const std::shared_ptr<Bonus> IUpdater::update(const std::shared_ptr<Bonus> b, const CBonusSystemNode & context) const
{
	return b;
}

std::string IUpdater::toString() const
{
	return typeid(*this).name();
}

JsonNode IUpdater::toJsonNode() const
{
	return JsonNode(JsonNode::JsonType::DATA_NULL);
}

GrowsWithLevelUpdater::GrowsWithLevelUpdater() : valPer20(0), stepSize(1)
{
}

GrowsWithLevelUpdater::GrowsWithLevelUpdater(int valPer20, int stepSize) : valPer20(valPer20), stepSize(stepSize)
{
}

const std::shared_ptr<Bonus> GrowsWithLevelUpdater::update(const std::shared_ptr<Bonus> b, const CBonusSystemNode & context) const
{
	if(context.getNodeType() == CBonusSystemNode::HERO)
	{
		int level = static_cast<const CGHeroInstance &>(context).level;
		int steps = stepSize ? level / stepSize : level;
		//rounding follows format for HMM3 creature specialty bonus
		int newVal = (valPer20 * steps + 19) / 20;
		//return copy of bonus with updated val
		std::shared_ptr<Bonus> newBonus = std::make_shared<Bonus>(*b);
		newBonus->val = newVal;
		return newBonus;
	}
	return b;
}

std::string GrowsWithLevelUpdater::toString() const
{
	return boost::str(boost::format("GrowsWithLevelUpdater(valPer20=%d, stepSize=%d)") % valPer20 % stepSize);
}

JsonNode GrowsWithLevelUpdater::toJsonNode() const
{
	JsonNode root(JsonNode::JsonType::DATA_STRUCT);

	root["type"].String() = "GROWS_WITH_LEVEL";
	root["parameters"].Vector().push_back(JsonUtils::intNode(valPer20));
	if(stepSize > 1)
		root["parameters"].Vector().push_back(JsonUtils::intNode(stepSize));

	return root;
}

TimesHeroLevelUpdater::TimesHeroLevelUpdater()
{
}

const std::shared_ptr<Bonus> TimesHeroLevelUpdater::update(const std::shared_ptr<Bonus> b, const CBonusSystemNode & context) const
{
	if(context.getNodeType() == CBonusSystemNode::HERO)
	{
		int level = static_cast<const CGHeroInstance &>(context).level;
		std::shared_ptr<Bonus> newBonus = std::make_shared<Bonus>(*b);
		newBonus->val *= level;
		return newBonus;
	}
	return b;
}

std::string TimesHeroLevelUpdater::toString() const
{
	return "TimesHeroLevelUpdater";
}

JsonNode TimesHeroLevelUpdater::toJsonNode() const
{
	return JsonUtils::stringNode("TIMES_HERO_LEVEL");
}

TimesStackLevelUpdater::TimesStackLevelUpdater()
{
}

const std::shared_ptr<Bonus> TimesStackLevelUpdater::update(const std::shared_ptr<Bonus> b, const CBonusSystemNode & context) const
{
	if(context.getNodeType() == CBonusSystemNode::STACK_INSTANCE)
	{
		int level = static_cast<const CStackInstance &>(context).getLevel();
		std::shared_ptr<Bonus> newBonus = std::make_shared<Bonus>(*b);
		newBonus->val *= level;
		return newBonus;
	}
	else if(context.getNodeType() == CBonusSystemNode::STACK_BATTLE)
	{
		const CStack & stack = static_cast<const CStack &>(context);
		//only update if stack doesn't have an instance (summons, war machines)
		//otherwise we'd end up multiplying twice
		if(stack.base == nullptr)
		{
			int level = stack.type->level;
			std::shared_ptr<Bonus> newBonus = std::make_shared<Bonus>(*b);
			newBonus->val *= level;
			return newBonus;
		}
	}
	return b;
}

std::string TimesStackLevelUpdater::toString() const
{
	return "TimesStackLevelUpdater";
}

JsonNode TimesStackLevelUpdater::toJsonNode() const
{
	return JsonUtils::stringNode("TIMES_STACK_LEVEL");
}
