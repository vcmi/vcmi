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

#include "CCreatureHandler.h"
#include "GameConstants.h"
#include "bonuses/Bonus.h"
#include "bonuses/BonusCache.h"
#include "bonuses/CBonusSystemNode.h"
#include "callback/GameCallbackHolder.h"
#include "serializer/Serializeable.h"
#include "mapObjects/CGObjectInstance.h"
#include "entities/artifact/CArtifactSet.h"

#include <vcmi/Entity.h>

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;
class CCreature;
class CGHeroInstance;
class CArmedInstance;
class CCreatureArtifactSet;
class JsonSerializeFormat;

class DLL_LINKAGE CStackBasicDescriptor
{
	CreatureID typeID;
	TQuantity count = -1; //exact quantity or quantity ID from CCreature::getQuantityID when getting info about enemy army

public:
	CStackBasicDescriptor();
	CStackBasicDescriptor(const CreatureID & id, TQuantity Count);
	CStackBasicDescriptor(const CCreature *c, TQuantity Count);
	virtual ~CStackBasicDescriptor() = default;

	const Creature * getType() const;
	const CCreature * getCreature() const;
	CreatureID getId() const;
	TQuantity getCount() const;

	virtual void setType(const CCreature * c);
	virtual void setCount(TQuantity amount);

	friend bool operator== (const CStackBasicDescriptor & l, const CStackBasicDescriptor & r);

	template <typename Handler> void serialize(Handler &h)
	{
		if(h.saving)
		{
			h & typeID;
		}
		else
		{
			CreatureID creatureID;
			h & creatureID;
			if(creatureID != CreatureID::NONE)
				setType(creatureID.toCreature());
		}

		h & count;
	}

	void serializeJson(JsonSerializeFormat & handler);
};

class DLL_LINKAGE CStackInstance : public CBonusSystemNode, public CStackBasicDescriptor, public CArtifactSet, public ACreature, public GameCallbackHolder
{
	BonusValueCache nativeTerrain;
	BonusValueCache initiative;

	CArmedInstance * armyInstance = nullptr; //stack must be part of some army, army must be part of some object

	IGameInfoCallback * getCallback() const final { return cb; }

	TExpType totalExperience;//commander needs same amount of exp as hero
public:
	struct RandomStackInfo
	{
		uint8_t level;
		uint8_t upgrade;
	};
	// helper variable used during loading map, when object (hero or town) have creatures that must have same alignment.
	std::optional<RandomStackInfo> randomStack;

	CArmedInstance * getArmy();
	const CArmedInstance * getArmy() const; //stack must be part of some army, army must be part of some object
	void setArmy(CArmedInstance *ArmyObj);

	TExpType getTotalExperience() const;
	TExpType getAverageExperience() const;
	virtual bool canGainExperience() const;

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CBonusSystemNode&>(*this);
		h & static_cast<CStackBasicDescriptor&>(*this);
		h & static_cast<CArtifactSet&>(*this);

		if (h.hasFeature(Handler::Version::STACK_INSTANCE_ARMY_FIX))
		{
			// no-op
		}
		if (h.hasFeature(Handler::Version::NO_RAW_POINTERS_IN_SERIALIZER))
		{
			ObjectInstanceID dummyID;
			h & dummyID;
		}
		else
		{
			std::shared_ptr<CGObjectInstance> army;
			h & army;
		}

		h & totalExperience;
		if (!h.hasFeature(Handler::Version::STACK_INSTANCE_EXPERIENCE_FIX))
		{
			totalExperience *= getCount();
		}
	}

	void serializeJson(JsonSerializeFormat & handler);

	//overrides CBonusSystemNode
	std::string bonusToString(const std::shared_ptr<Bonus>& bonus) const override; // how would bonus description look for this particular type of node
	ImagePath bonusToGraphics(const std::shared_ptr<Bonus> & bonus) const; //file name of graphics from StackSkills , in future possibly others

	//IConstBonusProvider
	const IBonusBearer* getBonusBearer() const override;
	//INativeTerrainProvider
	FactionID getFactionID() const override;

	virtual ui64 getPower() const;
	/// Returns total market value of resources needed to recruit this unit
	virtual ui64 getMarketValue() const;
	CCreature::CreatureQuantityId getQuantityID() const;
	std::string getQuantityTXT(bool capitalized = true) const;
	virtual int getExpRank() const;
	virtual int getLevel() const; //different for regular stack and commander
	CreatureID getCreatureID() const; //-1 if not available
	std::string getName() const; //plural or singular
	CStackInstance(IGameInfoCallback *cb, bool isHypothetic	= false);
	CStackInstance(IGameInfoCallback *cb, const CreatureID & id, TQuantity count, bool isHypothetic = false);
	virtual ~CStackInstance() = default;

	void setType(const CreatureID & creID);
	void setType(const CCreature * c) final;
	void setCount(TQuantity amount) final;

	/// Gives specified amount of stack experience that will not be scaled by unit size
	void giveAverageStackExperience(TExpType exp);
	void giveTotalStackExperience(TExpType exp);

	bool valid(bool allowUnrandomized) const;
	ArtPlacementMap putArtifact(const ArtifactPosition & pos, const CArtifactInstance * art) override;//from CArtifactSet
	void removeArtifact(const ArtifactPosition & pos) override;
	ArtBearer bearerType() const override; //from CArtifactSet
	std::string nodeName() const override; //from CBonusSystemnode
	PlayerColor getOwner() const override;

	int32_t getInitiative(int turn = 0) const final;
	TerrainId getNativeTerrain() const final;
	TerrainId getCurrentTerrain() const;
};

class DLL_LINKAGE CCommanderInstance : public CStackInstance
{
public:
	//TODO: what if Commander is not a part of creature set?

	//commander class is determined by its base creature
	ui8 alive; //maybe change to bool when breaking save compatibility?
	ui8 level; //required only to count callbacks
	std::string name; // each Commander has different name
	std::vector <ui8> secondarySkills; //ID -> level
	std::set <ui8> specialSkills;
	//std::vector <CArtifactInstance *> arts;
	CCommanderInstance(IGameInfoCallback *cb);
	CCommanderInstance(IGameInfoCallback *cb, const CreatureID & id);
	void setAlive (bool alive);
	void levelUp ();

	bool canGainExperience() const override;
	bool gainsLevel() const; //true if commander has lower level than should upon his experience
	ui64 getPower() const override {return 0;};
	int getExpRank() const override;
	int getLevel() const override;
	ArtBearer bearerType() const override; //from CArtifactSet

	template <typename Handler> void serialize(Handler &h)
	{
		h & static_cast<CStackInstance&>(*this);
		h & alive;
		h & level;
		h & name;
		h & secondarySkills;
		h & specialSkills;
	}
};

using TSlots = std::map<SlotID, std::unique_ptr<CStackInstance>>;
using TSimpleSlots = std::map<SlotID, std::pair<CreatureID, TQuantity>>;

using TPairCreatureSlot = std::pair<const CCreature *, SlotID>;
using TMapCreatureSlot = std::map<const CCreature *, SlotID>;

struct DLL_LINKAGE CreatureSlotComparer
{
	bool operator()(const TPairCreatureSlot & lhs, const TPairCreatureSlot & rhs);
};

using TCreatureQueue = std::priority_queue<TPairCreatureSlot, std::vector<TPairCreatureSlot>, CreatureSlotComparer>;

class IArmyDescriptor
{
public:
	virtual void clearSlots() = 0;
	virtual bool setCreature(SlotID slot, CreatureID cre, TQuantity count) = 0;
};

//simplified version of CCreatureSet
class DLL_LINKAGE CSimpleArmy : public IArmyDescriptor
{
public:
	TSimpleSlots army;
	void clearSlots() override;
	bool setCreature(SlotID slot, CreatureID cre, TQuantity count) override;
	operator bool() const;

	template <typename Handler> void serialize(Handler &h)
	{
		h & army;
	}
};

namespace NArmyFormation
{
	static const std::vector<std::string> names{ "wide", "tight" };
}

class DLL_LINKAGE CCreatureSet : public IArmyDescriptor, public virtual Serializeable //seven combined creatures
{
	CCreatureSet(const CCreatureSet &) = delete;
	CCreatureSet &operator=(const CCreatureSet&);

public:
	TSlots stacks; //slots[slot_id]->> pair(creature_id,creature_quantity)
	EArmyFormation formation = EArmyFormation::LOOSE; //0 - wide, 1 - tight

	CCreatureSet() = default; //Should be here to avoid compile errors
	virtual ~CCreatureSet();
	virtual void armyChanged();

	const CStackInstance & operator[](const SlotID & slot) const;

	const TSlots &Slots() const {return stacks;}

	void addToSlot(const SlotID & slot, const CreatureID & cre, TQuantity count, bool allowMerging = true); //Adds stack to slot. Slot must be empty or with same type creature
	void addToSlot(const SlotID & slot, std::unique_ptr<CStackInstance> stack, bool allowMerging = true); //Adds stack to slot. Slot must be empty or with same type creature
	void clearSlots() override;
	void setFormation(EArmyFormation tight);
	virtual CArmedInstance * getArmy() { return nullptr; }
	virtual const CArmedInstance * getArmy() const { return nullptr; }

	//basic operations
	void putStack(const SlotID & slot, std::unique_ptr<CStackInstance> stack); //adds new stack to the army, slot must be empty
	void setStackCount(const SlotID & slot, TQuantity count); //stack must exist!
	std::unique_ptr<CStackInstance> detachStack(const SlotID & slot); //removes stack from army but doesn't destroy it (so it can be moved somewhere else or safely deleted)
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

	void changeStackCount(const SlotID & slot, TQuantity toAdd); //stack must exist!
	bool setCreature (SlotID slot, CreatureID type, TQuantity quantity) override; //replaces creature in stack; slots 0 to 6, if quantity=0 erases stack
	void setToArmy(CSimpleArmy &src); //erases all our army and moves stacks from src to us; src MUST NOT be an armed object! WARNING: use it wisely. Or better do not use at all.

	const CStackInstance & getStack(const SlotID & slot) const; //stack must exist
	CStackInstance * getStackPtr(const SlotID & slot) const; //if stack doesn't exist, returns nullptr
	const CCreature * getCreature(const SlotID & slot) const; //workaround of map issue;
	int getStackCount(const SlotID & slot) const;
	TExpType getStackTotalExperience(const SlotID & slot) const;
	TExpType getStackAverageExperience(const SlotID & slot) const;
	SlotID findStack(const CStackInstance *stack) const; //-1 if none
	SlotID getSlotFor(const CreatureID & creature, ui32 slotsAmount = GameConstants::ARMY_SIZE) const; //returns -1 if no slot available
	SlotID getSlotFor(const CCreature *c, ui32 slotsAmount = GameConstants::ARMY_SIZE) const; //returns -1 if no slot available
	bool hasCreatureSlots(const CCreature * c, const SlotID & exclude) const;
	std::vector<SlotID> getCreatureSlots(const CCreature * c, const SlotID & exclude, TQuantity ignoreAmount = -1) const;
	bool isCreatureBalanced(const CCreature* c, TQuantity ignoreAmount = 1) const; // Check if the creature is evenly distributed across slots

	SlotID getFreeSlot(ui32 slotsAmount = GameConstants::ARMY_SIZE) const; //returns first free slot
	std::vector<SlotID> getFreeSlots(ui32 slotsAmount = GameConstants::ARMY_SIZE) const;
	std::queue<SlotID> getFreeSlotsQueue(ui32 slotsAmount = GameConstants::ARMY_SIZE) const;

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

	bool contains(const CStackInstance *stack) const;
	bool canBeMergedWith(const CCreatureSet &cs, bool allowMergingStacks = true) const;

	/// Returns true if this creature set contains all listed units
	/// If requireLastStack is true, then this function will also
	/// require presence of any unit other than requested (or more units than requested)
	bool hasUnits(const std::vector<CStackBasicDescriptor> & units, bool requireLastStack = true) const;

	template <typename Handler> void serialize(Handler &h)
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
