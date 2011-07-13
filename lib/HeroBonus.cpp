#define VCMI_DLL
#include "HeroBonus.h"
#include <boost/foreach.hpp>
#include "VCMI_Lib.h"
#include "CSpellHandler.h"
#include <sstream>
#include "CCreatureHandler.h"
#include <boost/assign/list_of.hpp>
#include "CCreatureSet.h"
#include <boost/algorithm/string/trim.hpp>
#include <boost/bind.hpp>
#include "CHeroHandler.h"
#include "CGeneralTextHandler.h"
#include "BattleState.h"
#include "CArtHandler.h"

#define FOREACH_PARENT(pname) 	TNodes lparents; getParents(lparents); BOOST_FOREACH(CBonusSystemNode *pname, lparents)
#define FOREACH_RED_CHILD(pname) 	TNodes lchildren; getRedChildren(lchildren); BOOST_FOREACH(CBonusSystemNode *pname, lchildren)
#define FOREACH_RED_PARENT(pname) 	TNodes lparents; getRedParents(lparents); BOOST_FOREACH(CBonusSystemNode *pname, lparents)

#define BONUS_NAME(x) ( #x, Bonus::x )
	DLL_EXPORT const std::map<std::string, int> bonusNameMap = boost::assign::map_list_of BONUS_LIST;
#undef BONUS_NAME

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

	BOOST_FOREACH(Bonus *i, bonuses)
	{
		switch(i->valType)
		{
		case Bonus::BASE_NUMBER:
			base += i->val;
			break;
		case Bonus::PERCENT_TO_ALL:
			percentToAll += i->val;
			break;
		case Bonus::PERCENT_TO_BASE:
			percentToBase += i->val;
			break;
		case Bonus::ADDITIVE_VALUE:
			additive += i->val;
			break;
		case Bonus::INDEPENDENT_MAX:
			if (!hasIndepMax)
			{
				indepMax = i->val;
				hasIndepMax = true;
			}
			else
			{
				amax(indepMax, i->val);
			}

			break;
		case Bonus::INDEPENDENT_MIN:
			if (!hasIndepMin)
			{
				indepMin = i->val;
				hasIndepMin = true;
			}
			else
			{
				amin(indepMin, i->val);
			}

			break;
		}
	}
	int modifiedBase = base + (base * percentToBase) / 100;
	modifiedBase += additive;
	int valFirst = (modifiedBase * (100 + percentToAll)) / 100;

	if(hasIndepMin && hasIndepMax)
		assert(indepMin < indepMax);
	if (hasIndepMax)
		amax(valFirst, indepMax);
	if (hasIndepMin)
		amin(valFirst, indepMin);

	return valFirst;
}
const Bonus * BonusList::getFirst(const CSelector &selector) const
{
	for (unsigned int i = 0; i < bonuses.size(); i++)
	{
		const Bonus *b = bonuses[i];
		if(selector(b))
			return &*b;
	}
	return NULL;
}

Bonus * BonusList::getFirst(const CSelector &select)
{
	for (unsigned int i = 0; i < bonuses.size(); i++)
	{
		Bonus *b = bonuses[i];
		if(select(b))
			return &*b;
	}
	return NULL;
}

void BonusList::getModifiersWDescr(TModDescr &out) const
{
	BOOST_FOREACH(Bonus *i, bonuses)
		out.push_back(std::make_pair(i->val, i->Description()));
}

void BonusList::getBonuses(boost::shared_ptr<BonusList> out, const CSelector &selector) const
{
// 	BOOST_FOREACH(Bonus *i, *this)
// 		if(selector(i) && i->effectRange == Bonus::NO_LIMIT)
// 			out.push_back(i);

	getBonuses(out, selector, 0);
}

void BonusList::getBonuses(boost::shared_ptr<BonusList> out, const CSelector &selector, const CSelector &limit, const bool caching /*= false*/) const
{
	for (unsigned int i = 0; i < bonuses.size(); i++)
	{
		Bonus *b = bonuses[i];

		//add matching bonuses that matches limit predicate or have NO_LIMIT if no given predicate
		if(caching || (selector(b) && ((!limit && b->effectRange == Bonus::NO_LIMIT) || (limit && limit(b)))))
			out->push_back(b);
	}
}

int BonusList::valOfBonuses(const CSelector &select) const
{
	boost::shared_ptr<BonusList> ret(new BonusList());
	CSelector limit = 0;
	getBonuses(ret, select, limit, false);
	ret->eliminateDuplicates();
	return ret->totalValue();
}

void BonusList::limit(const CBonusSystemNode &node)
{
	remove_if(boost::bind(&CBonusSystemNode::isLimitedOnUs, boost::ref(node), _1));
}


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

std::vector<Bonus*>::iterator BonusList::erase(std::vector<Bonus*>::const_iterator position)
{
	if (belongsToTree)
		CBonusSystemNode::incrementTreeChangedNum();
	return bonuses.erase(position);
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
	CSelector s = Selector::type(type);
	if(subtype != -1)
		s = s && Selector::subtype(subtype);

	return valOfBonuses(s);
}

int IBonusBearer::valOfBonuses(const CSelector &selector) const
{
	CSelector limit = 0;
	boost::shared_ptr<BonusList> hlp = getAllBonuses(selector, limit, NULL);
	return hlp->totalValue();
}
bool IBonusBearer::hasBonus(const CSelector &selector, const std::string &cachingStr /*= ""*/) const
{
	return getBonuses(selector, cachingStr)->size() > 0;
}

bool IBonusBearer::hasBonusOfType(Bonus::BonusType type, int subtype /*= -1*/) const
{
	CSelector s = Selector::type(type);
	std::string cachingStr = "";
	if(subtype != -1)
		s = s && Selector::subtype(subtype);
	else
	{
		cachingStr = "type_";
		cachingStr += ((char) type);
	}

	return hasBonus(s, cachingStr);
}

void IBonusBearer::getModifiersWDescr(TModDescr &out, Bonus::BonusType type, int subtype /*= -1 */) const
{
	getModifiersWDescr(out, subtype != -1 ? Selector::typeSubtype(type, subtype) : Selector::type(type));
}

void IBonusBearer::getModifiersWDescr(TModDescr &out, const CSelector &selector) const
{
	getBonuses(selector)->getModifiersWDescr(out);
}
int IBonusBearer::getBonusesCount(int from, int id) const
{
	return getBonusesCount(Selector::source(from, id));
}

int IBonusBearer::getBonusesCount(const CSelector &selector) const
{
	return getBonuses(selector)->size();
}

const boost::shared_ptr<BonusList> IBonusBearer::getBonuses(const CSelector &selector, const std::string &cachingStr /*= ""*/) const
{
	return getAllBonuses(selector, 0, NULL, cachingStr);
}

const boost::shared_ptr<BonusList> IBonusBearer::getBonuses(const CSelector &selector, const CSelector &limit, const std::string &cachingStr /*= ""*/) const
{
	return getAllBonuses(selector, limit, NULL, cachingStr);
}

bool IBonusBearer::hasBonusFrom(ui8 source, ui32 sourceID) const
{
	return hasBonus(Selector::source(source,sourceID));
}

int IBonusBearer::MoraleVal() const
{
	if(hasBonusOfType(Bonus::NON_LIVING) || hasBonusOfType(Bonus::UNDEAD) ||
		hasBonusOfType(Bonus::NO_MORALE) || hasBonusOfType(Bonus::SIEGE_WEAPON))
		return 0;

	int ret = valOfBonuses(Selector::type(Bonus::MORALE));

	if(hasBonusOfType(Bonus::SELF_MORALE)) //eg. minotaur
		amax(ret, +1);

	return abetw(ret, -3, +3);
}

int IBonusBearer::LuckVal() const
{
	if(hasBonusOfType(Bonus::NO_LUCK))
		return 0;

	int ret = valOfBonuses(Selector::type(Bonus::LUCK));
	
	if(hasBonusOfType(Bonus::SELF_LUCK)) //eg. halfling
		amax(ret, +1);

	return abetw(ret, -3, +3);
}

si32 IBonusBearer::Attack() const
{
	si32 ret = valOfBonuses(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK);

	if(int frenzyPower = valOfBonuses(Bonus::IN_FRENZY)) //frenzy for attacker
	{
		ret += frenzyPower * Defense(false);
	}
	amax(ret, 0);

	return ret;
}

si32 IBonusBearer::Defense(bool withFrenzy /*= true*/) const
{
	si32 ret = valOfBonuses(Bonus::PRIMARY_SKILL, PrimarySkill::DEFENSE);

	if(withFrenzy && hasBonusOfType(Bonus::IN_FRENZY)) //frenzy for defender
	{
		return 0;
	}
	amax(ret, 0);

	return ret;
}

ui16 IBonusBearer::MaxHealth() const
{
	return std::max(1, valOfBonuses(Bonus::STACK_HEALTH)); //never 0 or negative
}

ui32 IBonusBearer::getMinDamage() const
{
	return valOfBonuses(Selector::typeSubtype(Bonus::CREATURE_DAMAGE, 0) || Selector::typeSubtype(Bonus::CREATURE_DAMAGE, 1));
}
ui32 IBonusBearer::getMaxDamage() const
{
	return valOfBonuses(Selector::typeSubtype(Bonus::CREATURE_DAMAGE, 0) || Selector::typeSubtype(Bonus::CREATURE_DAMAGE, 2));
}

si32 IBonusBearer::manaLimit() const
{
	return si32(getPrimSkillLevel(3) * (100.0f + valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, 24)) / 10.0f);
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

	amax(ret, id/2); //minimal value is 0 for attack and defense and 1 for spell power and knowledge
	return ret;
}

si32 IBonusBearer::magicResistance() const
{
	return valOfBonuses(Selector::type(Bonus::MAGIC_RESISTANCE));
}

bool IBonusBearer::isLiving() const //TODO: theoreticaly there exists "LIVING" bonus in stack experience documentation
{
	return(!hasBonus(Selector::type(Bonus::UNDEAD) || Selector::type(Bonus::NON_LIVING)));
}

const boost::shared_ptr<BonusList> IBonusBearer::getSpellBonuses() const
{
	std::string cachingStr = "source_";
	cachingStr += (char) Bonus::SPELL_EFFECT;
	return getBonuses(Selector::sourceType(Bonus::SPELL_EFFECT), cachingStr);
}

Bonus * CBonusSystemNode::getBonus(const CSelector &selector)
{
	Bonus *ret = bonuses.getFirst(selector);
	if(ret)
		return ret;

	FOREACH_PARENT(pname)
	{
		ret = pname->getBonus(selector);
		if (ret)
			return ret;
	}

	return NULL;
}

const Bonus * CBonusSystemNode::getBonus( const CSelector &selector ) const
{
	return (const_cast<CBonusSystemNode*>(this))->getBonus(selector);
}

void CBonusSystemNode::getParents(TCNodes &out) const /*retreives list of parent nodes (nodes to inherit bonuses from) */
{
	for (unsigned int i = 0; i < parents.size(); i++)
	{
		const CBonusSystemNode *parent = parents[i];
		out.insert(parent);
	}
}

void CBonusSystemNode::getParents(TNodes &out)
{
	for (unsigned int i = 0; i < parents.size(); i++)
	{
		const CBonusSystemNode *parent = parents[i];
		out.insert(const_cast<CBonusSystemNode*>(parent));
	}	
}

void CBonusSystemNode::getAllBonusesRec(boost::shared_ptr<BonusList> out, const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root /*= NULL*/, const bool caching /*= false*/) const
{
	TCNodes lparents; 
	getParents(lparents); 
	BOOST_FOREACH(const CBonusSystemNode *p, lparents)
		p->getAllBonusesRec(out, selector, limit, root ? root : this, caching);
	
	bonuses.getBonuses(out, selector, limit, caching);

	if(!root)
		out->limit(*this);
}

const boost::shared_ptr<BonusList> CBonusSystemNode::getAllBonuses(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root /*= NULL*/, const std::string &cachingStr /*= ""*/) const
{
	boost::shared_ptr<BonusList> ret(new BonusList());
	if (CBonusSystemNode::cachingEnabled)
	{
		static boost::mutex m;
		boost::mutex::scoped_lock lock(m);

		if (cachedLast != treeChanged)
		{
			getAllBonusesRec(ret, selector, limit, this, true);
			ret->eliminateDuplicates();
			cachedBonuses = *ret;
			ret->clear();
			cachedRequests.clear();
			cachedLast = treeChanged;
		}
	
		if (cachingStr != "")
		{
			std::map<std::string, boost::shared_ptr<BonusList> >::iterator it(cachedRequests.find(cachingStr));
			if (cachedRequests.size() > 0 && it != cachedRequests.end())
			{
				ret = it->second;
				return ret;
			}
		}
	
		cachedBonuses.getBonuses(ret, selector, limit, false);
		if (!root)
			ret->limit(*this);

		if (cachingStr != "")
			cachedRequests[cachingStr] = ret;


		return ret;
	}
	else
	{
		getAllBonusesRec(ret, selector, limit, root, false);
		ret->eliminateDuplicates();
		return ret;
	}
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
	boost::shared_ptr<BonusList> bl(new BonusList);
	exportedBonuses.getBonuses(bl, s);
	BOOST_FOREACH(Bonus *b, *bl)
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

void CBonusSystemNode::removeBonus(Bonus *b)
{
	exportedBonuses -= b;
	if(b->propagator)
		unpropagateBonus(b);
	else
		bonuses -= b;
	delNull(b);
	CBonusSystemNode::treeChanged++;
}

bool CBonusSystemNode::isLimitedOnUs(Bonus *b) const
{
	return b->limiter && b->limiter->limit(b, *this);
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
	for (unsigned int i = 0; i < bonusesCpy.size(); i++)
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

const ui8 CBonusSystemNode::getNodeType() const
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

Bonus * Bonus::addLimiter(ILimiter *Limiter)
{
	return addLimiter(boost::shared_ptr<ILimiter>(Limiter));
}

Bonus * Bonus::addLimiter(boost::shared_ptr<ILimiter> Limiter)
{
	limiter = Limiter;
	return this;
}

Bonus * Bonus::addPropagator(IPropagator *Propagator)
{
	return addPropagator(boost::shared_ptr<IPropagator>(Propagator));
}

Bonus * Bonus::addPropagator(boost::shared_ptr<IPropagator> Propagator)
{
	propagator = Propagator;
	return this;
}

CSelector DLL_EXPORT operator&&(const CSelector &first, const CSelector &second)
{
	return CSelectorsConjunction(first, second);
}
CSelector DLL_EXPORT operator||(const CSelector &first, const CSelector &second)
{
	return CSelectorsAlternative(first, second);
}

namespace Selector
{
	DLL_EXPORT CSelectFieldEqual<TBonusType> type(&Bonus::type, 0);
	DLL_EXPORT CSelectFieldEqual<TBonusSubtype> subtype(&Bonus::subtype, 0);
	DLL_EXPORT CSelectFieldEqual<si32> info(&Bonus::additionalInfo, 0);
	DLL_EXPORT CSelectFieldEqual<ui16> duration(&Bonus::duration, 0);
	DLL_EXPORT CSelectFieldEqual<ui8> sourceType(&Bonus::source, 0);
	DLL_EXPORT CSelectFieldEqual<ui8> effectRange(&Bonus::effectRange, Bonus::NO_LIMIT);
	DLL_EXPORT CWillLastTurns turns;

	CSelector DLL_EXPORT typeSubtype(TBonusType Type, TBonusSubtype Subtype)
	{
		return type(Type) && subtype(Subtype);
	}

	CSelector DLL_EXPORT typeSubtypeInfo(TBonusType type, TBonusSubtype subtype, si32 info)
	{
		return CSelectFieldEqual<TBonusType>(&Bonus::type, type) && CSelectFieldEqual<TBonusSubtype>(&Bonus::subtype, subtype) && CSelectFieldEqual<si32>(&Bonus::additionalInfo, info);
	}

	CSelector DLL_EXPORT source(ui8 source, ui32 sourceID)
	{
		return CSelectFieldEqual<ui8>(&Bonus::source, source) && CSelectFieldEqual<ui32>(&Bonus::sid, sourceID);
	}

	CSelector DLL_EXPORT durationType(ui16 duration)
	{
		return CSelectFieldEqual<ui16>(&Bonus::duration, duration);
	}

	CSelector DLL_EXPORT sourceTypeSel(ui8 source)
	{
		return CSelectFieldEqual<ui8>(&Bonus::source, source);
	}

	bool DLL_EXPORT matchesType(const CSelector &sel, TBonusType type)
	{
		Bonus dummy;
		dummy.type = type;
		return sel(&dummy);
	}

	bool DLL_EXPORT matchesTypeSubtype(const CSelector &sel, TBonusType type, TBonusSubtype subtype)
	{
		Bonus dummy;
		dummy.type = type;
		dummy.subtype = subtype;
		return sel(&dummy);
	}

	bool DLL_EXPORT positiveSpellEffects(const Bonus *b)
	{
		if(b->source == Bonus::SPELL_EFFECT)
		{
			CSpell *sp = VLC->spellh->spells[b->sid];
			return sp->positiveness == 1;
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

DLL_EXPORT std::ostream & operator<<(std::ostream &out, const BonusList &bonusList)
{
	for (unsigned int i = 0; i < bonusList.size(); i++)
	{
		Bonus *b = bonusList[i];
		out << "Bonus " << i << "\n" << *b << std::endl;
	}
	return out;
}

DLL_EXPORT std::ostream & operator<<(std::ostream &out, const Bonus &bonus)
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

ILimiter::~ILimiter()
{
}

bool ILimiter::limit(const Bonus *b, const CBonusSystemNode &node) const /*return true to drop the bonus */
{
	return false;
}

bool CCreatureTypeLimiter::limit(const Bonus *b, const CBonusSystemNode &node) const
{
	const CCreature *c = retrieveCreature(&node);
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

bool HasAnotherBonusLimiter::limit( const Bonus *b, const CBonusSystemNode &node ) const
{
	if(isSubtypeRelevant)
	{
		return !node.hasBonusOfType(static_cast<Bonus::BonusType>(type), subtype);
	}
	else
	{
		return !node.hasBonusOfType(static_cast<Bonus::BonusType>(type));
	}
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

CreatureNativeTerrainLimiter::CreatureNativeTerrainLimiter(int TerrainType) 
	: terrainType(TerrainType)
{
}

CreatureNativeTerrainLimiter::CreatureNativeTerrainLimiter()
{

}
bool CreatureNativeTerrainLimiter::limit(const Bonus *b, const CBonusSystemNode &node) const
{
	const CCreature *c = retrieveCreature(&node);
	return !c || !iswith(c->faction, 0, 9) || VLC->heroh->nativeTerrains[c->faction] != terrainType; //drop bonus for non-creatures or non-native residents
	//TODO neutral creatues
}

CreatureFactionLimiter::CreatureFactionLimiter(int Faction)
	: faction(Faction)
{
}

CreatureFactionLimiter::CreatureFactionLimiter()
{
}
bool CreatureFactionLimiter::limit(const Bonus *b, const CBonusSystemNode &node) const
{
	const CCreature *c = retrieveCreature(&node);
	return !c || c->faction != faction; //drop bonus for non-creatures or non-native residents
}

CreatureAlignmentLimiter::CreatureAlignmentLimiter()
{
}

CreatureAlignmentLimiter::CreatureAlignmentLimiter(si8 Alignment)
	: alignment(Alignment)
{
}

bool CreatureAlignmentLimiter::limit(const Bonus *b, const CBonusSystemNode &node) const
{
	const CCreature *c = retrieveCreature(&node);
	if(!c) 
		return true;
	switch(alignment)
	{
	case GOOD:
		return !c->isGood(); //if not good -> return true (drop bonus)
	case NEUTRAL:
		return c->isEvil() || c->isGood();
	case EVIL:
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

bool RankRangeLimiter::limit( const Bonus *b, const CBonusSystemNode &node ) const
{
	const CStackInstance *csi = retreiveStackInstance(&node);
	if(csi)
		return csi->getExpRank() < minRank || csi->getExpRank() > maxRank;
	return true;
}

bool StackOwnerLimiter::limit(const Bonus *b, const CBonusSystemNode &node) const 
{
	const CStack *s = retreiveStackBattle(&node);
	if(s)
		return s->owner != owner;

 	const CStackInstance *csi = retreiveStackInstance(&node);
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
