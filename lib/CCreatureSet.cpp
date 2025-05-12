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

#include "CConfigHandler.h"
#include "CCreatureHandler.h"
#include "GameLibrary.h"
#include "IGameSettings.h"
#include "entities/hero/CHeroHandler.h"
#include "mapObjects/CGHeroInstance.h"
#include "modding/ModScope.h"
#include "IGameCallback.h"
#include "texts/CGeneralTextHandler.h"
#include "spells/CSpellHandler.h"
#include "IBonusTypeHandler.h"
#include "serializer/JsonSerializeFormat.h"
#include "gameState/CGameState.h"

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
		return i->second->getCreature();
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

	auto * armyObj = getArmy();
	bool isHypotheticArmy = armyObj ? armyObj->isHypothetic() : false;

	putStack(slot, std::make_unique<CStackInstance>(armyObj ? armyObj->cb : nullptr, type, quantity, isHypotheticArmy));
	return true;
}

SlotID CCreatureSet::getSlotFor(const CreatureID & creature, ui32 slotsAmount) const /*returns -1 if no slot available */
{
	return getSlotFor(creature.toCreature(), slotsAmount);
}

SlotID CCreatureSet::getSlotFor(const CCreature *c, ui32 slotsAmount) const
{
	assert(c);
	for(const auto & elem : stacks)
	{
		if(elem.second->getType() == c)
		{
			return elem.first; //if there is already such creature we return its slot id
		}
	}
	return getFreeSlot(slotsAmount);
}

bool CCreatureSet::hasCreatureSlots(const CCreature * c, const SlotID & exclude) const
{
	assert(c);
	for(const auto & elem : stacks) // elem is const
	{
		if(elem.first == exclude) // Check slot
			continue;

		if(!elem.second || !elem.second->getType()) // Check creature
			continue;

		if(elem.second->getType() == c)
			return true;
	}
	return false;
}

std::vector<SlotID> CCreatureSet::getCreatureSlots(const CCreature * c, const SlotID & exclude, TQuantity ignoreAmount) const
{
	assert(c);
	std::vector<SlotID> result;

	for(const auto & elem : stacks)
	{
		if(elem.first == exclude)
			continue;

		if(!elem.second || !elem.second->getType() || elem.second->getType() != c)
			continue;

		if(elem.second->getCount() == ignoreAmount || elem.second->getCount() < 1)
			continue;

		result.push_back(elem.first);
	}
	return result;
}

bool CCreatureSet::isCreatureBalanced(const CCreature * c, TQuantity ignoreAmount) const
{
	assert(c);
	TQuantity max = 0;
	auto min = std::numeric_limits<TQuantity>::max();

	for(const auto & elem : stacks)
	{
		if(!elem.second || !elem.second->getType() || elem.second->getType() != c)
			continue;

		const auto count = elem.second->getCount();

		if(count == ignoreAmount || count < 1)
			continue;


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
		const auto * creature = pair.second->getCreature();
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
		creatureQueue.push(std::make_pair(pair.second->getCreature(), pair.first));
	}
	return creatureQueue;
}

TQuantity CCreatureSet::getStackCount(const SlotID & slot) const
{
	if (!hasStackAtSlot(slot))
		return 0;
	return stacks.at(slot)->getCount();
}

TExpType CCreatureSet::getStackTotalExperience(const SlotID & slot) const
{
	return stacks.at(slot)->getTotalExperience();
}

TExpType CCreatureSet::getStackAverageExperience(const SlotID & slot) const
{
	return stacks.at(slot)->getAverageExperience();
}

bool CCreatureSet::mergeableStacks(std::pair<SlotID, SlotID> & out, const SlotID & preferable) const /*looks for two same stacks, returns slot positions */
{
	//try to match creature to our preferred stack
	if(preferable.validSlot() &&  vstd::contains(stacks, preferable))
	{
		const CCreature *cr = stacks.find(preferable)->second->getCreature();
		for(const auto & elem : stacks)
		{
			if(cr == elem.second->getType() && elem.first != preferable)
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
			if(stack.second->getType() == elem.second->getType() && stack.first != elem.first)
			{
				out.first = stack.first;
				out.second = elem.first;
				return true;
			}
		}
	}
	return false;
}

void CCreatureSet::addToSlot(const SlotID & slot, const CreatureID & cre, TQuantity count, bool allowMerging)
{
	const CCreature *c = cre.toCreature();

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

void CCreatureSet::addToSlot(const SlotID & slot, std::unique_ptr<CStackInstance> stack, bool allowMerging)
{
	assert(stack->valid(true));

	if(!hasStackAtSlot(slot))
	{
		putStack(slot, std::move(stack));
	}
	else if(allowMerging && stack->getType() == getCreature(slot))
	{
		joinStack(slot, std::move(stack));
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

ui64 CCreatureSet::getArmyStrength(int fortLevel) const
{
	ui64 ret = 0;
	for (const auto& elem : stacks)
	{
		ui64 powerToAdd = elem.second->getPower();
		if (fortLevel > 0)
		{
			if (!elem.second->hasBonusOfType(BonusType::FLYING))
			{
				powerToAdd /= fortLevel;
				if (!elem.second->hasBonusOfType(BonusType::SHOOTER))
					powerToAdd /= fortLevel;
			}
		} 
		ret += powerToAdd;
	}
	return ret;
}

ui64 CCreatureSet::getArmyCost() const
{
	ui64 ret = 0;
	for (const auto& elem : stacks)
		ret += elem.second->getMarketValue();
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

		return LIBRARY->generaltexth->arraytxt[(174 + mode) + 3*(int)quantity];
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
				text += LIBRARY->generaltexth->allTexts[237];
		}
	}
	return text;
}

int CCreatureSet::stacksCount() const
{
	return static_cast<int>(stacks.size());
}

void CCreatureSet::setFormation(EArmyFormation mode)
{
	formation = mode;
}

void CCreatureSet::setStackCount(const SlotID & slot, TQuantity count)
{
	stacks.at(slot)->setCount(count);
	armyChanged();
}

void CCreatureSet::giveAverageStackExperience(TExpType exp)
{
	for(const auto & stack : stacks)
	{
		stack.second->giveAverageStackExperience(exp);
		stack.second->nodeHasChanged();
	}
}

void CCreatureSet::giveTotalStackExperience(const SlotID & slot, TExpType exp)
{
	assert(hasStackAtSlot(slot));
	stacks[slot]->giveTotalStackExperience(exp);
	stacks[slot]->nodeHasChanged();
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

CStackInstance * CCreatureSet::getStackPtr(const SlotID & slot) const
{
	if(hasStackAtSlot(slot))
		return stacks.find(slot)->second.get();
	else return nullptr;
}

void CCreatureSet::eraseStack(const SlotID & slot)
{
	assert(hasStackAtSlot(slot));
	detachStack(slot);
}

bool CCreatureSet::contains(const CStackInstance *stack) const
{
	if(!stack)
		return false;

	for(const auto & elem : stacks)
		if(elem.second.get() == stack)
			return true;

	return false;
}

SlotID CCreatureSet::findStack(const CStackInstance *stack) const
{
	const auto * h = dynamic_cast<const CGHeroInstance *>(this);
	if (h && h->getCommander() == stack)
		return SlotID::COMMANDER_SLOT_PLACEHOLDER;

	if(!stack)
		return SlotID();

	for(const auto & elem : stacks)
		if(elem.second.get() == stack)
			return elem.first;

	return SlotID();
}

void CCreatureSet::putStack(const SlotID & slot, std::unique_ptr<CStackInstance> stack)
{
	assert(slot.getNum() < GameConstants::ARMY_SIZE);
	assert(!hasStackAtSlot(slot));

	stacks[slot] = std::move(stack);
	stacks[slot]->setArmy(getArmy());
	armyChanged();
}

void CCreatureSet::joinStack(const SlotID & slot, std::unique_ptr<CStackInstance> stack)
{
	[[maybe_unused]] const CCreature *c = getCreature(slot);
	assert(c == stack->getType());
	assert(c);

	//TODO move stuff
	changeStackCount(slot, stack->getCount());
	giveTotalStackExperience(slot, stack->getTotalExperience());
}

std::unique_ptr<CStackInstance> CCreatureSet::splitStack(const SlotID & slot, TQuantity toSplit)
{
	auto & currentStack = stacks.at(slot);
	assert(currentStack->getCount() > toSplit);

	TExpType experienceBefore = currentStack->getTotalExperience();
	currentStack->setCount(currentStack->getCount() - toSplit);
	TExpType experienceAfter = currentStack->getTotalExperience();

	auto newStack = std::make_unique<CStackInstance>(currentStack->cb, currentStack->getCreatureID(), toSplit);
	newStack->giveTotalStackExperience(experienceBefore - experienceAfter);

	return newStack;
}

void CCreatureSet::changeStackCount(const SlotID & slot, TQuantity toAdd)
{
	setStackCount(slot, getStackCount(slot) + toAdd);
}

CCreatureSet::~CCreatureSet() = default;

void CCreatureSet::setToArmy(CSimpleArmy &src)
{
	clearSlots();
	while(src)
	{
		auto i = src.army.begin();

		putStack(i->first, std::make_unique<CStackInstance>(getArmy()->cb, i->second.first, i->second.second));
		src.army.erase(i);
	}
}

std::unique_ptr<CStackInstance> CCreatureSet::detachStack(const SlotID & slot)
{
	assert(hasStackAtSlot(slot));
	std::unique_ptr<CStackInstance> ret = std::move(stacks[slot]);

	//if(CArmedInstance *armedObj = castToArmyObj())
	if(ret)
	{
		ret->setArmy(nullptr); //detaches from current armyobj
		assert(!ret->getArmy()); //we failed detaching?
	}

	stacks.erase(slot);
	armyChanged();
	return ret;
}

void CCreatureSet::setStackType(const SlotID & slot, const CreatureID & type)
{
	assert(hasStackAtSlot(slot));
	stacks[slot]->setType(type);
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
			SlotID dest = getSlotFor(elem.second->getCreature());
			if(!dest.validSlot() || hasStackAtSlot(dest))
				cresToAdd.insert(elem.second->getCreature());
		}
		return cresToAdd.size() <= freeSlots;
	}
	else
	{
		CCreatureSet cres;
		SlotID j;

		//get types of creatures that need their own slot
		for(const auto & elem : cs.stacks)
			if ((j = cres.getSlotFor(elem.second->getCreature())).validSlot())
				cres.addToSlot(j, elem.second->getId(), 1, true);  //merge if possible
			//cres.addToSlot(elem.first, elem.second->type->getId(), 1, true);
		for(const auto & elem : stacks)
		{
			if ((j = cres.getSlotFor(elem.second->getCreature())).validSlot())
				cres.addToSlot(j, elem.second->getId(), 1, true);  //merge if possible
			else
				return false; //no place found
		}
		return true; //all stacks found their slots
	}
}

bool CCreatureSet::hasUnits(const std::vector<CStackBasicDescriptor> & units, bool requireLastStack) const
{
	bool foundExtraCreatures = false;
	int testedSlots = 0;
	for(const auto & reqStack : units)
	{
		size_t count = 0;
		for(const auto & slot : Slots())
		{
			const auto & heroStack = slot.second;
			if (heroStack->getType() == reqStack.getType())
			{
				count += heroStack->getCount();
				testedSlots += 1;
			}
		}
		if (count > reqStack.getCount())
			foundExtraCreatures = true;

		if (count < reqStack.getCount()) //not enough creatures of this kind
			return false;
	}

	if (requireLastStack)
	{
		if (!foundExtraCreatures && testedSlots >= Slots().size())
			return false;
	}

	return true;
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

void CCreatureSet::serializeJson(JsonSerializeFormat & handler, const std::string & armyFieldName, const std::optional<int> fixedSize)
{
	if(handler.saving && stacks.empty())
		return;

	handler.serializeEnum("formation", formation, NArmyFormation::names);
	auto a = handler.enterArray(armyFieldName);


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
				auto newStack = std::make_unique<CStackInstance>(getArmy()->cb);
				newStack->serializeJson(handler);
				putStack(SlotID(static_cast<si32>(idx)), std::move(newStack));
			}
		}
	}
}

CStackInstance::CStackInstance(IGameCallback *cb, bool isHypothetic)
	: CBonusSystemNode(isHypothetic)
	, CStackBasicDescriptor(nullptr, 0)
	, CArtifactSet(cb)
	, GameCallbackHolder(cb)
	, nativeTerrain(this, Selector::type()(BonusType::TERRAIN_NATIVE))
	, initiative(this, Selector::type()(BonusType::STACKS_SPEED))
	, totalExperience(0)
{
	setNodeType(STACK_INSTANCE);
}

CStackInstance::CStackInstance(IGameCallback *cb, const CreatureID & id, TQuantity Count, bool isHypothetic)
	: CStackInstance(cb, false)
{
	setType(id);
	setCount(Count);
}

CCreature::CreatureQuantityId CStackInstance::getQuantityID() const
{
	return CCreature::getQuantityID(getCount());
}

int CStackInstance::getExpRank() const
{
	if (!LIBRARY->engineSettings()->getBoolean(EGameSettings::MODULE_STACK_EXPERIENCE))
		return 0;
	int tier = getType()->getLevel();
	if (vstd::iswithin(tier, 1, 7))
	{
		for(int i = static_cast<int>(LIBRARY->creh->expRanks[tier].size()) - 2; i > -1; --i) //sic!
		{ //exp values vary from 1st level to max exp at 11th level
			if (getAverageExperience() >= LIBRARY->creh->expRanks[tier][i])
				return ++i; //faster, but confusing - 0 index mean 1st level of experience
		}
		return 0;
	}
	else //higher tier
	{
		for(int i = static_cast<int>(LIBRARY->creh->expRanks[0].size()) - 2; i > -1; --i)
		{
			if (getAverageExperience() >= LIBRARY->creh->expRanks[0][i])
				return ++i;
		}
		return 0;
	}
}

int CStackInstance::getLevel() const
{
	return std::max(1, getType()->getLevel());
}

void CStackInstance::giveAverageStackExperience(TExpType desiredAmountPerUnit)
{
	if (!canGainExperience())
		return;

	int level = std::clamp(getLevel(), 1, 7);
	TExpType maxAmountPerUnit = LIBRARY->creh->expRanks[level].back();
	TExpType actualAmountPerUnit = std::min(desiredAmountPerUnit, maxAmountPerUnit * LIBRARY->creh->maxExpPerBattle[level]/100);
	TExpType maxExperience = maxAmountPerUnit * getCount();
	TExpType maxExperienceToGain = maxExperience - totalExperience;
	TExpType actualGainedExperience = std::min(maxExperienceToGain, actualAmountPerUnit * getCount());

	totalExperience	+= actualGainedExperience;
}

void CStackInstance::giveTotalStackExperience(TExpType experienceToGive)
{
	if (!canGainExperience())
		return;

	int level = std::clamp(getLevel(), 1, 7);
	TExpType maxAmountPerUnit = LIBRARY->creh->expRanks[level].back();
	TExpType maxExperience = maxAmountPerUnit * getCount();
	TExpType maxExperienceToGain = maxExperience - totalExperience;
	TExpType actualGainedExperience = std::min(maxExperienceToGain, experienceToGive);
	totalExperience	+= actualGainedExperience;
}

TExpType CStackInstance::getTotalExperience() const
{
	return totalExperience;
}

TExpType CStackInstance::getAverageExperience() const
{
	return totalExperience / getCount();
}

bool CStackInstance::canGainExperience() const
{
	return cb->getSettings().getBoolean(EGameSettings::MODULE_STACK_EXPERIENCE);
}

void CStackInstance::setType(const CreatureID & creID)
{
	if (creID == CreatureID::NONE)
		setType(nullptr);//FIXME: unused branch?
	else
		setType(creID.toCreature());
}

void CStackInstance::setType(const CCreature *c)
{
	if(getCreature())
	{
		detachFromSource(*getCreature());
		if (LIBRARY->engineSettings()->getBoolean(EGameSettings::MODULE_STACK_EXPERIENCE))
			totalExperience = totalExperience * LIBRARY->creh->expAfterUpgrade / 100;
	}

	CStackBasicDescriptor::setType(c);

	if(getCreature())
		attachToSource(*getCreature());
}

void CStackInstance::setCount(TQuantity newCount)
{
	assert(newCount >= 0);

	if (newCount < getCount())
	{
		TExpType averageExperience = totalExperience / getCount();
		totalExperience = averageExperience * newCount;
	}

	CStackBasicDescriptor::setCount(newCount);
}

std::string CStackInstance::bonusToString(const std::shared_ptr<Bonus>& bonus, bool description) const
{
	return LIBRARY->getBth()->bonusToString(bonus, this, description);
}

ImagePath CStackInstance::bonusToGraphics(const std::shared_ptr<Bonus> & bonus) const
{
	return LIBRARY->getBth()->bonusToGraphics(bonus);
}

CArmedInstance * CStackInstance::getArmy()
{
	return armyInstance;
}

const CArmedInstance * CStackInstance::getArmy() const
{
	return armyInstance;
}

void CStackInstance::setArmy(CArmedInstance * ArmyObj)
{
	auto oldArmy = getArmy();

	if(oldArmy)
	{
		detachFrom(*oldArmy);
		armyInstance = nullptr;
	}

	if(ArmyObj)
	{
		attachTo(const_cast<CArmedInstance&>(*ArmyObj));
		armyInstance = ArmyObj;
	}
}

std::string CStackInstance::getQuantityTXT(bool capitalized) const
{
	CCreature::CreatureQuantityId quantity = getQuantityID();

	if ((int)quantity)
	{
		if(settings["gameTweaks"]["numericCreaturesQuantities"].Bool())
			return CCreature::getQuantityRangeStringForId(quantity);

		return LIBRARY->generaltexth->arraytxt[174 + (int)quantity*3 - 1 - capitalized];
	}
	else
		return "";
}

bool CStackInstance::valid(bool allowUnrandomized) const
{
	if(!randomStack)
	{
		return (getType() && getType() == getId().toEntity(LIBRARY));
	}
	else
		return allowUnrandomized;
}

std::string CStackInstance::nodeName() const
{
	std::ostringstream oss;
	oss << "Stack of " << getCount() << " of ";
	if(getType())
		oss << getType()->getNamePluralTextID();
	else
		oss << "[UNDEFINED TYPE]";

	return oss.str();
}

PlayerColor CStackInstance::getOwner() const
{
	auto army = getArmy();
	return army ? army->getOwner() : PlayerColor::NEUTRAL;
}

int32_t CStackInstance::getInitiative(int turn) const
{
	if (turn == 0)
		return initiative.getValue();

	return ACreature::getInitiative(turn);
}

TerrainId CStackInstance::getNativeTerrain() const
{
	if (nativeTerrain.hasBonus())
		return TerrainId::ANY_TERRAIN;

	return getFactionID().toEntity(LIBRARY)->getNativeTerrain();
}

TerrainId CStackInstance::getCurrentTerrain() const
{
	assert(getArmy() != nullptr);
	return getArmy()->getCurrentTerrain();
}

CreatureID CStackInstance::getCreatureID() const
{
	if(getType())
		return getType()->getId();
	else
		return CreatureID::NONE;
}

std::string CStackInstance::getName() const
{
	return (getCount() > 1) ? getType()->getNamePluralTranslated() : getType()->getNameSingularTranslated();
}

ui64 CStackInstance::getPower() const
{
	assert(getType());
	return static_cast<ui64>(getType()->getAIValue()) * getCount();
}

ui64 CStackInstance::getMarketValue() const
{
	assert(getType());
	return getType()->getFullRecruitCost().marketValue() * getCount();
}

ArtBearer CStackInstance::bearerType() const
{
	return ArtBearer::CREATURE;
}

CStackInstance::ArtPlacementMap CStackInstance::putArtifact(const ArtifactPosition & pos, const CArtifactInstance * art)
{
	assert(!getArt(pos));
	assert(art->canBePutAt(this, pos));

	attachToSource(*art);
	return CArtifactSet::putArtifact(pos, art);
}

void CStackInstance::removeArtifact(const ArtifactPosition & pos)
{
	assert(getArt(pos));

	detachFromSource(*getArt(pos));
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
		if(getType() == nullptr)
		{
			uint8_t level = 0;
			uint8_t upgrade = 0;

			handler.serializeInt("level", level, 0);
			handler.serializeInt("upgrade", upgrade, 0);

			randomStack = RandomStackInfo{ level, upgrade };
		}
	}
}

FactionID CStackInstance::getFactionID() const
{
	if(getType())
		return getType()->getFactionID();
		
	return FactionID::NEUTRAL;
}

const IBonusBearer* CStackInstance::getBonusBearer() const
{
	return this;
}

CCommanderInstance::CCommanderInstance(IGameCallback *cb)
	:CStackInstance(cb)
{}

CCommanderInstance::CCommanderInstance(IGameCallback *cb, const CreatureID & id)
	: CStackInstance(cb)
	, name("Commando")
{
	alive = true;
	level = 1;
	setCount(1);
	setType(nullptr);
	setNodeType (CBonusSystemNode::COMMANDER);
	secondarySkills.resize (ECommander::SPELL_POWER + 1);
	setType(id);
	//TODO - parse them
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

bool CCommanderInstance::canGainExperience() const
{
	return alive && CStackInstance::canGainExperience();
}

int CCommanderInstance::getExpRank() const
{
	return LIBRARY->heroh->level (getTotalExperience());
}

int CCommanderInstance::getLevel() const
{
	return std::max (1, getExpRank());
}

void CCommanderInstance::levelUp ()
{
	level++;
	for(const auto & bonus : LIBRARY->creh->commanderLevelPremy)
	{ //grant all regular level-up bonuses
		accumulateBonus(bonus);
	}
}

ArtBearer CCommanderInstance::bearerType() const
{
	return ArtBearer::COMMANDER;
}

bool CCommanderInstance::gainsLevel() const
{
	return getTotalExperience() >= LIBRARY->heroh->reqExp(level + 1);
}

//This constructor should be placed here to avoid side effects
CStackBasicDescriptor::CStackBasicDescriptor() = default;

CStackBasicDescriptor::CStackBasicDescriptor(const CreatureID & id, TQuantity Count):
	typeID(id),
	count(Count)
{
}

CStackBasicDescriptor::CStackBasicDescriptor(const CCreature *c, TQuantity Count)
	: typeID(c ? c->getId() : CreatureID()), count(Count)
{
}

const CCreature * CStackBasicDescriptor::getCreature() const
{
	return typeID.hasValue() ? typeID.toCreature() : nullptr;
}

const Creature * CStackBasicDescriptor::getType() const
{
	return typeID.hasValue() ? typeID.toEntity(LIBRARY) : nullptr;
}

CreatureID CStackBasicDescriptor::getId() const
{
	return typeID;
}

TQuantity CStackBasicDescriptor::getCount() const
{
	return count;
}

void CStackBasicDescriptor::setType(const CCreature * c)
{
	typeID = c ? c->getId() : CreatureID();
}

void CStackBasicDescriptor::setCount(TQuantity newCount)
{
	assert(newCount >= 0);
	count = newCount;
}

bool operator== (const CStackBasicDescriptor & l, const CStackBasicDescriptor & r)
{
	return l.typeID == r.typeID && l.count == r.count;
}

void CStackBasicDescriptor::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeInt("amount", count);

	if(handler.saving)
	{
		if(typeID.hasValue())
		{
			std::string typeName = typeID.toEntity(LIBRARY)->getJsonKey();
			handler.serializeString("type", typeName);
		}
	}
	else
	{
		std::string typeName;
		handler.serializeString("type", typeName);
		if(!typeName.empty())
			setType(CreatureID(CreatureID::decode(typeName)).toCreature());
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
