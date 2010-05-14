#define VCMI_DLL
#include "HeroBonus.h"
#include <boost/foreach.hpp>
#include "VCMI_Lib.h"
#include "../hch/CSpellHandler.h"
#include <sstream>
#include "../hch/CCreatureHandler.h"

#define FOREACH_CONST_PARENT(pname, source) 	TCNodes parents; getParents(parents, source); BOOST_FOREACH(const CBonusSystemNode *pname, parents)
#define FOREACH_PARENT(pname, source) 	TNodes parents; getParents(parents, source); BOOST_FOREACH(CBonusSystemNode *pname, parents)

int DLL_EXPORT BonusList::totalValue() const
{
	int base = 0;
	int percentToBase = 0;
	int percentToAll = 0;
	int additive = 0;

	for(const_iterator i = begin(); i != end(); i++)
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
		}
	}
	int modifiedBase = base + (base * percentToBase) / 100;
	modifiedBase += additive;
	return (modifiedBase * (100 + percentToAll)) / 100;
}

const DLL_EXPORT Bonus * BonusList::getFirst(const CSelector &selector) const
{
	for (const_iterator i = begin(); i != end(); i++)
		if(selector(*i))
			return &*i;
	return NULL;
}

DLL_EXPORT Bonus * BonusList::getFirst(const CSelector &select)
{
	for (iterator i = begin(); i != end(); i++)
		if(select(*i))
			return &*i;
	return NULL;
}

void DLL_EXPORT BonusList::getModifiersWDescr(TModDescr &out) const
{
	for(const_iterator i = begin(); i != end(); i++)
		out.push_back(std::make_pair(i->val, i->Description()));
}

void DLL_EXPORT BonusList::getBonuses(BonusList &out, const CSelector &selector, const CBonusSystemNode *source /*= NULL*/) const
{
	for(const_iterator i = begin(); i != end(); i++)
		if(selector(*i) && i->effectRange == Bonus::NO_LIMIT)
			out.push_back(*i);
}

void DLL_EXPORT BonusList::getBonuses(BonusList &out, const CSelector &selector, const CSelector &limit, const CBonusSystemNode *source /*= NULL*/) const
{
	for(const_iterator i = begin(); i != end(); i++)
		if(selector(*i) && (!limit || limit(*i)))
			out.push_back(*i);
}

int CBonusSystemNode::valOfBonuses(Bonus::BonusType type, int subtype /*= -1*/) const
{
	CSelector s = Selector::type(type);
	if(subtype != -1)
		s = s && Selector::subtype(subtype);

	return valOfBonuses(s);
}

int CBonusSystemNode::valOfBonuses(Bonus::BonusType type, const CSelector &selector) const
{
	return valOfBonuses(Selector::type(type) && selector);
}

int CBonusSystemNode::valOfBonuses(const CSelector &selector, const CBonusSystemNode *root/* = NULL*/) const
{
	BonusList hlp;
	getBonuses(hlp, selector, root);
	return hlp.totalValue();
}

bool CBonusSystemNode::hasBonus(const CSelector &selector, const CBonusSystemNode *root/* = NULL*/) const
{
	return getBonuses(selector).size() > 0;
}

bool CBonusSystemNode::hasBonusOfType(Bonus::BonusType type, int subtype /*= -1*/) const
{
	CSelector s = Selector::type(type);
	if(subtype != -1)
		s = s && Selector::subtype(subtype);

	return hasBonus(s);
}

Bonus * CBonusSystemNode::getBonus(const CSelector &selector)
{
	Bonus *ret = bonuses.getFirst(selector);
	if(ret)
		return ret;

	FOREACH_PARENT(p, this)
		if(ret = p->getBonus(selector))
			return ret;

	return NULL;
}

void CBonusSystemNode::getModifiersWDescr(TModDescr &out, Bonus::BonusType type, int subtype /*= -1 */) const
{
	getModifiersWDescr(out, Selector::typeSybtype(type, subtype));
}

void CBonusSystemNode::getModifiersWDescr(TModDescr &out, const CSelector &selector, const CBonusSystemNode *root /*= NULL*/) const
{
	getBonuses(selector).getModifiersWDescr(out);
}
int CBonusSystemNode::getBonusesCount(int from, int id) const
{
	return getBonusesCount(Selector::source(from, id));
}

int CBonusSystemNode::getBonusesCount(const CSelector &selector, const CBonusSystemNode *root/* = NULL*/) const
{
	return getBonuses(selector, root).size();
}

void CBonusSystemNode::getParents(TCNodes &out, const CBonusSystemNode *root) const /*retreives list of parent nodes (nodes to inherit bonuses from) */
{
	return;
}

void CBonusSystemNode::getParents(TNodes &out, const CBonusSystemNode *root /*= NULL*/)
{
	//de-constify above
	TCNodes hlp;
	getParents(hlp, root);
	BOOST_FOREACH(const CBonusSystemNode *pname, hlp)
		out.insert(const_cast<CBonusSystemNode*>(pname));
}

void CBonusSystemNode::getBonuses(BonusList &out, const CSelector &selector, const CBonusSystemNode *root /*= NULL*/) const
{
	bonuses.getBonuses(out, selector);
	FOREACH_CONST_PARENT(p, root ? root : this)
		p->getBonuses(out, selector, root ? root : this);
}

BonusList CBonusSystemNode::getBonuses(const CSelector &selector, const CBonusSystemNode *root /*= NULL*/) const
{
	BonusList ret;
	getBonuses(ret, selector, root);
	return ret;
}

void CBonusSystemNode::getBonuses(BonusList &out, const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root /*= NULL*/) const
{
	bonuses.getBonuses(out, selector, limit);
	FOREACH_CONST_PARENT(p, root ? root : this)
		p->getBonuses(out, selector, limit, root ? root : this);
}

BonusList CBonusSystemNode::getBonuses(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root /*= NULL*/) const
{
	BonusList ret;
	getBonuses(ret, selector, limit, root);
	return ret;
}

bool CBonusSystemNode::hasBonusFrom(ui8 source, ui32 sourceID) const
{
	return hasBonus(Selector::source(source,sourceID));
}

int CBonusSystemNode::MoraleVal() const
{
	if(hasBonusOfType(Bonus::NON_LIVING) || hasBonusOfType(Bonus::UNDEAD) ||
		hasBonusOfType(Bonus::NO_MORALE) || hasBonusOfType(Bonus::SIEGE_WEAPON))
		return 0;

	int ret = valOfBonuses(Selector::type(Bonus::MORALE));

	if(hasBonusOfType(Bonus::SELF_MORALE)) //eg. minotaur
		amax(ret, +1);

	return abetw(ret, -3, +3);
}

int CBonusSystemNode::LuckVal() const
{
	if(hasBonusOfType(Bonus::NO_LUCK))
		return 0;

	int ret = valOfBonuses(Selector::type(Bonus::LUCK));
	
	if(hasBonusOfType(Bonus::SELF_LUCK)) //eg. halfling
		amax(ret, +1);

	return abetw(ret, -3, +3);
}

si32 CBonusSystemNode::Attack() const
{
	si32 ret = valOfBonuses(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK);

	if(int frenzyPower = valOfBonuses(Bonus::IN_FRENZY)) //frenzy for attacker
	{
		ret += frenzyPower * Defense(false);
	}

	return ret;
}

si32 CBonusSystemNode::Defense(bool withFrenzy /*= true*/) const
{
	si32 ret = valOfBonuses(Bonus::PRIMARY_SKILL, PrimarySkill::DEFENSE);

	if(withFrenzy && hasBonusOfType(Bonus::IN_FRENZY)) //frenzy for defender
	{
		return 0;
	}

	return ret;
}

ui16 CBonusSystemNode::MaxHealth() const
{
	return valOfBonuses(Bonus::STACK_HEALTH);
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
		return &VLC->spellh->spells[id];
	return NULL;
}

std::string Bonus::Description() const
{
	if(description.size())
		return description;

	std::ostringstream str;
	if(val < 0)
		str << '-';
	else if(val > 0)
		str << '+';

	str << val << " ";

	switch(source)
	{
	case CREATURE_ABILITY:
		str << VLC->creh->creatures[id]->namePl;
		break;
	}
	
	return str.str();
}

CSelector DLL_EXPORT operator&&(const CSelector &first, const CSelector &second)
{
	return CSelectorsConjunction(first, second);
}


namespace Selector
{
	DLL_EXPORT CSelectFieldEqual<TBonusType> type(&Bonus::type, 0);
	DLL_EXPORT CSelectFieldEqual<TBonusSubtype> subtype(&Bonus::subtype, 0);
	DLL_EXPORT CSelectFieldEqual<si32> info(&Bonus::additionalInfo, 0);
	DLL_EXPORT CSelectFieldEqual<ui8> sourceType(&Bonus::source, 0);
	DLL_EXPORT CSelectFieldEqual<ui8> effectRange(&Bonus::effectRange, Bonus::NO_LIMIT);
	DLL_EXPORT CWillLastTurns turns;;

	CSelector DLL_EXPORT typeSybtype(TBonusType Type, TBonusSubtype Subtype)
	{
		return type(Type) && subtype(Subtype);
	}

	CSelector DLL_EXPORT typeSybtypeInfo(TBonusType type, TBonusSubtype subtype, si32 info)
	{
		return CSelectFieldEqual<TBonusType>(&Bonus::type, type) && CSelectFieldEqual<TBonusSubtype>(&Bonus::subtype, subtype) && CSelectFieldEqual<si32>(&Bonus::additionalInfo, info);
	}

	CSelector DLL_EXPORT source(ui8 source, ui32 sourceID)
	{
		return CSelectFieldEqual<ui8>(&Bonus::source, source) && CSelectFieldEqual<ui32>(&Bonus::id, sourceID);
	}

	bool DLL_EXPORT matchesType(const CSelector &sel, TBonusType type)
	{
		Bonus dummy;
		dummy.type = type;
		return sel(dummy);
	}

	bool DLL_EXPORT matchesTypeSubtype(const CSelector &sel, TBonusType type, TBonusSubtype subtype)
	{
		Bonus dummy;
		dummy.type = type;
		dummy.subtype = subtype;
		return sel(dummy);
	}
}