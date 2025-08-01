/*
 * CCreatureSet.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CSimpleArmy.h"
#include "CStackInstance.h"

#include "serializer/Serializeable.h"

VCMI_LIB_NAMESPACE_BEGIN

class CStackInstance;
class CArmedInstance;
class CStackBasicDescriptor;
class JsonSerializeFormat;

using TSlots = std::map<SlotID, std::unique_ptr<CStackInstance>>;

using TPairCreatureSlot = std::pair<const CCreature *, SlotID>;
using TMapCreatureSlot = std::map<const CCreature *, SlotID>;

struct DLL_LINKAGE CreatureSlotComparer
{
	bool operator()(const TPairCreatureSlot & lhs, const TPairCreatureSlot & rhs);
};

using TCreatureQueue = std::priority_queue<TPairCreatureSlot, std::vector<TPairCreatureSlot>, CreatureSlotComparer>;

namespace NArmyFormation
{
static const std::vector<std::string> names{"wide", "tight"};
}

class DLL_LINKAGE CCreatureSet : public IArmyDescriptor, public virtual Serializeable, boost::noncopyable //seven combined creatures
{
	CCreatureSet(const CCreatureSet &) = delete;
	CCreatureSet & operator=(const CCreatureSet &) = delete;

public:
	TSlots stacks; //slots[slot_id]->> pair(creature_id,creature_quantity)
	EArmyFormation formation = EArmyFormation::LOOSE; //0 - wide, 1 - tight

	CCreatureSet() = default; //Should be here to avoid compile errors
	virtual ~CCreatureSet();
	virtual void armyChanged();

	const CStackInstance & operator[](const SlotID & slot) const;

	const TSlots &Slots() const {return stacks;}

	//Adds stack to slot. Slot must be empty or with same type creature
	void addToSlot(const SlotID & slot, const CreatureID & cre, TQuantity count, bool allowMerging = true);
	//Adds stack to slot. Slot must be empty or with same type creature
	void addToSlot(const SlotID & slot, std::unique_ptr<CStackInstance> stack, bool allowMerging = true);

	void clearSlots() override;
	void setFormation(EArmyFormation tight);
	virtual CArmedInstance * getArmy() { return nullptr; }
	virtual const CArmedInstance * getArmy() const { return nullptr; }

	//adds new stack to the army, slot must be empty
	void putStack(const SlotID & slot, std::unique_ptr<CStackInstance> stack);
	//stack must exist!
	void setStackCount(const SlotID & slot, TQuantity count);
	//removes stack from army but doesn't destroy it (so it can be moved somewhere else or safely deleted)
	std::unique_ptr<CStackInstance> detachStack(const SlotID & slot);

	void setStackType(const SlotID & slot, const CreatureID & type);

	/// Give specified amount of experience to all units in army
	/// Amount of granted experience is scaled by unit stack size
	void giveAverageStackExperience(TExpType exp);

	/// Give specified amount of experience to unit in specified slot
	/// Amount of granted experience is not scaled by unit stack size
	void giveTotalStackExperience(const SlotID & slot, TExpType exp);

	/// Erased stack from specified slot. Slot must be non-empty
	void eraseStack(const SlotID & slot);

	/// Joins stack into stack that occupies targeted slot.
	/// Slot must be non-empty and contain same creature type
	void joinStack(const SlotID & slot, std::unique_ptr<CStackInstance> stack); //adds new stack to the existing stack of the same type

	/// Splits off some units of specified stack and returns newly created stack
	/// Slot must be non-empty and contain more units that split quantity
	std::unique_ptr<CStackInstance> splitStack(const SlotID & slot, TQuantity toSplit);

	//stack must exist!
	void changeStackCount(const SlotID & slot, TQuantity toAdd);

	//replaces creature in stack; slots 0 to 6, if quantity=0 erases stack
	bool setCreature(SlotID slot, CreatureID type, TQuantity quantity) override;

	//erases all our army and moves stacks from src to us; src MUST NOT be an armed object! WARNING: use it wisely. Or better do not use at all.
	void setToArmy(CSimpleArmy & src);

	const CStackInstance & getStack(const SlotID & slot) const; //stack must exist
	CStackInstance * getStackPtr(const SlotID & slot) const; //if stack doesn't exist, returns nullptr
	const CCreature * getCreature(const SlotID & slot) const; //workaround of map issue;
	int getStackCount(const SlotID & slot) const;
	TExpType getStackTotalExperience(const SlotID & slot) const;
	TExpType getStackAverageExperience(const SlotID & slot) const;
	SlotID findStack(const CStackInstance * stack) const; //-1 if none
	SlotID getSlotFor(const CreatureID & creature, ui32 slotsAmount = GameConstants::ARMY_SIZE) const; //returns -1 if no slot available
	SlotID getSlotFor(const CCreature * c, ui32 slotsAmount = GameConstants::ARMY_SIZE) const; //returns -1 if no slot available
	bool hasCreatureSlots(const CCreature * c, const SlotID & exclude) const;
	std::vector<SlotID> getCreatureSlots(const CCreature * c, const SlotID & exclude, TQuantity ignoreAmount = -1) const;
	bool isCreatureBalanced(const CCreature * c, TQuantity ignoreAmount = 1) const; // Check if the creature is evenly distributed across slots

	SlotID getFreeSlot(ui32 slotsAmount = GameConstants::ARMY_SIZE) const; //returns first free slot
	std::vector<SlotID> getFreeSlots(ui32 slotsAmount = GameConstants::ARMY_SIZE) const;

	TMapCreatureSlot getCreatureMap() const;
	TCreatureQueue getCreatureQueue(const SlotID & exclude) const;

	bool mergeableStacks(std::pair<SlotID, SlotID> & out, const SlotID & preferable = SlotID()) const; //looks for two same stacks, returns slot positions;
	bool validTypes(bool allowUnrandomized = false) const; //checks if all types of creatures are set properly
	bool slotEmpty(const SlotID & slot) const;
	int stacksCount() const;
	virtual bool needsLastStack() const; //true if last stack cannot be taken
	ui64 getArmyStrength(int fortLevel = 0) const; //sum of AI values of creatures
	ui64 getArmyCost() const; //sum of cost of creatures
	ui64 getPower(const SlotID & slot) const; //value of specific stack
	std::string getRoughAmount(const SlotID & slot, int mode = 0) const; //rough size of specific stack
	std::string getArmyDescription() const;
	bool hasStackAtSlot(const SlotID & slot) const;

	bool contains(const CStackInstance * stack) const;
	bool canBeMergedWith(const CCreatureSet & cs, bool allowMergingStacks = true) const;

	/// Returns true if this creature set contains all listed units
	/// If requireLastStack is true, then this function will also
	/// require presence of any unit other than requested (or more units than requested)
	bool hasUnits(const std::vector<CStackBasicDescriptor> & units, bool requireLastStack = true) const;

	template<typename Handler>
	void serialize(Handler & h)
	{
		h & stacks;
		h & formation;
	}

	void serializeJson(JsonSerializeFormat & handler, const std::string & armyFieldName, const std::optional<int> fixedSize = std::nullopt);

	operator bool() const
	{
		return !stacks.empty();
	}
};

VCMI_LIB_NAMESPACE_END
