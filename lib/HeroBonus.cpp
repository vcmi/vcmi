#define VCMI_DLL
#include "HeroBonus.h"
#include <boost/foreach.hpp>

#define FOREACH_PARENT(pname) 	TCNodes parents; getParents(parents); BOOST_FOREACH(const CBonusSystemNode *pname, parents)

int BonusList::valOfBonuses( HeroBonus::BonusType type, int subtype /*= -1*/ ) const /*subtype -> subtype of bonus, if -1 then any */
{
	if(!this) //to avoid null-checking in maany places -> no bonus list means 0 bonus value
		return 0;

	int ret = 0;
	if(subtype == -1)
	{
		for(const_iterator i = begin(); i != end(); i++)
			if(i->type == type)
				ret += i->val;
	}
	else
	{
		for(const_iterator i = begin(); i != end(); i++)
			if(i->type == type && i->subtype == subtype)
				ret += i->val;
	}
	return ret;
}

bool BonusList::hasBonusOfType( HeroBonus::BonusType type, int subtype /*= -1*/ ) const 
{
	if(!this) //to avoid null-checking in maany places -> no bonus list means there is no searched bonus
		return 0;

	if(subtype == -1) //any subtype
	{
		for(const_iterator i = begin(); i != end(); i++)
			if(i->type == type)
				return true;
	}
	else //given subtype
	{
		for(const_iterator i = begin(); i != end(); i++)
			if(i->type == type && i->subtype == subtype)
				return true;
	}
	return false;
}

const HeroBonus * BonusList::getBonus( int from, int id ) const
{
	if(!this) //to avoid null-checking in maany places -> no bonus list means bonus cannot be retreived
		return NULL;

	for (const_iterator i = begin(); i != end(); i++)
		if(i->source == from  &&  i->id == id)
			return &*i;
	return NULL;
}

void BonusList::getModifiersWDescr( std::vector<std::pair<int,std::string> > &out, HeroBonus::BonusType type, int subtype /*= -1 */ ) const
{
	if(!this) //to avoid null-checking in maany places -> no bonus list means nothing has to be done here
		return;

	if(subtype == -1)
	{
		for(const_iterator i = begin(); i != end(); i++)
			if(i->type == type)
				out.push_back(std::make_pair(i->val, i->description));
	}
	else
	{
		for(const_iterator i = begin(); i != end(); i++)
			if(i->type == type && i->subtype == subtype)
				out.push_back(std::make_pair(i->val, i->description));
	}
}

int CBonusSystemNode::valOfBonuses(HeroBonus::BonusType type, int subtype /*= -1*/) const
{
	int ret = bonuses.valOfBonuses(type, subtype);

	FOREACH_PARENT(p)
		ret += p->valOfBonuses(type, subtype);

	return ret;
}

bool CBonusSystemNode::hasBonusOfType(HeroBonus::BonusType type, int subtype /*= -1*/) const
{
	if(!this) //to allow calls on NULL and avoid checking duplication
		return false; //if hero doesn't exist then bonus neither can

	if(bonuses.hasBonusOfType(type, subtype))
		return true;

	FOREACH_PARENT(p)
		if(p->hasBonusOfType(type, subtype))
			return true;

	return false;
}

const HeroBonus * CBonusSystemNode::getBonus(int from, int id) const
{
	return bonuses.getBonus(from, id);
}

void CBonusSystemNode::getModifiersWDescr(std::vector<std::pair<int,std::string> > &out, HeroBonus::BonusType type, int subtype /*= -1 */) const
{
	bonuses.getModifiersWDescr(out, type, subtype);

	FOREACH_PARENT(p)
		p->getModifiersWDescr(out, type, subtype);
}

int CBonusSystemNode::getBonusesCount(int from, int id) const
{
	int ret = 0;

	BOOST_FOREACH(const HeroBonus &hb, bonuses)
		if(hb.source == from  &&  hb.id == id)
			ret++;

	return ret;
}

void CBonusSystemNode::getParents(TCNodes &out, const CBonusSystemNode *source) const /*retreives list of parent nodes (nodes to inherit bonuses from) */
{
	return;
}

int NBonus::valOf(const CBonusSystemNode *obj, HeroBonus::BonusType type, int subtype /*= -1*/)
{
	if(obj)
		return obj->valOfBonuses(type, subtype);
	return 0;
}

bool NBonus::hasOfType(const CBonusSystemNode *obj, HeroBonus::BonusType type, int subtype /*= -1*/)
{
	if(obj)
		return obj->hasBonusOfType(type, subtype);
	return false;
}

const HeroBonus * NBonus::get(const CBonusSystemNode *obj, int from, int id)
{
	if(obj)
		return obj->getBonus(from, id);
	return NULL;
}

void NBonus::getModifiersWDescr(const CBonusSystemNode *obj, std::vector<std::pair<int,std::string> > &out, HeroBonus::BonusType type, int subtype /*= -1 */)
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