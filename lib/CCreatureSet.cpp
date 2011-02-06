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
#include "CSpellHandler.h"
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>

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

	if(hasStackAtSlot(slot)) //remove old creature
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

	if(!hasStackAtSlot(slot))
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

	if(!hasStackAtSlot(slot))
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
	return !hasStackAtSlot(slot);
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
	assert(hasStackAtSlot(slot));
	assert(count > 0);
	stacks[slot]->count = count;
	armyChanged();
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
	assert(hasStackAtSlot(slot));
	return *stacks.find(slot)->second;
}

void CCreatureSet::eraseStack(TSlot slot)
{
	assert(hasStackAtSlot(slot));
	CStackInstance *toErase = detachStack(slot);
	delNull(toErase);
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
	assert(!hasStackAtSlot(slot));
	stacks[slot] = stack;
	stack->setArmyObj(castToArmyObj());
	armyChanged();
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

void CCreatureSet::setToArmy(CSimpleArmy &src)
{
	clear();
	while(src)
	{
		TSimpleSlots::iterator i = src.army.begin();

		assert(i->second.type);
		assert(i->second.count);

		putStack(i->first, new CStackInstance(i->second.type, i->second.count));
		src.army.erase(i);
	}
}

CStackInstance * CCreatureSet::detachStack(TSlot slot)
{
	assert(hasStackAtSlot(slot));
	CStackInstance *ret = stacks[slot];

	//if(CArmedInstance *armedObj = castToArmyObj())
	{
		ret->setArmyObj(NULL); //detaches from current armyobj
	}

	assert(!ret->armyObj); //we failed detaching?
	stacks.erase(slot);
	armyChanged();
	return ret;
}

void CCreatureSet::setStackType(TSlot slot, const CCreature *type)
{
	assert(hasStackAtSlot(slot));
	CStackInstance *s = stacks[slot];
	s->setType(type->idNumber);
	armyChanged();
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

void CCreatureSet::armyChanged()
{
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
	setType(cre);
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
std::string CStackInstance::bonusToString(Bonus *bonus, bool description) const
{
	std::map<TBonusType, std::pair<std::string, std::string> >::iterator it = VLC->creh->stackBonuses.find(bonus->type);
	if (it != VLC->creh->stackBonuses.end())
	{
		std::string text;
		if (description) //long ability description
		{
			text = it->second.second;
			switch (bonus->type)
			{
				//no additional modifiers needed
				case Bonus::FLYING:
				case Bonus::UNLIMITED_RETALIATIONS:
				case Bonus::SHOOTER:
				case Bonus::FREE_SHOOTING:
				case Bonus::NO_SHOTING_PENALTY:
				case Bonus::NO_MELEE_PENALTY:
				case Bonus::NO_DISTANCE_PENALTY:
				case Bonus::NO_OBSTACLES_PENALTY:
				case Bonus::JOUSTING: //TODO: percent bonus?
				case Bonus::RETURN_AFTER_STRIKE:
				case Bonus::BLOCKS_RETALIATION:
				case Bonus::TWO_HEX_ATTACK_BREATH:
				case Bonus::THREE_HEADED_ATTACK:
				case Bonus::ATTACKS_ALL_ADJACENT:
				case Bonus::FULL_HP_REGENERATION:
				case Bonus::LIFE_DRAIN: //TODO: chance, hp percentage?
				case Bonus::SELF_MORALE:
				case Bonus::SELF_LUCK:
				case Bonus::FEAR:
				case Bonus::FEARLESS:
				case Bonus::CHARGE_IMMUNITY:
				case Bonus::HEALER:
				case Bonus::CATAPULT:
				case Bonus::DRAGON_NATURE:
				case Bonus::NON_LIVING:
				case Bonus::UNDEAD:
				break;
				//One numeric value
				//case Bonus::STACKS_SPEED: //Do we need description for creature stats?
				//case Bonus::STACK_HEALTH:
				case Bonus::MAGIC_RESISTANCE:
				case Bonus::SPELL_DAMAGE_REDUCTION:
				case Bonus::LEVEL_SPELL_IMMUNITY:
				case Bonus::CHANGES_SPELL_COST_FOR_ALLY:
				case Bonus::CHANGES_SPELL_COST_FOR_ENEMY:
				case Bonus::MANA_CHANNELING:
				case Bonus::MANA_DRAIN:
				case Bonus::HP_REGENERATION:
				case Bonus::ADDITIONAL_RETALIATION:
				case Bonus::DOUBLE_DAMAGE_CHANCE:
				case Bonus::ENEMY_DEFENCE_REDUCTION:
				case Bonus::MAGIC_MIRROR:
				case Bonus::DARKNESS: //Darkness Dragons any1?
					boost::algorithm::replace_first(text, "%d", boost::lexical_cast<std::string>(bonus->val));
					break;
				//Complex descriptions
				case Bonus::HATE: //TODO: customize damage percent
					boost::algorithm::replace_first(text, "%s", VLC->creh->creatures[bonus->subtype]->namePl);
					break;
				case Bonus::SPELL_IMMUNITY:
					boost::algorithm::replace_first(text, "%s", VLC->spellh->spells[bonus->subtype]->name);
					break;
				case Bonus::SPELL_AFTER_ATTACK:
					boost::algorithm::replace_first(text, "%d", boost::lexical_cast<std::string>(bonus->additionalInfo % 100));
					boost::algorithm::replace_first(text, "%s", VLC->spellh->spells[bonus->subtype]->name);
					break;
				default:
					{}//TODO: allow custom bonus types... someday, somehow
			}
		}
		else //short name
		{
			text = it->second.first;
			switch (bonus->type)
			{
				case Bonus::HATE:
					boost::algorithm::replace_first(text, "%s", VLC->creh->creatures[bonus->subtype]->namePl);
					break;
				case Bonus::LEVEL_SPELL_IMMUNITY:
					boost::algorithm::replace_first(text, "%d", boost::lexical_cast<std::string>(bonus->val));
					break;
				case Bonus::SPELL_IMMUNITY:
				case Bonus::SPELL_AFTER_ATTACK:
					boost::algorithm::replace_first(text, "%s", VLC->spellh->spells[bonus->subtype]->name);
					break;
			}
		}
		return text;
	}
	else
		return "";
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

void CStackInstance::deserializationFix()
{
	setType(type);
	setArmyObj(armyObj);
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

void CSimpleArmy::clear()
{
	army.clear();
}

CSimpleArmy::operator bool() const
{
	return army.size();
}

bool CSimpleArmy::setCreature(TSlot slot, TCreature cre, TQuantity count)
{
	assert(!vstd::contains(army, slot));
	army[slot] = CStackBasicDescriptor(cre, count);
	return true;
}
