/*
 * CCreatureSet.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CCreatureSet.h"

#include "ArtifactUtils.h"
#include "CConfigHandler.h"
#include "CCreatureHandler.h"
#include "VCMI_Lib.h"
#include "GameSettings.h"
#include "mapObjects/CGHeroInstance.h"
#include "modding/ModScope.h"
#include "IGameCallback.h"
#include "CGeneralTextHandler.h"
#include "spells/CSpellHandler.h"
#include "CHeroHandler.h"
#include "IBonusTypeHandler.h"
#include "serializer/JsonSerializeFormat.h"
#include "NetPacksBase.h"

#include <vcmi/FactionService.h>
#include <vcmi/Faction.h>

VCMI_LIB_NAMESPACE_BEGIN


bool CreatureSlotComparer::operator()(const TPairCreatureSlot & lhs, const TPairCreatureSlot & rhs)
{
	return lhs.first->getAIValue() < rhs.first->getAIValue(); // Descendant order sorting
}

const CStackInstance & CCreatureSet::operator[](const SlotID & slot) const
{
	auto i = stacks.find(slot);
	if (i != stacks.end())
		return *i->second;
	else
		throw std::runtime_error("That slot is empty!");
}

const CCreature * CCreatureSet::getCreature(const SlotID & slot) const
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
		logGlobal->error("Cannot set slot %d", slot.getNum());
		return false;
	}
	if(!quantity)
	{
		logGlobal->warn("Using set creature to delete stack?");
		eraseStack(slot);
		return true;
	}

	if(hasStackAtSlot(slot)) //remove old creature
		eraseStack(slot);

	auto * armyObj = castToArmyObj();
	bool isHypotheticArmy = armyObj ? armyObj->isHypothetic() : false;

	putStack(slot, new CStackInstance(type, quantity, isHypotheticArmy));
	return true;
}

SlotID CCreatureSet::getSlotFor(const CreatureID & creature, ui32 slotsAmount) const /*returns -1 if no slot available */
{
	return getSlotFor(VLC->creh->objects[creature], slotsAmount);
}

SlotID CCreatureSet::getSlotFor(const CCreature *c, ui32 slotsAmount) const
{
	assert(c && c->valid());
	for(const auto & elem : stacks)
	{
		assert(elem.second->type->valid());
		if(elem.second->type == c)
		{
			return elem.first; //if there is already such creature we return its slot id
		}
	}
	return getFreeSlot(slotsAmount);
}

bool CCreatureSet::hasCreatureSlots(const CCreature * c, const SlotID & exclude) const
{
	assert(c && c->valid());
	for(const auto & elem : stacks) // elem is const
	{
		if(elem.first == exclude) // Check slot
			continue;

		if(!elem.second || !elem.second->type) // Check creature
			continue;

		assert(elem.second->type->valid());

		if(elem.second->type == c)
			return true;
	}
	return false;
}

std::vector<SlotID> CCreatureSet::getCreatureSlots(const CCreature * c, const SlotID & exclude, TQuantity ignoreAmount) const
{
	assert(c && c->valid());
	std::vector<SlotID> result;

	for(const auto & elem : stacks)
	{
		if(elem.first == exclude)
			continue;

		if(!elem.second || !elem.second->type || elem.second->type != c)
			continue;

		if(elem.second->count == ignoreAmount || elem.second->count < 1)
			continue;

		assert(elem.second->type->valid());
		result.push_back(elem.first);
	}
	return result;
}

bool CCreatureSet::isCreatureBalanced(const CCreature * c, TQuantity ignoreAmount) const
{
	assert(c && c->valid());
	TQuantity max = 0;
	TQuantity min = std::numeric_limits<TQuantity>::max();

	for(const auto & elem : stacks)
	{
		if(!elem.second || !elem.second->type || elem.second->type != c)
			continue;

		const auto count = elem.second->count;

		if(count == ignoreAmount || count < 1)
			continue;

		assert(elem.second->type->valid());

		if(count > max)
			max = count;
		if(count < min)
			min = count;
		if(max - min > 1)
			return false;
	}
	return true;
}

SlotID CCreatureSet::getFreeSlot(ui32 slotsAmount) const
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

std::vector<SlotID> CCreatureSet::getFreeSlots(ui32 slotsAmount) const
{
	std::vector<SlotID> freeSlots;

	for(ui32 i = 0; i < slotsAmount; i++)
	{
		auto slot = SlotID(i);

		if(!vstd::contains(stacks, slot))
			freeSlots.push_back(slot);
	}
	return freeSlots;
}

std::queue<SlotID> CCreatureSet::getFreeSlotsQueue(ui32 slotsAmount) const
{
	std::queue<SlotID> freeSlots;

	for (ui32 i = 0; i < slotsAmount; i++)
	{
		auto slot = SlotID(i);

		if(!vstd::contains(stacks, slot))
			freeSlots.push(slot);
	}
	return freeSlots;
}

TMapCreatureSlot CCreatureSet::getCreatureMap() const
{
	TMapCreatureSlot creatureMap;
	TMapCreatureSlot::key_compare keyComp = creatureMap.key_comp();

	// https://stackoverflow.com/questions/97050/stdmap-insert-or-stdmap-find
	// https://www.cplusplus.com/reference/map/map/key_comp/
	for(const auto & pair : stacks)
	{
		const auto * creature = pair.second->type;
		auto slot = pair.first;
		auto lb = creatureMap.lower_bound(creature);

		if(lb != creatureMap.end() && !(keyComp(creature, lb->first)))
			continue;

		creatureMap.insert(lb, TMapCreatureSlot::value_type(creature, slot));
	}
	return creatureMap;
}

TCreatureQueue CCreatureSet::getCreatureQueue(const SlotID & exclude) const
{
	TCreatureQueue creatureQueue;

	for(const auto & pair : stacks)
	{
		if(pair.first == exclude)
			continue;
		creatureQueue.push(std::make_pair(pair.second->type, pair.first));
	}
	return creatureQueue;
}

TQuantity CCreatureSet::getStackCount(const SlotID & slot) const
{
	auto i = stacks.find(slot);
	if (i != stacks.end())
		return i->second->count;
	else
		return 0; //TODO? consider issuing a warning
}

TExpType CCreatureSet::getStackExperience(const SlotID & slot) const
{
	auto i = stacks.find(slot);
	if (i != stacks.end())
		return i->second->experience;
	else
		return 0; //TODO? consider issuing a warning
}

bool CCreatureSet::mergableStacks(std::pair<SlotID, SlotID> & out, const SlotID & preferable) const /*looks for two same stacks, returns slot positions */
{
	//try to match creature to our preferred stack
	if(preferable.validSlot() &&  vstd::contains(stacks, preferable))
	{
		const CCreature *cr = stacks.find(preferable)->second->type;
		for(const auto & elem : stacks)
		{
			if(cr == elem.second->type && elem.first != preferable)
			{
				out.first = preferable;
				out.second = elem.first;
				return true;
			}
		}
	}

	for(const auto & stack : stacks)
	{
		for(const auto & elem : stacks)
		{
			if(stack.second->type == elem.second->type && stack.first != elem.first)
			{
				out.first = stack.first;
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

void CCreatureSet::addToSlot(const SlotID & slot, const CreatureID & cre, TQuantity count, bool allowMerging)
{
	const CCreature *c = VLC->creh->objects[cre];

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
		logGlobal->error("Failed adding to slot!");
	}
}

void CCreatureSet::addToSlot(const SlotID & slot, CStackInstance * stack, bool allowMerging)
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
		logGlobal->error("Cannot add to slot %d stack %s", slot.getNum(), stack->nodeName());
	}
}

bool CCreatureSet::validTypes(bool allowUnrandomized) const
{
	for(const auto & elem : stacks)
	{
		if(!elem.second->valid(allowUnrandomized))
			return false;
	}
	return true;
}

bool CCreatureSet::slotEmpty(const SlotID & slot) const
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
	for(const auto & elem : stacks)
		ret += elem.second->getPower();
	return ret;
}

ui64 CCreatureSet::getPower(const SlotID & slot) const
{
	return getStack(slot).getPower();
}

std::string CCreatureSet::getRoughAmount(const SlotID & slot, int mode) const
{
	/// Mode represent return string format
	/// "Pack" - 0, "A pack of" - 1, "a pack of" - 2
	CCreature::CreatureQuantityId quantity = CCreature::getQuantityID(getStackCount(slot));

	if((int)quantity)
	{
		if(settings["gameTweaks"]["numericCreaturesQuantities"].Bool())
			return CCreature::getQuantityRangeStringForId(quantity);

		return VLC->generaltexth->arraytxt[(174 + mode) + 3*(int)quantity];
	}
	return "";
}

std::string CCreatureSet::getArmyDescription() const
{
	std::string text;
	std::vector<std::string> guards;
	for(const auto & elem : stacks)
	{
		auto str = boost::str(boost::format("%s %s") % getRoughAmount(elem.first, 2) % getCreature(elem.first)->getNamePluralTranslated());
		guards.push_back(str);
	}
	if(!guards.empty())
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
	return static_cast<int>(stacks.size());
}

void CCreatureSet::setFormation(bool tight)
{
	if (tight)
		formation = EArmyFormation::TIGHT;
	else
		formation = EArmyFormation::LOOSE;
}

void CCreatureSet::setStackCount(const SlotID & slot, TQuantity count)
{
	assert(hasStackAtSlot(slot));
	assert(stacks[slot]->count + count > 0);
	if (VLC->settings()->getBoolean(EGameSettings::MODULE_STACK_EXPERIENCE) && count > stacks[slot]->count)
		stacks[slot]->experience = static_cast<TExpType>(stacks[slot]->experience * (count / static_cast<double>(stacks[slot]->count)));
	stacks[slot]->count = count;
	armyChanged();
}

void CCreatureSet::giveStackExp(TExpType exp)
{
	for(TSlots::const_iterator i = stacks.begin(); i != stacks.end(); i++)
		i->second->giveStackExp(exp);
}
void CCreatureSet::setStackExp(const SlotID & slot, TExpType exp)
{
	assert(hasStackAtSlot(slot));
	stacks[slot]->experience = exp;
}

void CCreatureSet::clearSlots()
{
	while(!stacks.empty())
	{
		eraseStack(stacks.begin()->first);
	}
}

const CStackInstance & CCreatureSet::getStack(const SlotID & slot) const
{
	assert(hasStackAtSlot(slot));
	return *getStackPtr(slot);
}

const CStackInstance * CCreatureSet::getStackPtr(const SlotID & slot) const
{
	if(hasStackAtSlot(slot))
		return stacks.find(slot)->second;
	else return nullptr;
}

void CCreatureSet::eraseStack(const SlotID & slot)
{
	assert(hasStackAtSlot(slot));
	CStackInstance *toErase = detachStack(slot);
	vstd::clear_pointer(toErase);
}

bool CCreatureSet::contains(const CStackInstance *stack) const
{
	if(!stack)
		return false;

	for(const auto & elem : stacks)
		if(elem.second == stack)
			return true;

	return false;
}

SlotID CCreatureSet::findStack(const CStackInstance *stack) const
{
	const auto * h = dynamic_cast<const CGHeroInstance *>(this);
	if (h && h->commander == stack)
		return SlotID::COMMANDER_SLOT_PLACEHOLDER;

	if(!stack)
		return SlotID();

	for(const auto & elem : stacks)
		if(elem.second == stack)
			return elem.first;

	return SlotID();
}

CArmedInstance * CCreatureSet::castToArmyObj()
{
	return dynamic_cast<CArmedInstance *>(this);
}

void CCreatureSet::putStack(const SlotID & slot, CStackInstance * stack)
{
	assert(slot.getNum() < GameConstants::ARMY_SIZE);
	assert(!hasStackAtSlot(slot));
	stacks[slot] = stack;
	stack->setArmyObj(castToArmyObj());
	armyChanged();
}

void CCreatureSet::joinStack(const SlotID & slot, CStackInstance * stack)
{
	[[maybe_unused]] const CCreature *c = getCreature(slot);
	assert(c == stack->type);
	assert(c);

	//TODO move stuff
	changeStackCount(slot, stack->count);
	vstd::clear_pointer(stack);
}

void CCreatureSet::changeStackCount(const SlotID & slot, TQuantity toAdd)
{
	setStackCount(slot, getStackCount(slot) + toAdd);
}

CCreatureSet::~CCreatureSet()
{
	clearSlots();
}

void CCreatureSet::setToArmy(CSimpleArmy &src)
{
	clearSlots();
	while(src)
	{
		auto i = src.army.begin();

		putStack(i->first, new CStackInstance(i->second.first, i->second.second));
		src.army.erase(i);
	}
}

CStackInstance * CCreatureSet::detachStack(const SlotID & slot)
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

void CCreatureSet::setStackType(const SlotID & slot, const CreatureID & type)
{
	assert(hasStackAtSlot(slot));
	CStackInstance *s = stacks[slot];
	s->setType(type);
	armyChanged();
}

bool CCreatureSet::canBeMergedWith(const CCreatureSet &cs, bool allowMergingStacks) const
{
	if(!allowMergingStacks)
	{
		int freeSlots = stacksCount() - GameConstants::ARMY_SIZE;
		std::set<const CCreature*> cresToAdd;
		for(const auto & elem : cs.stacks)
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
		for(const auto & elem : cs.stacks)
			if ((j = cres.getSlotFor(elem.second->type)).validSlot())
				cres.addToSlot(j, elem.second->type->getId(), 1, true);  //merge if possible
			//cres.addToSlot(elem.first, elem.second->type->getId(), 1, true);
		for(const auto & elem : stacks)
		{
			if ((j = cres.getSlotFor(elem.second->type)).validSlot())
				cres.addToSlot(j, elem.second->type->getId(), 1, true);  //merge if possible
			else
				return false; //no place found
		}
		return true; //all stacks found their slots
	}
}

bool CCreatureSet::hasStackAtSlot(const SlotID & slot) const
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

void CCreatureSet::serializeJson(JsonSerializeFormat & handler, const std::string & fieldName, const std::optional<int> fixedSize)
{
	if(handler.saving && stacks.empty())
		return;

	auto a = handler.enterArray(fieldName);


	if(handler.saving)
	{
		size_t sz = 0;

		for(const auto & p : stacks)
			vstd::amax(sz, p.first.getNum()+1);

		if(fixedSize)
			vstd::amax(sz, fixedSize.value());

		a.resize(sz, JsonNode::JsonType::DATA_STRUCT);

		for(const auto & p : stacks)
		{
			auto s = a.enterStruct(p.first.getNum());
			p.second->serializeJson(handler);
		}
	}
	else
	{
		for(size_t idx = 0; idx < a.size(); idx++)
		{
			auto s = a.enterStruct(idx);

			TQuantity amount = 0;

			handler.serializeInt("amount", amount);

			if(amount > 0)
			{
				auto * new_stack = new CStackInstance();
				new_stack->serializeJson(handler);
				putStack(SlotID(static_cast<si32>(idx)), new_stack);
			}
		}
	}
}

CStackInstance::CStackInstance()
	: armyObj(_armyObj)
{
	init();
}

CStackInstance::CStackInstance(const CreatureID & id, TQuantity Count, bool isHypothetic):
	CBonusSystemNode(isHypothetic), armyObj(_armyObj)
{
	init();
	setType(id);
	count = Count;
}

CStackInstance::CStackInstance(const CCreature *cre, TQuantity Count, bool isHypothetic)
	: CBonusSystemNode(isHypothetic), armyObj(_armyObj)
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
	_armyObj = nullptr;
	setNodeType(STACK_INSTANCE);
}

CCreature::CreatureQuantityId CStackInstance::getQuantityID() const
{
	return CCreature::getQuantityID(count);
}

int CStackInstance::getExpRank() const
{
	if (!VLC->settings()->getBoolean(EGameSettings::MODULE_STACK_EXPERIENCE))
		return 0;
	int tier = type->getLevel();
	if (vstd::iswithin(tier, 1, 7))
	{
		for(int i = static_cast<int>(VLC->creh->expRanks[tier].size()) - 2; i > -1; --i) //sic!
		{ //exp values vary from 1st level to max exp at 11th level
			if (experience >= VLC->creh->expRanks[tier][i])
				return ++i; //faster, but confusing - 0 index mean 1st level of experience
		}
		return 0;
	}
	else //higher tier
	{
		for(int i = static_cast<int>(VLC->creh->expRanks[0].size()) - 2; i > -1; --i)
		{
			if (experience >= VLC->creh->expRanks[0][i])
				return ++i;
		}
		return 0;
	}
}

int CStackInstance::getLevel() const
{
	return std::max(1, static_cast<int>(type->getLevel()));
}

void CStackInstance::giveStackExp(TExpType exp)
{
	int level = type->getLevel();
	if (!vstd::iswithin(level, 1, 7))
		level = 0;

	CCreatureHandler * creh = VLC->creh;
	ui32 maxExp = creh->expRanks[level].back();

	vstd::amin(exp, static_cast<TExpType>(maxExp)); //prevent exp overflow due to different types
	vstd::amin(exp, (maxExp * creh->maxExpPerBattle[level])/100);
	vstd::amin(experience += exp, maxExp); //can't get more exp than this limit
}

void CStackInstance::setType(const CreatureID & creID)
{
	if(creID.getNum() >= 0 && creID.getNum() < VLC->creh->objects.size())
		setType(VLC->creh->objects[creID]);
	else
		setType((const CCreature*)nullptr);
}

void CStackInstance::setType(const CCreature *c)
{
	if(type)
	{
		detachFrom(const_cast<CCreature&>(*type));
		if (type->isMyUpgrade(c) && VLC->settings()->getBoolean(EGameSettings::MODULE_STACK_EXPERIENCE))
			experience = static_cast<TExpType>(experience * VLC->creh->expAfterUpgrade / 100.0);
	}

	CStackBasicDescriptor::setType(c);

	if(type)
		attachTo(const_cast<CCreature&>(*type));
}
std::string CStackInstance::bonusToString(const std::shared_ptr<Bonus>& bonus, bool description) const
{
	return VLC->getBth()->bonusToString(bonus, this, description);
}

ImagePath CStackInstance::bonusToGraphics(const std::shared_ptr<Bonus> & bonus) const
{
	return VLC->getBth()->bonusToGraphics(bonus);
}

void CStackInstance::setArmyObj(const CArmedInstance * ArmyObj)
{
	if(_armyObj)
		detachFrom(const_cast<CArmedInstance&>(*_armyObj));

	_armyObj = ArmyObj;

	if(ArmyObj)
		attachTo(const_cast<CArmedInstance&>(*_armyObj));
}

std::string CStackInstance::getQuantityTXT(bool capitalized) const
{
	CCreature::CreatureQuantityId quantity = getQuantityID();

	if ((int)quantity)
	{
		if(settings["gameTweaks"]["numericCreaturesQuantities"].Bool())
			return CCreature::getQuantityRangeStringForId(quantity);

		return VLC->generaltexth->arraytxt[174 + (int)quantity*3 - 1 - capitalized];
	}
	else
		return "";
}

bool CStackInstance::valid(bool allowUnrandomized) const
{
	if(!randomStack)
	{
		return (type  &&  type == VLC->creh->objects[type->getId()]);
	}
	else
		return allowUnrandomized;
}

std::string CStackInstance::nodeName() const
{
	std::ostringstream oss;
	oss << "Stack of " << count << " of ";
	if(type)
		oss << type->getNamePluralTextID();
	else
		oss << "[UNDEFINED TYPE]";

	return oss.str();
}

PlayerColor CStackInstance::getOwner() const
{
	return _armyObj ? _armyObj->getOwner() : PlayerColor::NEUTRAL;
}

void CStackInstance::deserializationFix()
{
	const CArmedInstance *armyBackup = _armyObj;
	_armyObj = nullptr;
	setArmyObj(armyBackup);
	artDeserializationFix(this);
}

CreatureID CStackInstance::getCreatureID() const
{
	if(type)
		return type->getId();
	else
		return CreatureID::NONE;
}

std::string CStackInstance::getName() const
{
	return (count > 1) ? type->getNamePluralTranslated() : type->getNameSingularTranslated();
}

ui64 CStackInstance::getPower() const
{
	assert(type);
	return type->getAIValue() * count;
}

ArtBearer::ArtBearer CStackInstance::bearerType() const
{
	return ArtBearer::CREATURE;
}

void CStackInstance::putArtifact(ArtifactPosition pos, CArtifactInstance * art)
{
	assert(!getArt(pos));
	assert(art->artType->canBePutAt(this, pos));

	CArtifactSet::putArtifact(pos, art);
	if(ArtifactUtils::isSlotEquipment(pos))
		attachTo(*art);
}

void CStackInstance::removeArtifact(ArtifactPosition pos)
{
	assert(getArt(pos));

	detachFrom(*getArt(pos));
	CArtifactSet::removeArtifact(pos);
}

void CStackInstance::serializeJson(JsonSerializeFormat & handler)
{
	//todo: artifacts
	CStackBasicDescriptor::serializeJson(handler);//must be first

	if(handler.saving)
	{
		if(randomStack)
		{
			int level = randomStack->level;
			int upgrade = randomStack->upgrade;

			handler.serializeInt("level", level, 0);
			handler.serializeInt("upgraded", upgrade, 0);
		}
	}
	else
	{
		//type set by CStackBasicDescriptor::serializeJson
		if(type == nullptr)
		{
			uint8_t level = 0;
			uint8_t upgrade = 0;

			handler.serializeInt("level", level, 0);
			handler.serializeInt("upgrade", upgrade, 0);

			randomStack = RandomStackInfo{ level, upgrade };
		}
	}
}

FactionID CStackInstance::getFaction() const
{
	if(type)
		return type->getFaction();
		
	return FactionID::NEUTRAL;
}

const IBonusBearer* CStackInstance::getBonusBearer() const
{
	return this;
}

CCommanderInstance::CCommanderInstance()
{
	init();
}

CCommanderInstance::CCommanderInstance(const CreatureID & id): name("Commando")
{
	init();
	setType(id);
	//TODO - parse them
}

void CCommanderInstance::init()
{
	alive = true;
	experience = 0;
	level = 1;
	count = 1;
	type = nullptr;
	_armyObj = nullptr;
	setNodeType (CBonusSystemNode::COMMANDER);
	secondarySkills.resize (ECommander::SPELL_POWER + 1);
}

void CCommanderInstance::setAlive (bool Alive)
{
	//TODO: helm of immortality
	alive = Alive;
	if (!alive)
	{
		removeBonusesRecursive(Bonus::UntilCommanderKilled);
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
	for(const auto & bonus : VLC->creh->commanderLevelPremy)
	{ //grant all regular level-up bonuses
		accumulateBonus(bonus);
	}
}

ArtBearer::ArtBearer CCommanderInstance::bearerType() const
{
	return ArtBearer::COMMANDER;
}

bool CCommanderInstance::gainsLevel() const
{
	return experience >= static_cast<TExpType>(VLC->heroh->reqExp(level + 1));
}

//This constructor should be placed here to avoid side effects
CStackBasicDescriptor::CStackBasicDescriptor() = default;

CStackBasicDescriptor::CStackBasicDescriptor(const CreatureID & id, TQuantity Count):
	type(VLC->creh->objects[id]), 
	count(Count)
{
}

CStackBasicDescriptor::CStackBasicDescriptor(const CCreature *c, TQuantity Count)
	: type(c), count(Count)
{
}

const Creature * CStackBasicDescriptor::getType() const
{
	return type;
}

TQuantity CStackBasicDescriptor::getCount() const
{
	return count;
}


void CStackBasicDescriptor::setType(const CCreature * c)
{
	type = c;
}

void CStackBasicDescriptor::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeInt("amount", count);

	if(handler.saving)
	{
		if(type)
		{
			std::string typeName = type->getJsonKey();
			handler.serializeString("type", typeName);
		}
	}
	else
	{
		std::string typeName;
		handler.serializeString("type", typeName);
		if(!typeName.empty())
			setType(VLC->creh->getCreature(ModScope::scopeMap(), typeName));
	}
}

void CSimpleArmy::clearSlots()
{
	army.clear();
}

CSimpleArmy::operator bool() const
{
	return !army.empty();
}

bool CSimpleArmy::setCreature(SlotID slot, CreatureID cre, TQuantity count)
{
	assert(!vstd::contains(army, slot));
	army[slot] = std::make_pair(cre, count);
	return true;
}

VCMI_LIB_NAMESPACE_END
