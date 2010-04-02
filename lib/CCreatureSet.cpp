#define VCMI_DLL
#include "CCreatureSet.h"
#include "../hch/CCreatureHandler.h"
#include "VCMI_Lib.h"
#include <assert.h>

const CStackInstance CCreatureSet::operator[](TSlot slot) const
{
	TSlots::const_iterator i = slots.find(slot);
	if (i != slots.end())
		return i->second;
	else
		throw std::string("That slot is empty!");
}

const CCreature* CCreatureSet::getCreature(TSlot slot) const /*workaround of map issue */
{
	TSlots::const_iterator i = slots.find(slot);
	if (i != slots.end())
		return i->second.type;
	else
		return NULL;
}

bool CCreatureSet::setCreature(TSlot slot, TCreature type, TQuantity quantity) /*slots 0 to 6 */
{
	slots[slot] = CStackInstance(type, quantity);  //brutal force
	if (quantity == 0)
		slots.erase(slot);

	if (slots.size() > 7) 
		return false;
	else 
		return true;
}

TSlot CCreatureSet::getSlotFor(TCreature creature, ui32 slotsAmount/*=7*/) const /*returns -1 if no slot available */
{
	for(TSlots::const_iterator i=slots.begin(); i!=slots.end(); ++i)
	{
		if(i->second.type->idNumber == creature)
		{
			return i->first; //if there is already such creature we return its slot id
		}
	}
	for(ui32 i=0; i<slotsAmount; i++)
	{
		if(slots.find(i) == slots.end())
		{
			return i; //return first free slot
		}
	}
	return -1; //no slot available
}

int CCreatureSet::getAmount(TSlot slot) const
{
	TSlots::const_iterator i = slots.find(slot);
	if (i != slots.end())
		return i->second.count;
	else
		return 0; //TODO? consider issuing a warning
}

bool CCreatureSet::mergableStacks(std::pair<TSlot, TSlot> &out, TSlot preferable /*= -1*/) /*looks for two same stacks, returns slot positions */
{
	//try to match creature to our preferred stack
	if(preferable >= 0  &&  vstd::contains(slots, preferable))
	{
		const CCreature *cr = slots[preferable].type;
		for(TSlots::const_iterator j=slots.begin(); j!=slots.end(); ++j)
		{
			if(cr == j->second.type && j->first != preferable)
			{
				out.first = preferable;
				out.second = j->first;
				return true;
			}
		}
	}

	for(TSlots::const_iterator i=slots.begin(); i!=slots.end(); ++i)
	{
		for(TSlots::const_iterator j=slots.begin(); j!=slots.end(); ++j)
		{
			if(i->second.type == j->second.type  &&  i->first != j->first)
			{
				out.first = i->first;
				out.second = j->first;
				return true;
			}
		}
	}
	return false;
}

void CCreatureSet::sweep()
{
	for(TSlots::iterator i=slots.begin(); i!=slots.end(); ++i)
	{
		if(!i->second.count)
		{
			slots.erase(i);
			sweep();
			break;
		}
	}
}

void CCreatureSet::addToSlot(TSlot slot, TCreature cre, TQuantity count)
{
	assert(slot >= 0);
	const CCreature *c = &VLC->creh->creatures[cre];
	assert(!vstd::contains(slots, slot) || slots[slot].type == c); //that slot was empty or contained same type creature
	slots[slot].type = c;
	slots[slot].count += count;
}

void CCreatureSet::addToSlot(TSlot slot, const CStackInstance &stack)
{
	addToSlot(slot, stack.type->idNumber, stack.count);
}

bool CCreatureSet::validTypes(bool allowUnrandomized /*= false*/) const
{
	for(TSlots::const_iterator i=slots.begin(); i!=slots.end(); ++i)
	{
		bool isRand = (i->second.idRand != -1);
		if(!isRand)
		{
			assert(i->second.type);
			assert(i->second.type == &VLC->creh->creatures[i->second.type->idNumber]);
		}
		else
			assert(allowUnrandomized);
	}
	return true;
}

CStackInstance::CStackInstance()
{
	init();
}

CStackInstance::CStackInstance(TCreature id, TQuantity Count)
{
	init();
	setType(id);
	count = Count;
}

CStackInstance::CStackInstance(const CCreature *cre, TQuantity Count)
{
	init();
	type = cre;
	count = Count;
}

void CStackInstance::init()
{
	experience = 0;
	count = 0;
	type = NULL;
	idRand = -1;
}

int CStackInstance::getQuantityID() const 
{
	return CCreature::getQuantityID(count);
}

void CStackInstance::setType(int creID)
{
	type = &VLC->creh->creatures[creID];
}