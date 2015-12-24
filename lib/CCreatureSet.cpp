#include "StdInc.h"
#include "CCreatureSet.h"

#include "CCreatureHandler.h"
#include "VCMI_Lib.h"
#include "CModHandler.h"
#include "mapObjects/CGHeroInstance.h"
#include "IGameCallback.h"
#include "CGameState.h"
#include "CGeneralTextHandler.h"
#include "spells/CSpellHandler.h"
#include "CHeroHandler.h"
#include "IBonusTypeHandler.h"

/*
 * CCreatureSet.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

const CStackInstance &CCreatureSet::operator[](SlotID slot) const
{
	auto i = stacks.find(slot);
	if (i != stacks.end())
		return *i->second;
	else
		throw std::runtime_error("That slot is empty!");
}

const CCreature* CCreatureSet::getCreature(SlotID slot) const
{
	auto i = stacks.find(slot);
	if (i != stacks.end())
		return i->second->type;
	else
		return nullptr;
}

bool CCreatureSet::setCreature(SlotID slot, CreatureID type, TQuantity quantity) /*slots 0 to 6 */
{
	if(!slot.validSlot())
	{
        logGlobal->errorStream() << "Cannot set slot " << slot;
		return false;
	}
	if(!quantity)
	{
        logGlobal->warnStream() << "Using set creature to delete stack?";
		eraseStack(slot);
		return true;
	}

	if(hasStackAtSlot(slot)) //remove old creature
		eraseStack(slot);

	putStack(slot, new CStackInstance(type, quantity));
	return true;
}

SlotID CCreatureSet::getSlotFor(CreatureID creature, ui32 slotsAmount/*=7*/) const /*returns -1 if no slot available */
{
	return getSlotFor(VLC->creh->creatures[creature], slotsAmount);
}

SlotID CCreatureSet::getSlotFor(const CCreature *c, ui32 slotsAmount/*=ARMY_SIZE*/) const
{
	assert(c->valid());
	for(auto & elem : stacks)
	{
		assert(elem.second->type->valid());
		if(elem.second->type == c)
		{
			return elem.first; //if there is already such creature we return its slot id
		}
	}
	return getFreeSlot(slotsAmount);
}

SlotID CCreatureSet::getFreeSlot(ui32 slotsAmount/*=ARMY_SIZE*/) const
{
	for(ui32 i=0; i<slotsAmount; i++)
	{
		if(!vstd::contains(stacks, SlotID(i)))
		{
			return SlotID(i); //return first free slot
		}
	}
	return SlotID(); //no slot available
}

int CCreatureSet::getStackCount(SlotID slot) const
{
	auto i = stacks.find(slot);
	if (i != stacks.end())
		return i->second->count;
	else
		return 0; //TODO? consider issuing a warning
}

TExpType CCreatureSet::getStackExperience(SlotID slot) const
{
	auto i = stacks.find(slot);
	if (i != stacks.end())
		return i->second->experience;
	else
		return 0; //TODO? consider issuing a warning
}


bool CCreatureSet::mergableStacks(std::pair<SlotID, SlotID> &out, SlotID preferable /*= -1*/) const /*looks for two same stacks, returns slot positions */
{
	//try to match creature to our preferred stack
	if(preferable.validSlot() &&  vstd::contains(stacks, preferable))
	{
		const CCreature *cr = stacks.find(preferable)->second->type;
		for(auto & elem : stacks)
		{
			if(cr == elem.second->type && elem.first != preferable)
			{
				out.first = preferable;
				out.second = elem.first;
				return true;
			}
		}
	}

	for(auto i=stacks.begin(); i!=stacks.end(); ++i)
	{
		for(auto & elem : stacks)
		{
			if(i->second->type == elem.second->type  &&  i->first != elem.first)
			{
				out.first = i->first;
				out.second = elem.first;
				return true;
			}
		}
	}
	return false;
}

void CCreatureSet::sweep()
{
	for(auto i=stacks.begin(); i!=stacks.end(); ++i)
	{
		if(!i->second->count)
		{
			stacks.erase(i);
			sweep();
			break;
		}
	}
}

void CCreatureSet::addToSlot(SlotID slot, CreatureID cre, TQuantity count, bool allowMerging/* = true*/)
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
        logGlobal->errorStream() << "Failed adding to slot!";
	}
}

void CCreatureSet::addToSlot(SlotID slot, CStackInstance *stack, bool allowMerging/* = true*/)
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
        logGlobal->errorStream() << "Cannot add to slot " << slot << " stack " << *stack;
	}
}

bool CCreatureSet::validTypes(bool allowUnrandomized /*= false*/) const
{
	for(auto & elem : stacks)
	{
		if(!elem.second->valid(allowUnrandomized))
			return false;
	}
	return true;
}

bool CCreatureSet::slotEmpty(SlotID slot) const
{
	return !hasStackAtSlot(slot);
}

bool CCreatureSet::needsLastStack() const
{
	return false;
}

ui64 CCreatureSet::getArmyStrength() const
{
	ui64 ret = 0;
	for(auto & elem : stacks)
		ret += elem.second->getPower();
	return ret;
}

ui64 CCreatureSet::getPower (SlotID slot) const
{
	return getStack(slot).getPower();
}

std::string CCreatureSet::getRoughAmount(SlotID slot, int mode) const
{
	/// Mode represent return string format
	/// "Pack" - 0, "A pack of" - 1, "a pack of" - 2
	int quantity = CCreature::getQuantityID(getStackCount(slot));
	if(quantity)
		return VLC->generaltexth->arraytxt[(174 + mode) + 3*CCreature::getQuantityID(getStackCount(slot))];
	return "";
}

std::string CCreatureSet::getArmyDescription() const
{
	std::string text;
	std::vector<std::string> guards;
	for(auto & elem : stacks)
	{
		auto str = boost::str(boost::format("%s %s") % getRoughAmount(elem.first, 2) % getCreature(elem.first)->namePl);
		guards.push_back(str);
	}
	if(guards.size())
	{
		for(int i = 0; i < guards.size(); i++)
		{
			text += guards[i];
			if(i + 2 < guards.size())
				text += ", ";
			else if(i + 2 == guards.size())
				text += VLC->generaltexth->allTexts[237];
		}
	}
	return text;
}

int CCreatureSet::stacksCount() const
{
	return stacks.size();
}

void CCreatureSet::setFormation(bool tight)
{
	formation = tight;
}

void CCreatureSet::setStackCount(SlotID slot, TQuantity count)
{
	assert(hasStackAtSlot(slot));
	assert(stacks[slot]->count + count > 0);
	if (VLC->modh->modules.STACK_EXP && count > stacks[slot]->count)
		stacks[slot]->experience *= (count / static_cast<double>(stacks[slot]->count));
	stacks[slot]->count = count;
	armyChanged();
}

void CCreatureSet::giveStackExp(TExpType exp)
{
	for(TSlots::const_iterator i = stacks.begin(); i != stacks.end(); i++)
		i->second->giveStackExp(exp);
}
void CCreatureSet::setStackExp(SlotID slot, TExpType exp)
{
	assert(hasStackAtSlot(slot));
	stacks[slot]->experience = exp;
}

void CCreatureSet::clear()
{
	while(!stacks.empty())
	{
		eraseStack(stacks.begin()->first);
	}
}

const CStackInstance& CCreatureSet::getStack(SlotID slot) const
{
	assert(hasStackAtSlot(slot));
	return *getStackPtr(slot);
}

const CStackInstance* CCreatureSet::getStackPtr(SlotID slot) const
{
	if(hasStackAtSlot(slot))
		return stacks.find(slot)->second;
	else return nullptr;
}

void CCreatureSet::eraseStack(SlotID slot)
{
	assert(hasStackAtSlot(slot));
	CStackInstance *toErase = detachStack(slot);
	vstd::clear_pointer(toErase);
}

bool CCreatureSet::contains(const CStackInstance *stack) const
{
	if(!stack)
		return false;

	for(auto & elem : stacks)
		if(elem.second == stack)
			return true;

	return false;
}

SlotID CCreatureSet::findStack(const CStackInstance *stack) const
{
	auto h = dynamic_cast<const CGHeroInstance *>(this);
	if (h && h->commander == stack)
		return SlotID::COMMANDER_SLOT_PLACEHOLDER;

	if(!stack)
		return SlotID();

	for(auto & elem : stacks)
		if(elem.second == stack)
			return elem.first;

	return SlotID();
}

CArmedInstance * CCreatureSet::castToArmyObj()
{
	return dynamic_cast<CArmedInstance *>(this);
}

void CCreatureSet::putStack(SlotID slot, CStackInstance *stack)
{
	assert(slot.getNum() < GameConstants::ARMY_SIZE);
	assert(!hasStackAtSlot(slot));
	stacks[slot] = stack;
	stack->setArmyObj(castToArmyObj());
	armyChanged();
}

void CCreatureSet::joinStack(SlotID slot, CStackInstance * stack)
{
	const CCreature *c = getCreature(slot);
	assert(c == stack->type);
	assert(c);
	UNUSED(c);

	//TODO move stuff
	changeStackCount(slot, stack->count);
	vstd::clear_pointer(stack);
}

void CCreatureSet::changeStackCount(SlotID slot, TQuantity toAdd)
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
		auto i = src.army.begin();

		assert(i->second.type);
		assert(i->second.count);

		putStack(i->first, new CStackInstance(i->second.type, i->second.count));
		src.army.erase(i);
	}
}

CStackInstance * CCreatureSet::detachStack(SlotID slot)
{
	assert(hasStackAtSlot(slot));
	CStackInstance *ret = stacks[slot];

	//if(CArmedInstance *armedObj = castToArmyObj())
    if(ret)
	{
		ret->setArmyObj(nullptr); //detaches from current armyobj
        assert(!ret->armyObj); //we failed detaching?
	}

	stacks.erase(slot);
	armyChanged();
	return ret;
}

void CCreatureSet::setStackType(SlotID slot, const CCreature *type)
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
		int freeSlots = stacksCount() - GameConstants::ARMY_SIZE;
		std::set<const CCreature*> cresToAdd;
		for(auto & elem : cs.stacks)
		{
			SlotID dest = getSlotFor(elem.second->type);
			if(!dest.validSlot() || hasStackAtSlot(dest))
				cresToAdd.insert(elem.second->type);
		}
		return cresToAdd.size() <= freeSlots;
	}
	else
	{
		CCreatureSet cres;
		SlotID j;

		//get types of creatures that need their own slot
		for(auto & elem : cs.stacks)
			if ((j = cres.getSlotFor(elem.second->type)).validSlot())
				cres.addToSlot(j, elem.second->type->idNumber, 1, true);  //merge if possible
			//cres.addToSlot(elem.first, elem.second->type->idNumber, 1, true);
		for(auto & elem : stacks)
		{
			if ((j = cres.getSlotFor(elem.second->type)).validSlot())
				cres.addToSlot(j, elem.second->type->idNumber, 1, true);  //merge if possible
			else
				return false; //no place found
		}
		return true; //all stacks found their slots
	}
}

bool CCreatureSet::hasStackAtSlot(SlotID slot) const
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

CStackInstance::CStackInstance(CreatureID id, TQuantity Count)
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
	type = nullptr;
	idRand = -1;
	_armyObj = nullptr;
	setNodeType(STACK_INSTANCE);
}

int CStackInstance::getQuantityID() const
{
	return CCreature::getQuantityID(count);
}

int CStackInstance::getExpRank() const
{
	if (!VLC->modh->modules.STACK_EXP)
		return 0;
	int tier = type->level;
	if (vstd::iswithin(tier, 1, 7))
	{
		for (int i = VLC->creh->expRanks[tier].size()-2; i >-1; --i)//sic!
		{ //exp values vary from 1st level to max exp at 11th level
			if (experience >= VLC->creh->expRanks[tier][i])
				return ++i; //faster, but confusing - 0 index mean 1st level of experience
		}
		return 0;
	}
	else //higher tier
	{
		for (int i = VLC->creh->expRanks[0].size()-2; i >-1; --i)
		{
			if (experience >= VLC->creh->expRanks[0][i])
				return ++i;
		}
		return 0;
	}
}

int CStackInstance::getLevel() const
{
	return std::max (1, (int)type->level);
}

si32 CStackInstance::magicResistance() const
{
	si32 val = valOfBonuses(Selector::type(Bonus::MAGIC_RESISTANCE));
	if (const CGHeroInstance * hero = dynamic_cast<const CGHeroInstance *>(_armyObj))
	{
		//resistance skill
		val += hero->valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, SecondarySkill::RESISTANCE);
	}
	vstd::amin (val, 100);
	return val;
}

void CStackInstance::giveStackExp(TExpType exp)
{
	int level = type->level;
	if (!vstd::iswithin(level, 1, 7))
		level = 0;

	CCreatureHandler * creh = VLC->creh;
	ui32 maxExp = creh->expRanks[level].back();

	vstd::amin(exp, (TExpType)maxExp); //prevent exp overflow due to different types
	vstd::amin(exp, (maxExp * creh->maxExpPerBattle[level])/100);
	vstd::amin(experience += exp, maxExp); //can't get more exp than this limit
}

void CStackInstance::setType(CreatureID creID)
{
	if(creID >= 0 && creID < VLC->creh->creatures.size())
		setType(VLC->creh->creatures[creID]);
	else
		setType((const CCreature*)nullptr);
}

void CStackInstance::setType(const CCreature *c)
{
	if(type)
	{
		detachFrom(const_cast<CCreature*>(type));
		if (type->isMyUpgrade(c) && VLC->modh->modules.STACK_EXP)
			experience *= VLC->creh->expAfterUpgrade / 100.0;
	}

	type = c;
	if(type)
		attachTo(const_cast<CCreature*>(type));
}
std::string CStackInstance::bonusToString(const Bonus *bonus, bool description) const
{
	if(Bonus::MAGIC_RESISTANCE == bonus->type)
	{
		return "";
	}
	else
	{
		return VLC->getBth()->bonusToString(bonus, this, description);
	}
	
}

std::string CStackInstance::bonusToGraphics(const Bonus *bonus) const
{
	return VLC->getBth()->bonusToGraphics(bonus);
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

std::string CStackInstance::getQuantityTXT(bool capitalized /*= true*/) const
{
	int quantity = getQuantityID();

	if (quantity)
		return VLC->generaltexth->arraytxt[174 + quantity*3 - 1 - capitalized];
	else
		return "";
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
	oss << "Stack of " << count << " of ";
	if(type)
		oss << type->namePl;
	else if(idRand >= 0)
		oss << "[no type, idRand=" << idRand << "]";
	else
		oss << "[UNDEFINED TYPE]";

	return oss.str();
}

void CStackInstance::deserializationFix()
{
	const CCreature *backup = type;
	type = nullptr;
		setType(backup);
	const CArmedInstance *armyBackup = _armyObj;
	_armyObj = nullptr;
	setArmyObj(armyBackup);
	artDeserializationFix(this);
}

CreatureID CStackInstance::getCreatureID() const
{
	if(type)
		return type->idNumber;
	else
		return CreatureID::NONE;
}

std::string CStackInstance::getName() const
{
	return (count > 1) ? type->namePl : type->nameSing;
}

ui64 CStackInstance::getPower() const
{
	assert(type);
	return type->AIValue * count;
}

ArtBearer::ArtBearer CStackInstance::bearerType() const
{
	return ArtBearer::CREATURE;
}

CCommanderInstance::CCommanderInstance()
{
	init();
	name = "Unnamed";
}

CCommanderInstance::CCommanderInstance (CreatureID id)
{
	init();
	setType(id);
	name = "Commando"; //TODO - parse them
}

void CCommanderInstance::init()
{
	alive = true;
	experience = 0;
	level = 1;
	count = 1;
	type = nullptr;
	idRand = -1;
	_armyObj = nullptr;
	setNodeType (CBonusSystemNode::COMMANDER);
	secondarySkills.resize (ECommander::SPELL_POWER + 1);
}

CCommanderInstance::~CCommanderInstance()
{

}

void CCommanderInstance::setAlive (bool Alive)
{
	//TODO: helm of immortality
	alive = Alive;
	if (!alive)
	{
		popBonuses(Bonus::UntilCommanderKilled);
	}
}

void CCommanderInstance::giveStackExp (TExpType exp)
{
	if (alive)
		experience += exp;
}

int CCommanderInstance::getExpRank() const
{
	return VLC->heroh->level (experience);
}

int CCommanderInstance::getLevel() const
{
	return std::max (1, getExpRank());
}

void CCommanderInstance::levelUp ()
{
	level++;
	for (auto bonus : VLC->creh->commanderLevelPremy)
	{ //grant all regular level-up bonuses
		accumulateBonus (*bonus);
	}
}

ArtBearer::ArtBearer CCommanderInstance::bearerType() const
{
	return ArtBearer::COMMANDER;
}

bool CCommanderInstance::gainsLevel() const
{
	return experience >= VLC->heroh->reqExp(level+1);
}

CStackBasicDescriptor::CStackBasicDescriptor()
{
	type = nullptr;
	count = -1;
}

CStackBasicDescriptor::CStackBasicDescriptor(CreatureID id, TQuantity Count)
	: type (VLC->creh->creatures[id]), count(Count)
{
}

CStackBasicDescriptor::CStackBasicDescriptor(const CCreature *c, TQuantity Count)
	: type(c), count(Count)
{
}

DLL_LINKAGE std::ostream & operator<<(std::ostream & str, const CStackInstance & sth)
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

bool CSimpleArmy::setCreature(SlotID slot, CreatureID cre, TQuantity count)
{
	assert(!vstd::contains(army, slot));
	army[slot] = CStackBasicDescriptor(cre, count);
	return true;
}
