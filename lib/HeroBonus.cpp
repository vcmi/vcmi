#include "StdInc.h"
#include "HeroBonus.h"

#include "VCMI_Lib.h"
#include "CSpellHandler.h"
#include "CCreatureHandler.h"
#include "CCreatureSet.h"
#include "CHeroHandler.h"
#include "CGeneralTextHandler.h"
#include "BattleState.h"
#include "CArtHandler.h"
#include "GameConstants.h"

#define FOREACH_PARENT(pname) 	TNodes lparents; getParents(lparents); BOOST_FOREACH(CBonusSystemNode *pname, lparents)
#define FOREACH_CPARENT(pname) 	TCNodes lparents; getParents(lparents); BOOST_FOREACH(const CBonusSystemNode *pname, lparents)
#define FOREACH_RED_CHILD(pname) 	TNodes lchildren; getRedChildren(lchildren); BOOST_FOREACH(CBonusSystemNode *pname, lchildren)
#define FOREACH_RED_PARENT(pname) 	TNodes lparents; getRedParents(lparents); BOOST_FOREACH(CBonusSystemNode *pname, lparents)

#define BONUS_NAME(x) ( #x, Bonus::x )
	const std::map<std::string, int> bonusNameMap = boost::assign::map_list_of BONUS_LIST;
#undef BONUS_NAME

#define BONUS_VALUE(x) ( #x, Bonus::x )
	const std::map<std::string, int> bonusValueMap = boost::assign::map_list_of BONUS_VALUE_LIST;
#undef BONUS_VALUE

#define BONUS_SOURCE(x) ( #x, Bonus::x )
	const std::map<std::string, int> bonusSourceMap = boost::assign::map_list_of BONUS_SOURCE_LIST;
#undef BONUS_SOURCE

#define BONUS_ITEM(x) ( #x, Bonus::x )

const std::map<std::string, int> bonusDurationMap = boost::assign::map_list_of 
	BONUS_ITEM(PERMANENT)
	BONUS_ITEM(ONE_BATTLE)
	BONUS_ITEM(ONE_DAY)
	BONUS_ITEM(ONE_WEEK)
	BONUS_ITEM(N_TURNS)
	BONUS_ITEM(N_DAYS)
	BONUS_ITEM(UNITL_BEING_ATTACKED)
	BONUS_ITEM(UNTIL_ATTACK)
	BONUS_ITEM(STACK_GETS_TURN)
	BONUS_ITEM(COMMANDER_KILLED);

const std::map<std::string, int> bonusLimitEffect = boost::assign::map_list_of
	BONUS_ITEM(NO_LIMIT)
	BONUS_ITEM(ONLY_DISTANCE_FIGHT)
	BONUS_ITEM(ONLY_MELEE_FIGHT)
	BONUS_ITEM(ONLY_ENEMY_ARMY);

const bmap<std::string, TLimiterPtr> bonusLimiterMap = boost::assign::map_list_of
	("SHOOTER_ONLY", make_shared<HasAnotherBonusLimiter>(Bonus::SHOOTER))
	("DRAGON_NATURE", make_shared<HasAnotherBonusLimiter>(Bonus::DRAGON_NATURE))
	("IS_UNDEAD", make_shared<HasAnotherBonusLimiter>(Bonus::UNDEAD));

const bmap<std::string, TPropagatorPtr> bonusPropagatorMap = boost::assign::map_list_of
	("BATTLE_WIDE", make_shared<CPropagatorNodeType>(CBonusSystemNode::BATTLE))
	("VISITED_TOWN_AND_VISITOR", make_shared<CPropagatorNodeType>(CBonusSystemNode::TOWN_AND_VISITOR));


#define BONUS_LOG_LINE(x) tlog5 << x << std::endl

int CBonusSystemNode::treeChanged = 1;
const bool CBonusSystemNode::cachingEnabled = true;

BonusList::BonusList(bool BelongsToTree /* =false */) : belongsToTree(BelongsToTree)
{

}

BonusList::BonusList(const BonusList &bonusList)
{
	bonuses.resize(bonusList.size());
	std::copy(bonusList.begin(), bonusList.end(), bonuses.begin());
	belongsToTree = false;
}

BonusList& BonusList::operator=(const BonusList &bonusList)
{
	bonuses.resize(bonusList.size());
	std::copy(bonusList.begin(), bonusList.end(), bonuses.begin());
	belongsToTree = false;
	return *this;
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

	for (size_t i = 0; i < bonuses.size(); i++)
	{
		Bonus *b = bonuses[i];

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

	const int notIndepBonuses = boost::count_if(bonuses, [](const Bonus *b) 
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
const Bonus * BonusList::getFirst(const CSelector &selector) const
{
	for (ui32 i = 0; i < bonuses.size(); i++)
	{
		const Bonus *b = bonuses[i];
		if(selector(b))
			return &*b;
	}
	return NULL;
}

Bonus * BonusList::getFirst(const CSelector &select)
{
	for (ui32 i = 0; i < bonuses.size(); i++)
	{
		Bonus *b = bonuses[i];
		if(select(b))
			return &*b;
	}
	return NULL;
}

void BonusList::getModifiersWDescr(TModDescr &out) const
{
	for (size_t i = 0; i < bonuses.size(); i++)
	{
		Bonus *b = bonuses[i];
		out.push_back(std::make_pair(b->val, b->Description()));
	}
}

void BonusList::getBonuses(BonusList & out, const CSelector &selector) const
{
// 	BOOST_FOREACH(Bonus *i, *this)
// 		if(selector(i) && i->effectRange == Bonus::NO_LIMIT)
// 			out.push_back(i);

	getBonuses(out, selector, 0);
}

void BonusList::getBonuses(BonusList & out, const CSelector &selector, const CSelector &limit) const
{
	for (ui32 i = 0; i < bonuses.size(); i++)
	{
		Bonus *b = bonuses[i];

		//add matching bonuses that matches limit predicate or have NO_LIMIT if no given predicate
		if(selector(b) && ((!limit && b->effectRange == Bonus::NO_LIMIT) || (limit && limit(b))))
			out.push_back(b);
	}
}

void BonusList::getAllBonuses(BonusList &out) const
{
	BOOST_FOREACH(Bonus *b, bonuses)
		out.push_back(b);
}

int BonusList::valOfBonuses(const CSelector &select) const
{
	BonusList ret;
	CSelector limit = 0;
	getBonuses(ret, select, limit);
	ret.eliminateDuplicates();
	return ret.totalValue();
}

// void BonusList::limit(const CBonusSystemNode &node)
// {
// 	remove_if(boost::bind(&CBonusSystemNode::isLimitedOnUs, boost::ref(node), _1));
// }


void BonusList::eliminateDuplicates()
{
	sort( bonuses.begin(), bonuses.end() );
	bonuses.erase( unique( bonuses.begin(), bonuses.end() ), bonuses.end() );
}

void BonusList::push_back(Bonus* const &x)
{
	bonuses.push_back(x);
	
	if (belongsToTree)
		CBonusSystemNode::incrementTreeChangedNum();
}

std::vector<Bonus*>::iterator BonusList::erase(const int position)
{
	if (belongsToTree)
		CBonusSystemNode::incrementTreeChangedNum();
	return bonuses.erase(bonuses.begin() + position);
}

void BonusList::clear()
{
	bonuses.clear();

	if (belongsToTree)
		CBonusSystemNode::incrementTreeChangedNum();
}

std::vector<BonusList*>::size_type BonusList::operator-=(Bonus* const &i)
{
	std::vector<Bonus*>::iterator itr = std::find(bonuses.begin(), bonuses.end(), i);
	if(itr == bonuses.end())
		return false;
	bonuses.erase(itr);

	if (belongsToTree)
		CBonusSystemNode::incrementTreeChangedNum();
	return true;
}

void BonusList::resize(std::vector<Bonus*>::size_type sz, Bonus* c )
{
	bonuses.resize(sz, c);

	if (belongsToTree)
		CBonusSystemNode::incrementTreeChangedNum();
}

void BonusList::insert(std::vector<Bonus*>::iterator position, std::vector<Bonus*>::size_type n, Bonus* const &x)
{
	bonuses.insert(position, n, x);

	if (belongsToTree)
		CBonusSystemNode::incrementTreeChangedNum();
}

int IBonusBearer::valOfBonuses(Bonus::BonusType type, const CSelector &selector) const
{
	return valOfBonuses(Selector::type(type) && selector);
}

int IBonusBearer::valOfBonuses(Bonus::BonusType type, int subtype /*= -1*/) const
{
	std::stringstream cachingStr;
	cachingStr << "type_" << type << "s_" << subtype;
	
	CSelector s = Selector::type(type);
	if(subtype != -1)
		s = s && Selector::subtype(subtype);

	return valOfBonuses(s, cachingStr.str());
}

int IBonusBearer::valOfBonuses(const CSelector &selector, const std::string &cachingStr) const
{
	CSelector limit = 0;
	TBonusListPtr hlp = getAllBonuses(selector, limit, NULL, cachingStr);
	return hlp->totalValue();
}
bool IBonusBearer::hasBonus(const CSelector &selector, const std::string &cachingStr /*= ""*/) const
{
	return getBonuses(selector, cachingStr)->size() > 0;
}

bool IBonusBearer::hasBonusOfType(Bonus::BonusType type, int subtype /*= -1*/) const
{
	std::stringstream cachingStr;
	cachingStr << "type_" << type << "s_" << subtype;

	CSelector s = Selector::type(type);
	if(subtype != -1)
		s = s && Selector::subtype(subtype);

	return hasBonus(s, cachingStr.str());
}

void IBonusBearer::getModifiersWDescr(TModDescr &out, Bonus::BonusType type, int subtype /*= -1 */) const
{
	std::stringstream cachingStr;
	cachingStr << "type_" << type << "s_" << subtype;
	getModifiersWDescr(out, subtype != -1 ? Selector::typeSubtype(type, subtype) : Selector::type(type), cachingStr.str());
}

void IBonusBearer::getModifiersWDescr(TModDescr &out, const CSelector &selector, const std::string &cachingStr /* =""*/) const
{
	getBonuses(selector, cachingStr)->getModifiersWDescr(out);
}
int IBonusBearer::getBonusesCount(int from, int id) const
{
	std::stringstream cachingStr;
	cachingStr << "source_" << from << "id_" << id;
	return getBonusesCount(Selector::source(from, id), cachingStr.str());
}

int IBonusBearer::getBonusesCount(const CSelector &selector, const std::string &cachingStr /* =""*/) const
{
	return getBonuses(selector, cachingStr)->size();
}

const TBonusListPtr IBonusBearer::getBonuses(const CSelector &selector, const std::string &cachingStr /*= ""*/) const
{
	return getAllBonuses(selector, 0, NULL, cachingStr);
}

const TBonusListPtr IBonusBearer::getBonuses(const CSelector &selector, const CSelector &limit, const std::string &cachingStr /*= ""*/) const
{
	return getAllBonuses(selector, limit, NULL, cachingStr);
}

bool IBonusBearer::hasBonusFrom(ui8 source, ui32 sourceID) const
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

si32 IBonusBearer::Attack() const
{
	si32 ret = valOfBonuses(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK);

	if (int frenzyPower = valOfBonuses(Bonus::IN_FRENZY)) //frenzy for attacker
	{
		ret += frenzyPower * Defense(false);
	}
	vstd::amax(ret, 0);

	return ret;
}

si32 IBonusBearer::Defense(bool withFrenzy /*= true*/) const
{
	si32 ret = valOfBonuses(Bonus::PRIMARY_SKILL, PrimarySkill::DEFENSE);

	if(withFrenzy && hasBonusOfType(Bonus::IN_FRENZY)) //frenzy for defender
	{
		return 0;
	}
	vstd::amax(ret, 0);

	return ret;
}

ui32 IBonusBearer::MaxHealth() const
{
	return std::max(1, valOfBonuses(Bonus::STACK_HEALTH)); //never 0
}

ui32 IBonusBearer::getMinDamage() const
{
	std::stringstream cachingStr;
	cachingStr << "type_" << Bonus::CREATURE_DAMAGE << "s_0Otype_" << Bonus::CREATURE_DAMAGE << "s_1";
	return valOfBonuses(Selector::typeSubtype(Bonus::CREATURE_DAMAGE, 0) || Selector::typeSubtype(Bonus::CREATURE_DAMAGE, 1), cachingStr.str());
}
ui32 IBonusBearer::getMaxDamage() const
{
	std::stringstream cachingStr;
	cachingStr << "type_" << Bonus::CREATURE_DAMAGE << "s_0Otype_" << Bonus::CREATURE_DAMAGE << "s_2";
	return valOfBonuses(Selector::typeSubtype(Bonus::CREATURE_DAMAGE, 0) || Selector::typeSubtype(Bonus::CREATURE_DAMAGE, 2), cachingStr.str());
}

si32 IBonusBearer::manaLimit() const
{
	return si32(getPrimSkillLevel(3) * (100.0 + valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, 24)) / 10.0);
}

int IBonusBearer::getPrimSkillLevel(int id) const
{
	int ret = 0;
	if(id == PrimarySkill::ATTACK)
		ret = Attack();
	else if(id == PrimarySkill::DEFENSE)
		ret = Defense();
	else
		ret = valOfBonuses(Bonus::PRIMARY_SKILL, id);

	vstd::amax(ret, id/2); //minimal value is 0 for attack and defense and 1 for spell power and knowledge
	return ret;
}

si32 IBonusBearer::magicResistance() const
{
	return valOfBonuses(Bonus::MAGIC_RESISTANCE);
}

bool IBonusBearer::isLiving() const //TODO: theoreticaly there exists "LIVING" bonus in stack experience documentation
{
	std::stringstream cachingStr;
	cachingStr << "type_" << Bonus::UNDEAD << "s_-1Otype_" << Bonus::NON_LIVING << "s_-11type_" << Bonus::SIEGE_WEAPON; //I don't relaly get what string labels mean?
	return(!hasBonus(Selector::type(Bonus::UNDEAD) || Selector::type(Bonus::NON_LIVING) || Selector::type(Bonus::SIEGE_WEAPON), cachingStr.str()));
}

const TBonusListPtr IBonusBearer::getSpellBonuses() const
{
	std::stringstream cachingStr;
	cachingStr << "source_" << Bonus::SPELL_EFFECT;
	return getBonuses(Selector::sourceType(Bonus::SPELL_EFFECT), cachingStr.str());
}

const Bonus * IBonusBearer::getEffect(ui16 id, int turn /*= 0*/) const
{
	//TODO should check only local bonuses?
	auto bonuses = getAllBonuses();
	BOOST_FOREACH(const Bonus *it, *bonuses)
	{
		if(it->source == Bonus::SPELL_EFFECT && it->sid == id)
		{
			if(!turn || it->turnsRemain > turn)
				return &(*it);
		}
	}
	return NULL;
}

ui8 IBonusBearer::howManyEffectsSet(ui16 id) const
{
	//TODO should check only local bonuses?
	ui8 ret = 0;

	auto bonuses = getAllBonuses();
	BOOST_FOREACH(const Bonus *it, *bonuses)
	{
		if(it->source == Bonus::SPELL_EFFECT && it->sid == id) //effect found
		{
			++ret;
		}
	}

	return ret;
}

const TBonusListPtr IBonusBearer::getAllBonuses() const
{
	auto matchAll= [] (const Bonus *) { return true; };
	auto matchNone= [] (const Bonus *) { return true; };
	return getAllBonuses(matchAll, matchNone);
}

const Bonus * IBonusBearer::getBonus(const CSelector &selector) const
{
	auto bonuses = getAllBonuses();
	return bonuses->getFirst(selector);
}

Bonus * CBonusSystemNode::getBonusLocalFirst(const CSelector &selector)
{
	Bonus *ret = bonuses.getFirst(selector);
	if(ret)
		return ret;

	FOREACH_PARENT(pname)
	{
		ret = pname->getBonusLocalFirst(selector);
		if (ret)
			return ret;
	}

	return NULL;
}

const Bonus * CBonusSystemNode::getBonusLocalFirst( const CSelector &selector ) const
{
	return (const_cast<CBonusSystemNode*>(this))->getBonusLocalFirst(selector);
}

void CBonusSystemNode::getParents(TCNodes &out) const /*retreives list of parent nodes (nodes to inherit bonuses from) */
{
	for (ui32 i = 0; i < parents.size(); i++)
	{
		const CBonusSystemNode *parent = parents[i];
		out.insert(parent);
	}
}

void CBonusSystemNode::getParents(TNodes &out)
{
	for (ui32 i = 0; i < parents.size(); i++)
	{
		const CBonusSystemNode *parent = parents[i];
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

const TBonusListPtr CBonusSystemNode::getAllBonuses(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root /*= NULL*/, const std::string &cachingStr /*= ""*/) const
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
		auto ret = make_shared<BonusList>();
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

const TBonusListPtr CBonusSystemNode::getAllBonusesWithoutCaching(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root /*= NULL*/) const
{
	auto ret = make_shared<BonusList>();

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

		BOOST_FOREACH(Bonus *b, beforeLimiting) 
			rootBonuses.push_back(b);

		rootBonuses.eliminateDuplicates();
		root->limitBonuses(rootBonuses, limitedRootBonuses);

		BOOST_FOREACH(Bonus *b, beforeLimiting)
			if(vstd::contains(limitedRootBonuses, b))
				afterLimiting.push_back(b);

		afterLimiting.getBonuses(*ret, selector, limit);
	}
	else
		beforeLimiting.getBonuses(*ret, selector, limit);

	return ret;
}

CBonusSystemNode::CBonusSystemNode() : bonuses(true), exportedBonuses(true), nodeType(UNKNOWN), cachedLast(0)
{
}

CBonusSystemNode::~CBonusSystemNode()
{
	detachFromAll();

	if(children.size())
	{
		tlog2 << "Warning: an orphaned child!\n";
		while(children.size())
			children.front()->detachFrom(this);
	}
	
	BOOST_FOREACH(Bonus *b, exportedBonuses)
		delete b;
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
	CBonusSystemNode::treeChanged++;
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
	CBonusSystemNode::treeChanged++;
}

void CBonusSystemNode::popBonuses(const CSelector &s)
{
	BonusList bl;
	exportedBonuses.getBonuses(bl, s);
	BOOST_FOREACH(Bonus *b, bl)
		removeBonus(b);

	BOOST_FOREACH(CBonusSystemNode *child, children)
		child->popBonuses(s);
}

// void CBonusSystemNode::addNewBonus(const Bonus &b)
// {
// 	addNewBonus(new Bonus(b));
// }

void CBonusSystemNode::addNewBonus(Bonus *b)
{
	assert(!vstd::contains(exportedBonuses,b));
	exportedBonuses.push_back(b);
	exportBonus(b);
	CBonusSystemNode::treeChanged++;
}

void CBonusSystemNode::accumulateBonus(Bonus &b)
{
	Bonus *bonus = exportedBonuses.getFirst(Selector::typeSubtype(b.type, b.subtype)); //only local bonuses are interesting //TODO: what about value type?
	if(bonus)
		bonus->val += b.val;
	else
		addNewBonus(new Bonus(b)); //duplicate needed, original may get destroyed
}

void CBonusSystemNode::removeBonus(Bonus *b)
{
	exportedBonuses -= b;
	if(b->propagator)
		unpropagateBonus(b);
	else
		bonuses -= b;
	vstd::clear_pointer(b);
	CBonusSystemNode::treeChanged++;
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

void CBonusSystemNode::propagateBonus(Bonus * b)
{
	if(b->propagator->shouldBeAttached(this))
	{
		bonuses.push_back(b);
		BONUS_LOG_LINE("#$# " << b->Description() << " #propagated to# " << nodeName());
	}

	FOREACH_RED_CHILD(child)
		child->propagateBonus(b);
}

void CBonusSystemNode::unpropagateBonus(Bonus * b)
{
	if(b->propagator->shouldBeAttached(this))
	{
		bonuses -= b;
		while(vstd::contains(bonuses, b))
		{
			tlog1 << "Bonus was duplicated (" << b->Description() << ") at " << nodeName() << std::endl;
			bonuses -= b;
		}
		BONUS_LOG_LINE("#$#" << b->Description() << " #is no longer propagated to# " << nodeName());
	}

	FOREACH_RED_CHILD(child)
		child->unpropagateBonus(b);
}

void CBonusSystemNode::newChildAttached(CBonusSystemNode *child)
{
	assert(!vstd::contains(children, child));
	children.push_back(child);
	BONUS_LOG_LINE(child->nodeName() << " #attached to# " << nodeName());
}

void CBonusSystemNode::childDetached(CBonusSystemNode *child)
{
	assert(vstd::contains(children, child));
	children -= child;
	BONUS_LOG_LINE(child->nodeName() << " #detached from# " << nodeName());
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
		BOOST_FOREACH(CBonusSystemNode *child, children)
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
		BOOST_FOREACH(CBonusSystemNode *child, children)
		{
			out.insert(child);
		}
	}
}

void CBonusSystemNode::newRedDescendant(CBonusSystemNode *descendant)
{
	BOOST_FOREACH(Bonus *b, exportedBonuses)
		if(b->propagator)
			descendant->propagateBonus(b);

	FOREACH_RED_PARENT(parent)
		parent->newRedDescendant(descendant);
}

void CBonusSystemNode::removedRedDescendant(CBonusSystemNode *descendant)
{
	BOOST_FOREACH(Bonus *b, exportedBonuses)
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

void CBonusSystemNode::battleTurnPassed()
{
	BonusList bonusesCpy = exportedBonuses; //copy, because removing bonuses invalidates iters
	for (ui32 i = 0; i < bonusesCpy.size(); i++)
	{
		Bonus *b = bonusesCpy[i];

		if(b->duration & Bonus::N_TURNS)
		{
			b->turnsRemain--;
			if(b->turnsRemain <= 0)
				removeBonus(b);
		}
	}
}

void CBonusSystemNode::exportBonus(Bonus * b)
{
	if(b->propagator)
		propagateBonus(b);
	else
		bonuses.push_back(b);

	CBonusSystemNode::treeChanged++;
}

void CBonusSystemNode::exportBonuses()
{
	BOOST_FOREACH(Bonus *b, exportedBonuses)
		exportBonus(b);
}

ui8 CBonusSystemNode::getNodeType() const
{
	return nodeType;
}

BonusList& CBonusSystemNode::getBonusList()
{
	return bonuses;
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

void CBonusSystemNode::setNodeType(ui8 type)
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

void CBonusSystemNode::incrementTreeChangedNum()
{
	treeChanged++;
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
			Bonus *b = undecided[i];
			BonusLimitationContext context = {b, *this, out};
			int decision = b->limit(context); //bonuses without limiters will be accepted by default
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
	auto ret = make_shared<BonusList>();
	limitBonuses(allBonuses, *ret);
	return ret;
}

void CBonusSystemNode::treeHasChanged()
{
	treeChanged++;
}

int NBonus::valOf(const CBonusSystemNode *obj, Bonus::BonusType type, int subtype /*= -1*/)
{
	if(obj)
		return obj->valOfBonuses(type, subtype);
	return 0;
}

bool NBonus::hasOfType(const CBonusSystemNode *obj, Bonus::BonusType type, int subtype /*= -1*/)
{
	if(obj)
		return obj->hasBonusOfType(type, subtype);
	return false;
}

void NBonus::getModifiersWDescr(const CBonusSystemNode *obj, TModDescr &out, Bonus::BonusType type, int subtype /*= -1 */)
{
	if(obj)
		return obj->getModifiersWDescr(out, type, subtype);
}

int NBonus::getCount(const CBonusSystemNode *obj, int from, int id)
{
	if(obj)
		return obj->getBonusesCount(from, id);
	return 0;
}

const CSpell * Bonus::sourceSpell() const
{
	if(source == SPELL_EFFECT)
		return VLC->spellh->spells[sid];
	return NULL;
}

std::string Bonus::Description() const
{
	if(description.size())
		return description;

	std::ostringstream str;
	str << std::showpos << val << " ";

	switch(source)
	{
	case ARTIFACT:
		str << VLC->arth->artifacts[sid]->Name();
		break;;
	case SPELL_EFFECT:
		str << VLC->spellh->spells[sid]->name;
		break;
	case CREATURE_ABILITY:
		str << VLC->creh->creatures[sid]->namePl;
		break;
	case SECONDARY_SKILL:
		str << VLC->generaltexth->skillName[sid]/* << " secondary skill"*/;
		break;
	}
	
	return str.str();
}

Bonus::Bonus(ui16 Dur, ui8 Type, ui8 Src, si32 Val, ui32 ID, std::string Desc, si32 Subtype/*=-1*/) 
	: duration(Dur), type(Type), subtype(Subtype), source(Src), val(Val), sid(ID), description(Desc)
{
	additionalInfo = -1;
	turnsRemain = 0;
	valType = ADDITIVE_VALUE;
	effectRange = NO_LIMIT;
	boost::algorithm::trim(description);
}

Bonus::Bonus(ui16 Dur, ui8 Type, ui8 Src, si32 Val, ui32 ID, si32 Subtype/*=-1*/, ui8 ValType /*= ADDITIVE_VALUE*/) 
	: duration(Dur), type(Type), subtype(Subtype), source(Src), val(Val), sid(ID), valType(ValType)
{
	additionalInfo = -1;
	turnsRemain = 0;
	effectRange = NO_LIMIT;
}

Bonus::Bonus()
{
	subtype = -1;
	additionalInfo = -1;
	turnsRemain = 0;
	valType = ADDITIVE_VALUE;
	effectRange = NO_LIMIT;
}

Bonus::~Bonus()
{
}

Bonus * Bonus::addLimiter(TLimiterPtr Limiter)
{
	limiter = Limiter;
	return this;
}

Bonus * Bonus::addPropagator(TPropagatorPtr Propagator)
{
	propagator = Propagator;
	return this;
}

int Bonus::limit(const BonusLimitationContext &context) const
{
	if (limiter)
		return limiter->callNext(context);
	else
		return ILimiter::ACCEPT; //accept if there's no limiter
}

CSelector DLL_LINKAGE operator&&(const CSelector &first, const CSelector &second)
{
	return CSelectorsConjunction(first, second);
}
CSelector DLL_LINKAGE operator||(const CSelector &first, const CSelector &second)
{
	return CSelectorsAlternative(first, second);
}

namespace Selector
{
	DLL_LINKAGE CSelectFieldEqual<TBonusType> type(&Bonus::type, 0);
	DLL_LINKAGE CSelectFieldEqual<TBonusSubtype> subtype(&Bonus::subtype, 0);
	DLL_LINKAGE CSelectFieldEqual<si32> info(&Bonus::additionalInfo, 0);
	DLL_LINKAGE CSelectFieldEqual<ui16> duration(&Bonus::duration, 0);
	DLL_LINKAGE CSelectFieldEqual<ui8> sourceType(&Bonus::source, 0);
	DLL_LINKAGE CSelectFieldEqual<ui8> effectRange(&Bonus::effectRange, Bonus::NO_LIMIT);
	DLL_LINKAGE CWillLastTurns turns;

	CSelector DLL_LINKAGE typeSubtype(TBonusType Type, TBonusSubtype Subtype)
	{
		return type(Type) && subtype(Subtype);
	}

	CSelector DLL_LINKAGE typeSubtypeInfo(TBonusType type, TBonusSubtype subtype, si32 info)
	{
		return CSelectFieldEqual<TBonusType>(&Bonus::type, type) && CSelectFieldEqual<TBonusSubtype>(&Bonus::subtype, subtype) && CSelectFieldEqual<si32>(&Bonus::additionalInfo, info);
	}

	CSelector DLL_LINKAGE source(ui8 source, ui32 sourceID)
	{
		return CSelectFieldEqual<ui8>(&Bonus::source, source) && CSelectFieldEqual<ui32>(&Bonus::sid, sourceID);
	}

	CSelector DLL_EXPORT durationType(ui16 duration)
	{
		return CSelectFieldEqual<ui16>(&Bonus::duration, duration);
	}

	CSelector DLL_LINKAGE sourceTypeSel(ui8 source)
	{
		return CSelectFieldEqual<ui8>(&Bonus::source, source);
	}

	bool DLL_LINKAGE matchesType(const CSelector &sel, TBonusType type)
	{
		Bonus dummy;
		dummy.type = type;
		return sel(&dummy);
	}

	bool DLL_LINKAGE matchesTypeSubtype(const CSelector &sel, TBonusType type, TBonusSubtype subtype)
	{
		Bonus dummy;
		dummy.type = type;
		dummy.subtype = subtype;
		return sel(&dummy);
	}

	bool DLL_LINKAGE positiveSpellEffects(const Bonus *b)
	{
		if(b->source == Bonus::SPELL_EFFECT)
		{
			CSpell *sp = VLC->spellh->spells[b->sid];
			return sp->isPositive();
		}
		return false; //not a spell effect
	}
}

const CStack * retreiveStackBattle(const CBonusSystemNode *node)
{
	switch(node->getNodeType())
	{
	case CBonusSystemNode::STACK_BATTLE:
		return static_cast<const CStack*>(node);
	default:
		return NULL;
	}
}

const CStackInstance * retreiveStackInstance(const CBonusSystemNode *node)
{
	switch(node->getNodeType())
	{
	case CBonusSystemNode::STACK_INSTANCE:
		return (static_cast<const CStackInstance *>(node));
	case CBonusSystemNode::STACK_BATTLE:
		return (static_cast<const CStack*>(node))->base;
	default:
		return NULL;
	}
}

const CCreature * retrieveCreature(const CBonusSystemNode *node)
{
	switch(node->getNodeType())
	{
	case CBonusSystemNode::CREATURE:
		return (static_cast<const CCreature *>(node));
	default:
		const CStackInstance *csi = retreiveStackInstance(node);
		if(csi)
			return csi->type;
		return NULL;
	}
}

DLL_LINKAGE std::ostream & operator<<(std::ostream &out, const BonusList &bonusList)
{
	for (ui32 i = 0; i < bonusList.size(); i++)
	{
		Bonus *b = bonusList[i];
		out << "Bonus " << i << "\n" << *b << std::endl;
	}
	return out;
}

DLL_LINKAGE std::ostream & operator<<(std::ostream &out, const Bonus &bonus)
{
	for(std::map<std::string, int>::const_iterator i = bonusNameMap.begin(); i != bonusNameMap.end(); i++)
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

int LimiterDecorator::limit(const BonusLimitationContext &context) const /*return true to drop the bonus */
{
	return false;
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
	//drop bonus if it's not our creature and (we dont check upgrades or its not our upgrade)
}

CCreatureTypeLimiter::CCreatureTypeLimiter(const CCreature &Creature, ui8 IncludeUpgrades /*= true*/)
	:creature(&Creature), includeUpgrades(IncludeUpgrades)
{
}

CCreatureTypeLimiter::CCreatureTypeLimiter()
{
	creature = NULL;
	includeUpgrades = false;
}

HasAnotherBonusLimiter::HasAnotherBonusLimiter( TBonusType bonus )
	: type(bonus), subtype(0), isSubtypeRelevant(false)
{
}

HasAnotherBonusLimiter::HasAnotherBonusLimiter( TBonusType bonus, TBonusSubtype _subtype )
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

// CBonusSystemNode * IPropagator::getDestNode(CBonusSystemNode *source, CBonusSystemNode *redParent, CBonusSystemNode *redChild)
// {
// 	tlog1 << "IPropagator::getDestNode called!\n";
// 	return source;
// }

bool IPropagator::shouldBeAttached(CBonusSystemNode *dest)
{
	return false;
}

// CBonusSystemNode * CPropagatorNodeType::getDestNode(CBonusSystemNode *source, CBonusSystemNode *redParent, CBonusSystemNode *redChild)
// {
// 	return NULL;
// }

CPropagatorNodeType::CPropagatorNodeType()
{

}

CPropagatorNodeType::CPropagatorNodeType(ui8 NodeType)
	: nodeType(NodeType)
{
}

bool CPropagatorNodeType::shouldBeAttached(CBonusSystemNode *dest)
{
	return nodeType == dest->getNodeType();
}

int LimiterDecorator::callNext(const BonusLimitationContext &context) const
{
	if (next)
	{
		return (limit(context) || callNext(context)); //either of limiters will cause bonus to drop
	}
	else //we are last on the list
		return limit (context);
}

CreatureNativeTerrainLimiter::CreatureNativeTerrainLimiter(int TerrainType) 
	: terrainType(TerrainType)
{
}

CreatureNativeTerrainLimiter::CreatureNativeTerrainLimiter()
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
{
}

int CreatureFactionLimiter::limit(const BonusLimitationContext &context) const
{
	const CCreature *c = retrieveCreature(&context.node);
	return !c || c->faction != faction; //drop bonus for non-creatures or non-native residents
}

CreatureAlignmentLimiter::CreatureAlignmentLimiter()
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
		tlog1 << "Warning: illegal alignment in limiter!\n";
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
	const CStackInstance *csi = retreiveStackInstance(&context.node);
	if(csi)
	{
		if (csi->getNodeType() == Bonus::COMMANDER) //no stack exp bonuses for commander creatures
			return true;
		return csi->getExpRank() < minRank || csi->getExpRank() > maxRank;
	}
	return true;
}

int StackOwnerLimiter::limit(const BonusLimitationContext &context) const 
{
	const CStack *s = retreiveStackBattle(&context.node);
	if(s)
		return s->owner != owner;

 	const CStackInstance *csi = retreiveStackInstance(&context.node);
 	if(csi && csi->armyObj)
 		return csi->armyObj->tempOwner != owner;
 	return true;
}

StackOwnerLimiter::StackOwnerLimiter()
	: owner(-1)
{
}

StackOwnerLimiter::StackOwnerLimiter(ui8 Owner)
	: owner(Owner)
{
}
