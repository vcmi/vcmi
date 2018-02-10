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
#include "CGeneralTextHandler.h"
#include "CSkillHandler.h"
#include "CStack.h"
#include "CArtHandler.h"

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
	{"IS_UNDEAD", std::make_shared<HasAnotherBonusLimiter>(Bonus::UNDEAD)}
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
		data->eliminateDuplicates();
		cachedLast = target->getTreeVersion();
	}
	return data;
}

const BonusList * CBonusProxy::operator->() const
{
	return get().get();
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

	for (auto& b : bonuses)
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
	ret.eliminateDuplicates();
	return ret.totalValue();
}

void BonusList::eliminateDuplicates()
{
	sort( bonuses.begin(), bonuses.end() );
	bonuses.erase( unique( bonuses.begin(), bonuses.end() ), bonuses.end() );
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
	std::stringstream cachingStr;
	cachingStr << "type_" << type << "s_" << subtype;

	CSelector s = Selector::type(type);
	if(subtype != -1)
		s = s.And(Selector::subtype(subtype));

	return valOfBonuses(s, cachingStr.str());
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
	std::stringstream cachingStr;
	cachingStr << "type_" << type << "s_" << subtype;

	CSelector s = Selector::type(type);
	if(subtype != -1)
		s = s.And(Selector::subtype(subtype));

	return hasBonus(s, cachingStr.str());
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
	std::stringstream cachingStr;
	cachingStr << "source_" << source << "id_" << sourceID;
	return hasBonus(Selector::source(source,sourceID), cachingStr.str());
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
	int ret = valOfBonuses(Bonus::PRIMARY_SKILL, id);

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
	std::stringstream cachingStr;
	cachingStr << "type_" << Bonus::UNDEAD << "s_-1Otype_" << Bonus::NON_LIVING << "s_-11type_" << Bonus::SIEGE_WEAPON; //I don't really get what string labels mean?
	return !hasBonus(Selector::type(Bonus::UNDEAD)
					.Or(Selector::type(Bonus::NON_LIVING))
					.Or(Selector::type(Bonus::SIEGE_WEAPON)), cachingStr.str());
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
	FOREACH_CPARENT(p)
	{
		p->getBonusesRec(out, selector, limit);
	}

	bonuses.getBonuses(out, selector, limit);
}

void CBonusSystemNode::getAllBonusesRec(BonusList &out) const
{
	FOREACH_CPARENT(p)
	{
		p->getAllBonusesRec(out);
	}

	bonuses.getAllBonuses(out);
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
			allBonuses.eliminateDuplicates();
			limitBonuses(allBonuses, cachedBonuses);

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
	beforeLimiting.eliminateDuplicates();

	if(!root || root == this)
	{
		limitBonuses(beforeLimiting, afterLimiting);
		afterLimiting.getBonuses(*ret, selector, limit);
	}
	else if(root)
	{
		//We want to limit our query against an external node. We get all its bonuses,
		// add the ones we're considering and see if they're cut out by limiters
		BonusList rootBonuses, limitedRootBonuses;
		getAllBonusesRec(rootBonuses);

		for(auto b : beforeLimiting)
			rootBonuses.push_back(b);

		rootBonuses.eliminateDuplicates();
		root->limitBonuses(rootBonuses, limitedRootBonuses);

		for(auto b : beforeLimiting)
			if(vstd::contains(limitedRootBonuses, b))
				afterLimiting.push_back(b);

		afterLimiting.getBonuses(*ret, selector, limit);
	}
	return ret;
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

void CBonusSystemNode::popBonuses(const CSelector &s)
{
	BonusList bl;
	exportedBonuses.getBonuses(bl, s, Selector::all);
	for(auto b : bl)
		removeBonus(b);

	for(CBonusSystemNode *child : children)
		child->popBonuses(s);
}

void CBonusSystemNode::updateBonuses(const CSelector &s)
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
		child->updateBonuses(s);
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
		while(vstd::contains(bonuses, b))
		{
			logBonus->error("Bonus was duplicated (%s) at %s", b->Description(), nodeName());
			bonuses -= b;
		}
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
			BonusLimitationContext context = {b, *this, out};
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
		default:
			//todo: handle all possible sources
			str << "Unknown";
			break;
		}
	else
		str << description;

	if(val != 0)
		str << " " << std::showpos << val;

	return str.str();
}

Bonus::Bonus(ui16 Dur, BonusType Type, BonusSource Src, si32 Val, ui32 ID, std::string Desc, si32 Subtype)
	: duration(Dur), type(Type), subtype(Subtype), source(Src), val(Val), sid(ID), description(Desc)
{
	additionalInfo = -1;
	turnsRemain = 0;
	valType = ADDITIVE_VALUE;
	effectRange = NO_LIMIT;
	boost::algorithm::trim(description);
}

Bonus::Bonus(ui16 Dur, BonusType Type, BonusSource Src, si32 Val, ui32 ID, si32 Subtype, ValueType ValType)
	: duration(Dur), type(Type), subtype(Subtype), source(Src), val(Val), sid(ID), valType(ValType)
{
	additionalInfo = -1;
	turnsRemain = 0;
	effectRange = NO_LIMIT;
}

Bonus::Bonus()
{
	duration = PERMANENT;
	turnsRemain = 0;
	type = NONE;
	subtype = -1;
	additionalInfo = -1;

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
	DLL_LINKAGE CSelectFieldEqual<si32> info(&Bonus::additionalInfo);
	DLL_LINKAGE CSelectFieldEqual<Bonus::BonusSource> sourceType(&Bonus::source);
	DLL_LINKAGE CSelectFieldEqual<Bonus::LimitEffect> effectRange(&Bonus::effectRange);
	DLL_LINKAGE CWillLastTurns turns;
	DLL_LINKAGE CWillLastDays days;

	CSelector DLL_LINKAGE typeSubtype(Bonus::BonusType Type, TBonusSubtype Subtype)
	{
		return type(Type).And(subtype(Subtype));
	}

	CSelector DLL_LINKAGE typeSubtypeInfo(Bonus::BonusType type, TBonusSubtype subtype, si32 info)
	{
		return CSelectFieldEqual<Bonus::BonusType>(&Bonus::type)(type)
			.And(CSelectFieldEqual<TBonusSubtype>(&Bonus::subtype)(subtype))
			.And(CSelectFieldEqual<si32>(&Bonus::additionalInfo)(info));
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
	printField(additionalInfo);
	printField(turnsRemain);
	printField(valType);
	printField(effectRange);
#undef printField

	return out;
}

std::shared_ptr<Bonus> Bonus::addLimiter(TLimiterPtr Limiter)
{
	if (limiter)
	{
		//If we already have limiter list, retrieve it
		auto limiterList = std::dynamic_pointer_cast<LimiterList>(limiter);
		if(!limiterList)
		{
			//Create a new limiter list with old limiter and the new one will be pushed later
			limiterList = std::make_shared<LimiterList>();
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

	//do not accept for now but it may change if more bonuses gets included
	return NOT_SURE;
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

CreatureNativeTerrainLimiter::CreatureNativeTerrainLimiter(int TerrainType)
	: terrainType(TerrainType)
{
}

CreatureNativeTerrainLimiter::CreatureNativeTerrainLimiter()
	: terrainType(-1)
{

}

int CreatureNativeTerrainLimiter::limit(const BonusLimitationContext &context) const
{
	const CCreature *c = retrieveCreature(&context.node);
	return !c || !c->isItNativeTerrain(terrainType); //drop bonus for non-creatures or non-native residents
	//TODO neutral creatues
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

int LimiterList::limit( const BonusLimitationContext &context ) const
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

void LimiterList::add( TLimiterPtr limiter )
{
	limiters.push_back(limiter);
}
