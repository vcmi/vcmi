#define VCMI_DLL
#include "CCreatureSet.h"
#include "../hch/CCreatureHandler.h"
#include "VCMI_Lib.h"
#include <assert.h>
#include "../hch/CObjectHandler.h"
#include "IGameCallback.h"
#include "CGameState.h"
#include "../hch/CGeneralTextHandler.h"

const CStackInstance &CCreatureSet::operator[](TSlot slot) const
{
	TSlots::const_iterator i = slots.find(slot);
	if (i != slots.end())
		return *i->second;
	else
		throw std::string("That slot is empty!");
}

const CCreature* CCreatureSet::getCreature(TSlot slot) const /*workaround of map issue */
{
	TSlots::const_iterator i = slots.find(slot);
	if (i != slots.end())
		return i->second->type;
	else
		return NULL;
}

bool CCreatureSet::setCreature(TSlot slot, TCreature type, TQuantity quantity) /*slots 0 to 6 */
{
	if(slot > 6 || slot < 0)
	{
		tlog1 << "Cannot set slot " << slot << std::endl;
		return false;
	}
	if(!quantity)
	{
		tlog2 << "Using set creature to delete stack?\n";
		eraseStack(slot);
	}

	if(vstd::contains(slots, slot)) //remove old creature
	{
		eraseStack(slot);
	}

	CStackInstance *stack = new CStackInstance(type, quantity);
	stack->armyObj = castToArmyObj(); //brutal force
	slots[slot] = stack;
}

TSlot CCreatureSet::getSlotFor(TCreature creature, ui32 slotsAmount/*=7*/) const /*returns -1 if no slot available */
{
	for(TSlots::const_iterator i=slots.begin(); i!=slots.end(); ++i)
	{
		if(i->second->type->idNumber == creature)
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
		return i->second->count;
	else
		return 0; //TODO? consider issuing a warning
}

bool CCreatureSet::mergableStacks(std::pair<TSlot, TSlot> &out, TSlot preferable /*= -1*/) const /*looks for two same stacks, returns slot positions */
{
	//try to match creature to our preferred stack
	if(preferable >= 0  &&  vstd::contains(slots, preferable))
	{
		const CCreature *cr = slots.find(preferable)->second->type;
		for(TSlots::const_iterator j=slots.begin(); j!=slots.end(); ++j)
		{
			if(cr == j->second->type && j->first != preferable)
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
			if(i->second->type == j->second->type  &&  i->first != j->first)
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
		if(!i->second->count)
		{
			slots.erase(i);
			sweep();
			break;
		}
	}
}

void CCreatureSet::addToSlot(TSlot slot, TCreature cre, TQuantity count, bool allowMerging/* = true*/)
{
	const CCreature *c = VLC->creh->creatures[cre];

	if(!vstd::contains(slots, slot))
	{
		setCreature(slot, cre, count);
	}
	else if(slots[slot]->type == c && allowMerging); //that slot was empty or contained same type creature
	{
		setStackCount(slot, slots[slot]->count + count);
	}
}

void CCreatureSet::addToSlot(TSlot slot, CStackInstance *stack, bool allowMerging/* = true*/)
{
	assert(stack->type == VLC->creh->creatures[stack->type->idNumber]);

	if(!vstd::contains(slots, slot))
	{
		slots[slot] = stack;
		stack->setArmyObj(castToArmyObj());
	}
	else
	{
		addToSlot(slot, stack->type->idNumber, stack->count, allowMerging);
	}
}

bool CCreatureSet::validTypes(bool allowUnrandomized /*= false*/) const
{
	for(TSlots::const_iterator i=slots.begin(); i!=slots.end(); ++i)
	{
		bool isRand = (i->second->idRand != -1);
		if(!isRand)
		{
			assert(i->second->type);
			assert(i->second->type == VLC->creh->creatures[i->second->type->idNumber]);
		}
		else
			assert(allowUnrandomized);
	}
	return true;
}

bool CCreatureSet::slotEmpty(TSlot slot) const
{
	return !vstd::contains(slots, slot);
}

bool CCreatureSet::needsLastStack() const
{
	return false;
}

int CCreatureSet::getArmyStrength() const
{
	int ret = 0;
	for(TSlots::const_iterator i = slots.begin(); i != slots.end(); i++)
		ret += i->second->type->AIValue * i->second->count;
	return ret;
}

ui64 CCreatureSet::getPower (TSlot slot) const
{
	return getCreature(slot)->AIValue * getAmount(slot);
}
std::string CCreatureSet::getRoughAmount (TSlot slot) const
{
	return VLC->generaltexth->arraytxt[174 + 3*CCreature::getQuantityID(getAmount(slot))];
}

int CCreatureSet::stacksCount() const
{
	return slots.size();
}

void CCreatureSet::addStack(TSlot slot, CStackInstance *stack)
{
	addToSlot(slot, stack, false);
}

void CCreatureSet::setFormation(bool tight)
{
	formation = tight;
}

void CCreatureSet::setStackCount(TSlot slot, TQuantity count)
{
	assert(vstd::contains(slots, slot));
	assert(count > 0);
	slots[slot]->count = count;
}

void CCreatureSet::clear()
{
	slots.clear();
}

const CStackInstance& CCreatureSet::getStack(TSlot slot) const
{
	assert(vstd::contains(slots, slot));
	return *slots.find(slot)->second;
}

void CCreatureSet::eraseStack(TSlot slot)
{
	assert(vstd::contains(slots, slot));
	slots.erase(slot);
}

bool CCreatureSet::contains(const CStackInstance *stack) const
{
	if(!stack) 
		return false;

	for(TSlots::const_iterator i = slots.begin(); i != slots.end(); ++i)
		if(i->second == stack)
			return true;

	return false;
}

TSlot CCreatureSet::findStack(const CStackInstance *stack) const
{
	if(!stack) 
		return -1;

	for(TSlots::const_iterator i = slots.begin(); i != slots.end(); ++i)
		if(i->second == stack)
			return i->first;

	return -1;
}

CArmedInstance * CCreatureSet::castToArmyObj()
{
	return dynamic_cast<CArmedInstance *>(this);
}

CStackInstance::CStackInstance()
{
	init();
}

CStackInstance::CStackInstance(TCreature id, TQuantity Count, const CArmedInstance *ArmyObj)
{
	init();
	setType(id);
	setArmyObj(ArmyObj);
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
	armyObj = NULL;
	nodeType = STACK;
}

int CStackInstance::getQuantityID() const 
{
	return CCreature::getQuantityID(count);
}

void CStackInstance::setType(int creID)
{
	if(type)
		detachFrom(const_cast<CCreature*>(type));

	type = VLC->creh->creatures[creID];

	attachTo(const_cast<CCreature*>(type));
}

void CStackInstance::setArmyObj(const CArmedInstance *ArmyObj)
{
	if(armyObj)
		detachFrom(const_cast<CArmedInstance*>(armyObj));

	armyObj = ArmyObj;

	attachTo(const_cast<CArmedInstance*>(armyObj));
}
// void CStackInstance::getParents(TCNodes &out, const CBonusSystemNode *source /*= NULL*/) const
// {
// 	out.insert(type);
// 
// 	if(source && source != this) //we should be root, if not - do not inherit anything
// 		return;
// 
// 	if(armyObj)
// 		out.insert(armyObj);
// 	else
// 		out.insert(&IObjectInterface::cb->gameState()->globalEffects);
// }

std::string CStackInstance::getQuantityTXT(bool capitalized /*= true*/) const
{
	return VLC->generaltexth->arraytxt[174 + getQuantityID()*3 + 2 - capitalized];
}

CStackBasicDescriptor::CStackBasicDescriptor()
{
	type = NULL;
	count = -1;
}

CStackBasicDescriptor::CStackBasicDescriptor(TCreature id, TQuantity Count)
	: type (VLC->creh->creatures[id]), count(Count)
{
}