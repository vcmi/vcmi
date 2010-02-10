#define VCMI_DLL
#include "HeroBonus.h"

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