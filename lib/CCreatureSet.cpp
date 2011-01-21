#define VCMI_DLL
#include "CCreatureSet.h"
#include "CCreatureHandler.h"
#include "VCMI_Lib.h"
#include <assert.h>
#include "CObjectHandler.h"
#include "IGameCallback.h"
#include "CGameState.h"
#include "CGeneralTextHandler.h"
#include <sstream>

const CStackInstance &CCreatureSet::operator[](TSlot slot) const
{
	TSlots::const_iterator i = stacks.find(slot);
	if (i != stacks.end())
		return *i->second;
	else
		throw std::string("That slot is empty!");
}

const CCreature* CCreatureSet::getCreature(TSlot slot) const
{
	TSlots::const_iterator i = stacks.find(slot);
	if (i != stacks.end())
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
		return true;
	}

	if(vstd::contains(stacks, slot)) //remove old creature
		eraseStack(slot);

	putStack(slot, new CStackInstance(type, quantity));
	return true;
}

TSlot CCreatureSet::getSlotFor(TCreature creature, ui32 slotsAmount/*=7*/) const /*returns -1 if no slot available */
{
	return getSlotFor(VLC->creh->creatures[creature], slotsAmount);
}

TSlot CCreatureSet::getSlotFor(const CCreature *c, ui32 slotsAmount/*=ARMY_SIZE*/) const
{
	assert(c->valid());
	for(TSlots::const_iterator i=stacks.begin(); i!=stacks.end(); ++i)
	{
		assert(i->second->type->valid());
		if(i->second->type == c)
		{
			return i->first; //if there is already such creature we return its slot id
		}
	}
	for(ui32 i=0; i<slotsAmount; i++)
	{
		if(stacks.find(i) == stacks.end())
		{
			return i; //return first free slot
		}
	}
	return -1; //no slot available
}

int CCreatureSet::getStackCount(TSlot slot) const
{
	TSlots::const_iterator i = stacks.find(slot);
	if (i != stacks.end())
		return i->second->count;
	else
		return 0; //TODO? consider issuing a warning
}

bool CCreatureSet::mergableStacks(std::pair<TSlot, TSlot> &out, TSlot preferable /*= -1*/) const /*looks for two same stacks, returns slot positions */
{
	//try to match creature to our preferred stack
	if(preferable >= 0  &&  vstd::contains(stacks, preferable))
	{
		const CCreature *cr = stacks.find(preferable)->second->type;
		for(TSlots::const_iterator j=stacks.begin(); j!=stacks.end(); ++j)
		{
			if(cr == j->second->type && j->first != preferable)
			{
				out.first = preferable;
				out.second = j->first;
				return true;
			}
		}
	}

	for(TSlots::const_iterator i=stacks.begin(); i!=stacks.end(); ++i)
	{
		for(TSlots::const_iterator j=stacks.begin(); j!=stacks.end(); ++j)
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
	for(TSlots::iterator i=stacks.begin(); i!=stacks.end(); ++i)
	{
		if(!i->second->count)
		{
			stacks.erase(i);
			sweep();
			break;
		}
	}
}

void CCreatureSet::addToSlot(TSlot slot, TCreature cre, TQuantity count, bool allowMerging/* = true*/)
{
	const CCreature *c = VLC->creh->creatures[cre];

	if(!vstd::contains(stacks, slot))
	{
		setCreature(slot, cre, count);
	}
	else if(getCreature(slot) == c && allowMerging) //that slot was empty or contained same type creature
	{
		setStackCount(slot, getStackCount(slot) + count);
	}
	else
	{
		tlog1 << "Failed adding to slot!\n";
	}
}

void CCreatureSet::addToSlot(TSlot slot, CStackInstance *stack, bool allowMerging/* = true*/)
{
	assert(stack->valid(true));

	if(!vstd::contains(stacks, slot))
	{
		putStack(slot, stack);
	}
	else if(allowMerging && stack->type == getCreature(slot))
	{
		joinStack(slot, stack);
	}
	else
	{
		tlog1 << "Cannot add to slot " << slot << " stack " << *stack << std::endl;
	}
}

bool CCreatureSet::validTypes(bool allowUnrandomized /*= false*/) const
{
	for(TSlots::const_iterator i=stacks.begin(); i!=stacks.end(); ++i)
	{
		if(!i->second->valid(allowUnrandomized))
			return false;
	}
	return true;
}

bool CCreatureSet::slotEmpty(TSlot slot) const
{
	return !vstd::contains(stacks, slot);
}

bool CCreatureSet::needsLastStack() const
{
	return false;
}

int CCreatureSet::getArmyStrength() const
{
	int ret = 0;
	for(TSlots::const_iterator i = stacks.begin(); i != stacks.end(); i++)
		ret += i->second->type->AIValue * i->second->count;
	return ret;
}

ui64 CCreatureSet::getPower (TSlot slot) const
{
	return getCreature(slot)->AIValue * getStackCount(slot);
}
std::string CCreatureSet::getRoughAmount (TSlot slot) const
{
	return VLC->generaltexth->arraytxt[174 + 3*CCreature::getQuantityID(getStackCount(slot))];
}

int CCreatureSet::stacksCount() const
{
	return stacks.size();
}

void CCreatureSet::setFormation(bool tight)
{
	formation = tight;
}

void CCreatureSet::setStackCount(TSlot slot, TQuantity count)
{
	assert(vstd::contains(stacks, slot));
	assert(count > 0);
	stacks[slot]->count = count;
}

void CCreatureSet::clear()
{
	while(!stacks.empty())
	{
		eraseStack(stacks.begin()->first);
	}
}

const CStackInstance& CCreatureSet::getStack(TSlot slot) const
{
	assert(vstd::contains(stacks, slot));
	return *stacks.find(slot)->second;
}

void CCreatureSet::eraseStack(TSlot slot)
{
	assert(vstd::contains(stacks, slot));
	delNull(stacks[slot]);
	stacks.erase(slot);
}

bool CCreatureSet::contains(const CStackInstance *stack) const
{
	if(!stack) 
		return false;

	for(TSlots::const_iterator i = stacks.begin(); i != stacks.end(); ++i)
		if(i->second == stack)
			return true;

	return false;
}

TSlot CCreatureSet::findStack(const CStackInstance *stack) const
{
	if(!stack) 
		return -1;

	for(TSlots::const_iterator i = stacks.begin(); i != stacks.end(); ++i)
		if(i->second == stack)
			return i->first;

	return -1;
}

CArmedInstance * CCreatureSet::castToArmyObj()
{
	return dynamic_cast<CArmedInstance *>(this);
}

void CCreatureSet::putStack(TSlot slot, CStackInstance *stack)
{
	assert(!vstd::contains(stacks, slot));
	stacks[slot] = stack;
	stack->setArmyObj(castToArmyObj());
}

void CCreatureSet::joinStack(TSlot slot, CStackInstance * stack)
{
	const CCreature *c = getCreature(slot);
	assert(c == stack->type);
	assert(c);

	//TODO move stuff 
	changeStackCount(slot, stack->count);
	delNull(stack);
}

void CCreatureSet::changeStackCount(TSlot slot, TQuantity toAdd)
{
	setStackCount(slot, getStackCount(slot) + toAdd);
}

CCreatureSet::CCreatureSet()
{
	formation = false;
}

CCreatureSet::CCreatureSet(const CCreatureSet&)
{
	assert(0);
}

CCreatureSet::~CCreatureSet()
{
	clear();
}

void CCreatureSet::setToArmy(CCreatureSet &src)
{
	clear();
	while(src)
	{
		TSlots::iterator i = src.stacks.begin();

		assert(i->second->type);
		assert(i->second->valid(false));
		assert(i->second->armyObj == NULL);

		putStack(i->first, i->second);
		src.stacks.erase(i);
	}
}

CStackInstance * CCreatureSet::detachStack(TSlot slot)
{
	assert(vstd::contains(stacks, slot));
	CStackInstance *ret = stacks[slot];

	if(CArmedInstance *armedObj = castToArmyObj())
	{
		ret->setArmyObj(NULL); //detaches from current armyobj
	}

	assert(!ret->armyObj); //we failed detaching?

	stacks.erase(slot);
	return ret;
}

void CCreatureSet::setStackType(TSlot slot, const CCreature *type)
{
	assert(vstd::contains(stacks, slot));
	CStackInstance *s = stacks[slot];
	s->setType(type->idNumber);
}

bool CCreatureSet::canBeMergedWith(const CCreatureSet &cs, bool allowMergingStacks) const
{
	if(!allowMergingStacks)
	{
		int freeSlots = stacksCount() - ARMY_SIZE;
		std::set<const CCreature*> cresToAdd;
		for(TSlots::const_iterator i = cs.stacks.begin(); i != cs.stacks.end(); i++)
		{
			TSlot dest = getSlotFor(i->second->type);
			if(dest < 0 || hasStackAtSlot(dest))
				cresToAdd.insert(i->second->type);
		}
		return cresToAdd.size() <= freeSlots;
	}
	else
	{
		std::set<const CCreature*> cres;

		//get types of creatures that need their own slot
		for(TSlots::const_iterator i = cs.stacks.begin(); i != cs.stacks.end(); i++)
			cres.insert(i->second->type);
		for(TSlots::const_iterator i = stacks.begin(); i != stacks.end(); i++)
			cres.insert(i->second->type);

		return cres.size() <= ARMY_SIZE;
	}
}

bool CCreatureSet::hasStackAtSlot(TSlot slot) const
{
	return vstd::contains(stacks, slot);
}

CCreatureSet & CCreatureSet::operator=(const CCreatureSet&cs)
{
	assert(0);
	return *this;
}

CStackInstance::CStackInstance()
	: armyObj(_armyObj)
{
	init();
}

CStackInstance::CStackInstance(TCreature id, TQuantity Count)
	: armyObj(_armyObj)
{
	init();
	setType(id);
	count = Count;
}

CStackInstance::CStackInstance(const CCreature *cre, TQuantity Count)
	: armyObj(_armyObj)
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
	_armyObj = NULL;
	nodeType = STACK;
}

int CStackInstance::getQuantityID() const 
{
	return CCreature::getQuantityID(count);
}

void CStackInstance::setType(int creID)
{
	setType(VLC->creh->creatures[creID]);
}

void CStackInstance::setType(const CCreature *c)
{
	if(type)
		detachFrom(const_cast<CCreature*>(type));

	type = c;

	attachTo(const_cast<CCreature*>(type));
}

void CStackInstance::setArmyObj(const CArmedInstance *ArmyObj)
{
	if(_armyObj)
		detachFrom(const_cast<CArmedInstance*>(_armyObj));

	_armyObj = ArmyObj;
	if(ArmyObj)
	{
		attachTo(const_cast<CArmedInstance*>(_armyObj));
	}
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

bool CStackInstance::valid(bool allowUnrandomized) const
{
	bool isRand = (idRand != -1);
	if(!isRand)
	{
		return (type  &&  type == VLC->creh->creatures[type->idNumber]);
	}
	else
		return allowUnrandomized;
}

CStackInstance::~CStackInstance()
{

}

std::string CStackInstance::nodeName() const
{
	std::ostringstream oss;
	oss << "Stack of " << count << " creatures of ";
	if(type)
		oss << type->namePl;
	else if(idRand)
		oss << "[no type, idRand=" << idRand << "]";
	else
		oss << "[UNDEFINED TYPE]";

	return oss.str();
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

CStackBasicDescriptor::CStackBasicDescriptor(const CCreature *c, TQuantity Count)
	: type(c), count(Count)
{
}

DLL_EXPORT std::ostream & operator<<(std::ostream & str, const CStackInstance & sth)
{
	if(!sth.valid(true))
		str << "an invalid stack!";

	str << "stack with " << sth.count << " of ";
	if(sth.type)
		str << sth.type->namePl;
	else
		str << sth.idRand;

	return str;
}