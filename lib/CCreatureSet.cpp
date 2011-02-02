#define VCMI_DLL
#include "CCreatureSet.h"
#include "../hch/CCreatureHandler.h"
#include "VCMI_Lib.h"
#include <assert.h>
#include "../hch/CObjectHandler.h"
#include "IGameCallback.h"
#include "CGameState.h"
#include "../hch/CGeneralTextHandler.h"
#include "../hch/CSpellHandler.h"
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>

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

	if (slots.size() > ARMY_SIZE) 
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

bool CCreatureSet::mergableStacks(std::pair<TSlot, TSlot> &out, TSlot preferable /*= -1*/) const /*looks for two same stacks, returns slot positions */
{
	//try to match creature to our preferred stack
	if(preferable >= 0  &&  vstd::contains(slots, preferable))
	{
		const CCreature *cr = slots.find(preferable)->second.type;
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

void CCreatureSet::addToSlot(TSlot slot, TCreature cre, TQuantity count, bool allowMerging/* = true*/)
{
	assert(slot >= 0);
	const CCreature *c = VLC->creh->creatures[cre];
	assert(!vstd::contains(slots, slot) || (slots[slot].type == c && allowMerging)); //that slot was empty or contained same type creature
	slots[slot].type = c;
	slots[slot].count += count;

	//TODO
	const CArmedInstance *armedObj = dynamic_cast<const CArmedInstance *>(this);
	if(armedObj && !slots[slot].armyObj)
		slots[slot].armyObj = armedObj;
}

void CCreatureSet::addToSlot(TSlot slot, const CStackInstance &stack, bool allowMerging/* = true*/)
{
	assert(stack.type == VLC->creh->creatures[stack.type->idNumber]);
	addToSlot(slot, stack.type->idNumber, stack.count, allowMerging	);
}

bool CCreatureSet::validTypes(bool allowUnrandomized /*= false*/) const
{
	for(TSlots::const_iterator i=slots.begin(); i!=slots.end(); ++i)
	{
		bool isRand = (i->second.idRand != -1);
		if(!isRand)
		{
			assert(i->second.type);
			assert(i->second.type == VLC->creh->creatures[i->second.type->idNumber]);
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
		ret += i->second.type->AIValue * i->second.count;
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

void CCreatureSet::addStack(TSlot slot, const CStackInstance &stack)
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
	slots[slot].count = count;
}

void CCreatureSet::clear()
{
	slots.clear();
}

const CStackInstance& CCreatureSet::getStack(TSlot slot) const
{
	assert(vstd::contains(slots, slot));
	return slots.find(slot)->second;
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
		if(&i->second == stack)
			return true;

	return false;
}

CStackInstance::CStackInstance()
{
	init();
}

CStackInstance::CStackInstance(TCreature id, TQuantity Count, const CArmedInstance *ArmyObj)
{
	init();
	setType(id);
	count = Count;
	armyObj = ArmyObj;
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
	type = VLC->creh->creatures[creID];
}

void CStackInstance::getParents(TCNodes &out, const CBonusSystemNode *source /*= NULL*/) const
{
	out.insert(type);

	if(source && source != this) //we should be root, if not - do not inherit anything
		return;

	if(armyObj)
		out.insert(armyObj);
	else
		out.insert(&IObjectInterface::cb->gameState()->globalEffects);
}
std::string CStackInstance::bonusToString(Bonus *bonus, bool description) const
{
	std::map<TBonusType, std::pair<std::string, std::string>>::iterator it = VLC->creh->stackBonuses.find(bonus->type);
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
					boost::algorithm::replace_first(text, "%s", VLC->spellh->spells[bonus->subtype].name);
					break;
				case Bonus::SPELL_AFTER_ATTACK:
					boost::algorithm::replace_first(text, "%d", boost::lexical_cast<std::string>(bonus->additionalInfo % 100));
					boost::algorithm::replace_first(text, "%s", VLC->spellh->spells[bonus->subtype].name);
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
					boost::algorithm::replace_first(text, "%s", VLC->spellh->spells[bonus->subtype].name);
					break;
			}
		}
		return text;
	}
	else
		return "";
}

std::string CStackInstance::getQuantityTXT(bool capitalized /*= true*/) const
{
	return VLC->generaltexth->arraytxt[174 + getQuantityID()*3 + 2 - capitalized];
}