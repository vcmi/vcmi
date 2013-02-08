#include "StdInc.h"
#include "CCreatureSet.h"

#include "CCreatureHandler.h"
#include "VCMI_Lib.h"
#include "CModHandler.h"
#include "CObjectHandler.h"
#include "IGameCallback.h"
#include "CGameState.h"
#include "CGeneralTextHandler.h"
#include "CSpellHandler.h"
#include "CHeroHandler.h"

/*
 * CCreatureSet.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

const CStackInstance &CCreatureSet::operator[](TSlot slot) const
{
	TSlots::const_iterator i = stacks.find(slot);
	if (i != stacks.end())
		return *i->second;
	else
		throw std::runtime_error("That slot is empty!");
}

const CCreature* CCreatureSet::getCreature(TSlot slot) const
{
	TSlots::const_iterator i = stacks.find(slot);
	if (i != stacks.end())
		return i->second->type;
	else
		return NULL;
}

bool CCreatureSet::setCreature(TSlot slot, CreatureID::CreatureID type, TQuantity quantity) /*slots 0 to 6 */
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

TSlot CCreatureSet::getSlotFor(CreatureID::CreatureID creature, ui32 slotsAmount/*=7*/) const /*returns -1 if no slot available */
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

TSlot CCreatureSet::getFreeSlot(ui32 slotsAmount/*=ARMY_SIZE*/) const
{
	for (ui32 i = 0; i < slotsAmount; i++)
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

TExpType CCreatureSet::getStackExperience(TSlot slot) const
{
	TSlots::const_iterator i = stacks.find(slot);
	if (i != stacks.end())
		return i->second->experience;
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

void CCreatureSet::addToSlot(TSlot slot, CreatureID::CreatureID cre, TQuantity count, bool allowMerging/* = true*/)
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

ui64 CCreatureSet::getArmyStrength() const
{
	ui64 ret = 0;
	for(TSlots::const_iterator i = stacks.begin(); i != stacks.end(); i++)
		ret += i->second->getPower();
	return ret;
}

ui64 CCreatureSet::getPower (TSlot slot) const
{
	return getStack(slot).getPower();
}
std::string CCreatureSet::getRoughAmount (TSlot slot) const
{
	int quantity = CCreature::getQuantityID(getStackCount(slot));
	if (quantity)
		return VLC->generaltexth->arraytxt[174 + 3*CCreature::getQuantityID(getStackCount(slot))];
	return "";
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
void CCreatureSet::setStackExp(TSlot slot, TExpType exp)
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

const CStackInstance& CCreatureSet::getStack(TSlot slot) const
{
	assert(hasStackAtSlot(slot));
	return *getStackPtr(slot);
}

const CStackInstance* CCreatureSet::getStackPtr(TSlot slot) const
{
	if(hasStackAtSlot(slot))
		return stacks.find(slot)->second;
	else return NULL;
}

void CCreatureSet::eraseStack(TSlot slot)
{
	assert(hasStackAtSlot(slot));
	CStackInstance *toErase = detachStack(slot);
	vstd::clear_pointer(toErase);
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
	auto h = dynamic_cast<const CGHeroInstance *>(this);
	if (h && h->commander == stack)
		return -2;

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
	assert(slot < GameConstants::ARMY_SIZE);
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
	vstd::clear_pointer(stack);
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
		int freeSlots = stacksCount() - GameConstants::ARMY_SIZE;
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
		CCreatureSet cres;

		//get types of creatures that need their own slot
		for(TSlots::const_iterator i = cs.stacks.begin(); i != cs.stacks.end(); i++)
			cres.addToSlot(i->first, i->second->type->idNumber, 1, true);
		TSlot j;
		for(TSlots::const_iterator i = stacks.begin(); i != stacks.end(); i++)
		{
			if ((j = cres.getSlotFor(i->second->type)) >= 0)
				cres.addToSlot(j, i->second->type->idNumber, 1, true);  //merge if possible
			else
				return false; //no place found
		}
		return true; //all stacks found their slots
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

CStackInstance::CStackInstance(CreatureID::CreatureID id, TQuantity Count)
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

void CStackInstance::setType(CreatureID::CreatureID creID)
{
	if(creID >= 0 && creID < VLC->creh->creatures.size())
		setType(VLC->creh->creatures[creID]);
	else
		setType((const CCreature*)NULL);
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
				case Bonus::NO_MELEE_PENALTY:
				case Bonus::NO_DISTANCE_PENALTY:
				case Bonus::NO_WALL_PENALTY:
				case Bonus::JOUSTING: //TODO: percent bonus?
				case Bonus::RETURN_AFTER_STRIKE:
				case Bonus::BLOCKS_RETALIATION:
				case Bonus::TWO_HEX_ATTACK_BREATH:
				case Bonus::THREE_HEADED_ATTACK:
				case Bonus::ATTACKS_ALL_ADJACENT:
				case Bonus::ADDITIONAL_ATTACK: //TODO: what with more than one attack? Axe of Ferocity for example
				case Bonus::FULL_HP_REGENERATION:
				case Bonus::MANA_DRAIN:
				case Bonus::LIFE_DRAIN:
				case Bonus::REBIRTH:
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
				case Bonus::FIRE_IMMUNITY:
				case Bonus::WATER_IMMUNITY:
				case Bonus::AIR_IMMUNITY:
				case Bonus::EARTH_IMMUNITY:
				case Bonus::RECEPTIVE:
				case Bonus::DIRECT_DAMAGE_IMMUNITY:
				break;
				//One numeric value. magic resistance handled separately
				case Bonus::SPELL_RESISTANCE_AURA:
				case Bonus::SPELL_DAMAGE_REDUCTION:
				case Bonus::LEVEL_SPELL_IMMUNITY:
				case Bonus::HP_REGENERATION:
				case Bonus::ADDITIONAL_RETALIATION:
				case Bonus::DEFENSIVE_STANCE:
				case Bonus::DOUBLE_DAMAGE_CHANCE:
				case Bonus::DARKNESS: //Darkness Dragons any1?
					boost::algorithm::replace_first(text, "%d", boost::lexical_cast<std::string>(valOfBonuses(Selector::typeSubtype(bonus->type, bonus->subtype))));
					break;
				//Complex descriptions
				//case Bonus::SECONDARY_SKILL_PREMY: //only if there's no simple MR
				//	if (bonus->subtype == CGHeroInstance::RESISTANCE)
				//	{
				//		if (!hasBonusOfType(Bonus::MAGIC_RESISTANCE))
				//			boost::algorithm::replace_first(text, "%d", boost::lexical_cast<std::string>( magicResistance() ));
				//	}
				//	break;
				//case Bonus::MAGIC_RESISTANCE:
				//		boost::algorithm::replace_first(text, "%d", boost::lexical_cast<std::string>( magicResistance() ));
				//	break;
				case Bonus::HATE:
					boost::algorithm::replace_first(text, "%d", boost::lexical_cast<std::string>(valOfBonuses(Selector::typeSubtype(bonus->type, bonus->subtype))));
					boost::algorithm::replace_first(text, "%s", VLC->creh->creatures[bonus->subtype]->namePl);
					break;
				case Bonus::SPELL_AFTER_ATTACK:
				case Bonus::SPELL_BEFORE_ATTACK:
				{
					boost::algorithm::replace_first(text, "%d", boost::lexical_cast<std::string>(valOfBonuses(Selector::typeSubtype(bonus->type, bonus->subtype))));
					boost::algorithm::replace_first(text, "%s", VLC->spellh->spells[bonus->subtype]->name);
					break;
				}
				case Bonus::SPELL_LIKE_ATTACK:
				{
					boost::algorithm::replace_first(text, "%s", VLC->spellh->spells[bonus->subtype]->name);
					break;
				}
				default:
					{}//TODO: allow custom bonus types... someday, somehow
			}
		}
		else //short name
		{
			text = it->second.first;
			switch (bonus->type)
			{
				case Bonus::MANA_CHANNELING:
				case Bonus::MAGIC_MIRROR:
				case Bonus::CHANGES_SPELL_COST_FOR_ALLY:
				case Bonus::CHANGES_SPELL_COST_FOR_ENEMY:
				case Bonus::ENEMY_DEFENCE_REDUCTION:
				case Bonus::REBIRTH:
				case Bonus::DEATH_STARE:
				case Bonus::LIFE_DRAIN:
					boost::algorithm::replace_first(text, "%d", boost::lexical_cast<std::string>(valOfBonuses(Selector::typeSubtype(bonus->type, bonus->subtype))));
					break;
				case Bonus::HATE:
				case Bonus::DAEMON_SUMMONING:
					boost::algorithm::replace_first(text, "%s", VLC->creh->creatures[bonus->subtype]->namePl);
					break;
				case Bonus::LEVEL_SPELL_IMMUNITY:
					boost::algorithm::replace_first(text, "%d", boost::lexical_cast<std::string>(bonus->val));
					break;
				case Bonus::SPELL_AFTER_ATTACK:
				case Bonus::SPELL_BEFORE_ATTACK:
				case Bonus::SPELL_IMMUNITY:
				case Bonus::SPELLCASTER:
				case Bonus::ENCHANTER:
				case Bonus::ENCHANTED:
					boost::algorithm::replace_first(text, "%s", VLC->spellh->spells[bonus->subtype]->name);
					break;
				case Bonus::MAGIC_RESISTANCE:
					text = ""; //handled separately
					break;
				//case Bonus::SECONDARY_SKILL_PREMY:
				//	if (bonus->subtype != CGHeroInstance::RESISTANCE || hasBonusOfType(Bonus::MAGIC_RESISTANCE)) //handle it there
				//	text = "";
				//	break;
			}
		}
		return text;
	}
	else
		return "";
}

std::string CStackInstance::bonusToGraphics(Bonus *bonus) const
{
	std::string fileName;
	switch (bonus->type)
	{
			//"E_ALIVE.bmp"
			//"E_ART.bmp"
		case Bonus::ENCHANTED:
			fileName = "E_BLESS.bmp"; break;
			//"E_BLOCK.bmp"
			//"E_BLOCK1.bmp"
			//"E_BLOCK2.bmp"
		case Bonus::TWO_HEX_ATTACK_BREATH:
			fileName = "E_BREATH.bmp"; break;
		case Bonus::SPELL_AFTER_ATTACK:
			fileName = "E_CAST.bmp"; break;
		case Bonus::ENCHANTER:
			fileName = "E_CAST1.bmp"; break;
		case Bonus::RANDOM_SPELLCASTER:
			fileName = "RandomBoost.bmp"; break;
		case Bonus::SPELL_BEFORE_ATTACK:
			fileName ="E_CAST2.bmp"; break;
		case Bonus::SPELLCASTER:
			fileName = "E_CASTER.bmp"; break;
		case Bonus::JOUSTING:
			fileName = "E_CHAMP.bmp"; break;
		case Bonus::DOUBLE_DAMAGE_CHANCE:
			fileName = "E_DBLOW.bmp"; break;
		case Bonus::DEATH_STARE:
			fileName = "E_DEATH.bmp"; break;
		case Bonus::DEFENSIVE_STANCE:
			fileName = "E_DEFBON.bmp"; break;
		case Bonus::NO_DISTANCE_PENALTY:
			fileName = "E_DIST.bmp"; break;
		case Bonus::ADDITIONAL_ATTACK:
			fileName = "E_DOUBLE.bmp"; break;
		case Bonus::DRAGON_NATURE:
			fileName = "E_DRAGON.bmp"; break;
		case Bonus::MAGIC_RESISTANCE:
			fileName = "E_DWARF.bmp"; break;
		case Bonus::SECONDARY_SKILL_PREMY:
			if (bonus->subtype == SecondarySkill::RESISTANCE)
			{
				fileName = "E_DWARF.bmp";
			}
			break;
		case Bonus::FEAR:
			fileName = "E_FEAR.bmp"; break;
		case Bonus::FEARLESS:
			fileName = "E_FEARL.bmp"; break;
		case Bonus::FLYING:
			fileName = "E_FLY.bmp"; break;
		case Bonus::SPELL_DAMAGE_REDUCTION:
			fileName = "E_GOLEM.bmp"; break;
		case Bonus::RETURN_AFTER_STRIKE:
			fileName = "E_HARPY.bmp"; break;
		case Bonus::HATE:
			fileName = "E_HATE.bmp"; break;
		case Bonus::KING1:
			fileName = "E_KING1.bmp"; break;
		case Bonus::KING2:
			fileName = "E_KING2.bmp"; break;
		case Bonus::KING3:
			fileName = "E_KING3.bmp"; break;
		case Bonus::CHANGES_SPELL_COST_FOR_ALLY:
			fileName = "E_MANA.bmp"; break;
		case Bonus::CHANGES_SPELL_COST_FOR_ENEMY:
			fileName = "MagicDamper.bpm"; break;
		case Bonus::NO_MELEE_PENALTY:
			fileName = "E_MELEE.bmp"; break;
		case Bonus::MIND_IMMUNITY:
			fileName = "E_MIND.bmp"; break;
		case Bonus::SELF_MORALE:
			fileName = "E_MINOT.bmp"; break;
		case Bonus::NO_MORALE:
			fileName = "E_MORAL.bmp"; break;
		case Bonus::RECEPTIVE:
			fileName = "E_NOFRIM.bmp"; break;
		case Bonus::NO_WALL_PENALTY:
			fileName = "E_OBST.bmp"; break;
		case Bonus::ENEMY_DEFENCE_REDUCTION:
			fileName = "E_RDEF.bmp"; break;
		case Bonus::REBIRTH:
			fileName = "E_REBIRTH.bmp"; break;
		case Bonus::BLOCKS_RETALIATION:
			fileName = "E_RETAIL.bmp"; break;
		case Bonus::UNLIMITED_RETALIATIONS:
		case Bonus::ADDITIONAL_RETALIATION:
			fileName = "E_RETAIL1.bmp"; break;
		case Bonus::ATTACKS_ALL_ADJACENT:
			fileName = "E_ROUND.bmp"; break;
			//"E_SGNUM.bmp"
			//"E_SGTYPE.bmp"
		case Bonus::SHOOTER:
			fileName = "E_SHOOT.bmp"; break;
		case Bonus::FREE_SHOOTING: //shooter is not blocked by enemy
			fileName = "E_SHOOTA.bmp"; break;
			//"E_SHOOTN.bmp"
		case Bonus::SPELL_IMMUNITY:
		{
			switch (bonus->subtype)
			{
				case 62: //Blind
					fileName = "E_SPBLIND.bmp"; break;
				case 35: // Dispell
					fileName = "E_SPDISP.bmp"; break;
				case 78: // Dispell beneficial spells
					fileName = "E_SPDISB.bmp"; break;
				case 60: //Hypnotize
					fileName = "E_SPHYPN.bmp"; break;
				case 18: //Implosion
					fileName = "E_SPIMP.bmp"; break;
				case 59: //Berserk
					fileName = "E_SPBERS.bmp"; break;
				case 23: //Meteor Shower
					fileName = "E_SPMET.bmp"; break;
				case 26: //Armageddon
					fileName = "E_SPARM.bmp"; break;
				case 54: //Slow
					fileName = "E_SPSLOW.bmp"; break;
				//TODO: some generic spell handling?
			}
			break;
		}
			//"E_SPAWILL.bmp"
		case Bonus::DIRECT_DAMAGE_IMMUNITY:
			fileName = "E_SPDIR.bmp"; break;
			//"E_SPDISB.bmp"
			//"E_SPDISP.bmp"
			//"E_SPEATH.bmp"
			//"E_SPEATH1.bmp"
		case Bonus::FIRE_IMMUNITY:
			switch (bonus->subtype)
			{
				case 0:
					fileName =  "E_SPFIRE.bmp"; break; //all
				case 1:
					fileName =  "E_SPFIRE1.bmp"; break; //not positive
				case 2:
					fileName =  "E_FIRE.bmp"; break; //direct damage
			}
			break;
		case Bonus::WATER_IMMUNITY:
			switch (bonus->subtype)
			{
				case 0:
					fileName =  "E_SPWATER.bmp"; break; //all
				case 1:
					fileName =  "E_SPWATER1.bmp"; break; //not positive
				case 2:
					fileName =  "E_SPCOLD.bmp"; break; //direct damage
			}
			break;
		case Bonus::AIR_IMMUNITY: 
			switch (bonus->subtype)
			{
				case 0:
					fileName =  "E_SPAIR.bmp"; break; //all
				case 1:
					fileName =  "E_SPAIR1.bmp"; break; //not positive
				case 2:
					fileName = "E_LIGHT.bmp"; break;//direct damage
			}
			break;
		case Bonus::EARTH_IMMUNITY: 
			switch (bonus->subtype)
			{
				case 0:
					fileName =  "E_SPEATH.bmp"; break; //all
				case 1:
				case 2: //no specific icon for direct damage immunity
					fileName =  "E_SPEATH1.bmp"; break; //not positive
			}
			break;
		case Bonus::LEVEL_SPELL_IMMUNITY:
		{
			if (vstd::iswithin(bonus->val, 1 , 5))
			{
				fileName = "E_SPLVL" + boost::lexical_cast<std::string>(bonus->val) + ".bmp";
			}
			break;
		}
			//"E_SUMMON.bmp"
			//"E_SUMMON1.bmp"
			//"E_SUMMON2.bmp"
		case Bonus::FULL_HP_REGENERATION:
		case Bonus::HP_REGENERATION:
			fileName = "E_TROLL.bmp"; break;
		case Bonus::UNDEAD:
			fileName = "E_UNDEAD.bmp"; break;
		case Bonus::SPELL_RESISTANCE_AURA:
			fileName = "E_UNIC.bmp"; break;
		case Bonus::THREE_HEADED_ATTACK:
			fileName = "ThreeHeaded.bmp"; break;
		case Bonus::DAEMON_SUMMONING:
			fileName = "RiseDemons.bmp"; break;
		case Bonus::CHARGE_IMMUNITY:
			fileName = "ChargeImmune.bmp"; break;
		case Bonus::HEALER:
			fileName = "Healer.bmp"; break;
		case Bonus::CATAPULT:
			fileName = "Catapult.bmp"; break;
		case Bonus::MANA_CHANNELING:
			fileName = "ManaChannel.bmp"; break;
		case Bonus::MANA_DRAIN:
			fileName = "ManaDrain.bmp"; break;
		case Bonus::LIFE_DRAIN:
			fileName = "DrainLife.bmp"; break;
	}
	if(!fileName.empty())
		fileName = "zvs/Lib1.res/" + fileName;
	return fileName;
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
	else if(idRand)
		oss << "[no type, idRand=" << idRand << "]";
	else
		oss << "[UNDEFINED TYPE]";

	return oss.str();
}

void CStackInstance::deserializationFix()
{
	const CCreature *backup = type;
	type = NULL;
		setType(backup);
	const CArmedInstance *armyBackup = _armyObj;
	_armyObj = NULL;
	setArmyObj(armyBackup);
	artDeserializationFix(this);
}

CreatureID::CreatureID CStackInstance::getCreatureID() const
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

CCommanderInstance::CCommanderInstance (CreatureID::CreatureID id)
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
	type = NULL;
	idRand = -1;
	_armyObj = NULL;
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
		getBonusList().remove_if (Bonus::UntilCommanderKilled);
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

void CCommanderInstance::levelUp ()
{
	level++;
	BOOST_FOREACH (auto bonus, VLC->creh->commanderLevelPremy)
	{ //grant all regular level-up bonuses
		accumulateBonus (*bonus);
	}
}

ArtBearer::ArtBearer CCommanderInstance::bearerType() const
{
	return ArtBearer::COMMANDER;
}

CStackBasicDescriptor::CStackBasicDescriptor()
{
	type = NULL;
	count = -1;
}

CStackBasicDescriptor::CStackBasicDescriptor(CreatureID::CreatureID id, TQuantity Count)
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

bool CSimpleArmy::setCreature(TSlot slot, CreatureID::CreatureID cre, TQuantity count)
{
	assert(!vstd::contains(army, slot));
	army[slot] = CStackBasicDescriptor(cre, count);
	return true;
}
