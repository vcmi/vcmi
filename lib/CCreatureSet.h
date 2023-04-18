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

#include "HeroBonus.h"
#include "GameConstants.h"
#include "CArtHandler.h"
#include "CCreatureHandler.h"

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
public:
	const CCreature *type = nullptr;
	TQuantity count = -1; //exact quantity or quantity ID from CCreature::getQuantityID when getting info about enemy army

	CStackBasicDescriptor();
	CStackBasicDescriptor(const CreatureID & id, TQuantity Count);
	CStackBasicDescriptor(const CCreature *c, TQuantity Count);
	virtual ~CStackBasicDescriptor() = default;

	const Creature * getType() const;
	TQuantity getCount() const;

	virtual void setType(const CCreature * c);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		if(h.saving)
		{
			auto idNumber = type ? type->getId() : CreatureID(CreatureID::NONE);
			h & idNumber;
		}
		else
		{
			CreatureID idNumber;
			h & idNumber;
			if(idNumber != CreatureID::NONE)
				setType(dynamic_cast<const CCreature*>(VLC->creatures()->getByIndex(idNumber)));
			else
				type = nullptr;
		}
		h & count;
	}

	void serializeJson(JsonSerializeFormat & handler);
};

class DLL_LINKAGE CStackInstance : public CBonusSystemNode, public CStackBasicDescriptor, public CArtifactSet, public IConstBonusNativeTerrainProvider
{
protected:
	const CArmedInstance *_armyObj; //stack must be part of some army, army must be part of some object

public:
	struct RandomStackInfo
	{
		uint8_t level;
		uint8_t upgrade;
	};
	// helper variable used during loading map, when object (hero or town) have creatures that must have same alignment.
	std::optional<RandomStackInfo> randomStack;

	const CArmedInstance * const & armyObj; //stack must be part of some army, army must be part of some object
	TExpType experience;//commander needs same amount of exp as hero

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CBonusSystemNode&>(*this);
		h & static_cast<CStackBasicDescriptor&>(*this);
		h & static_cast<CArtifactSet&>(*this);
		h & _armyObj;
		h & experience;
		BONUS_TREE_DESERIALIZATION_FIX
	}

	void serializeJson(JsonSerializeFormat & handler);

	//overrides CBonusSystemNode
	std::string bonusToString(const std::shared_ptr<Bonus>& bonus, bool description) const override; // how would bonus description look for this particular type of node
	std::string bonusToGraphics(const std::shared_ptr<Bonus> & bonus) const; //file name of graphics from StackSkills , in future possibly others

	//IConstBonusProvider
	const IBonusBearer* getBonusBearer() const override;
	//INativeTerrainProvider
	FactionID getFaction() const override;

	virtual ui64 getPower() const;
	CCreature::CreatureQuantityId getQuantityID() const;
	std::string getQuantityTXT(bool capitalized = true) const;
	virtual int getExpRank() const;
	virtual int getLevel() const; //different for regular stack and commander
	si32 magicResistance() const override;
	CreatureID getCreatureID() const; //-1 if not available
	std::string getName() const; //plural or singular
	virtual void init();
	CStackInstance();
	CStackInstance(const CreatureID & id, TQuantity count, bool isHypothetic = false);
	CStackInstance(const CCreature *cre, TQuantity count, bool isHypothetic = false);
	virtual ~CStackInstance() = default;

	void setType(const CreatureID & creID);
	void setType(const CCreature * c) override;
	void setArmyObj(const CArmedInstance *ArmyObj);
	virtual void giveStackExp(TExpType exp);
	bool valid(bool allowUnrandomized) const;
	void putArtifact(ArtifactPosition pos, CArtifactInstance * art) override;//from CArtifactSet
	ArtBearer::ArtBearer bearerType() const override; //from CArtifactSet
	virtual std::string nodeName() const override; //from CBonusSystemnode
	void deserializationFix();
	PlayerColor getOwner() const override;
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
	std::set <ui8> specialSKills;
	//std::vector <CArtifactInstance *> arts;
	void init() override;
	CCommanderInstance();
	CCommanderInstance(const CreatureID & id);
	void setAlive (bool alive);
	void giveStackExp (TExpType exp) override;
	void levelUp ();

	bool gainsLevel() const; //true if commander has lower level than should upon his experience
	ui64 getPower() const override {return 0;};
	int getExpRank() const override;
	int getLevel() const override;
	ArtBearer::ArtBearer bearerType() const override; //from CArtifactSet

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CStackInstance&>(*this);
		h & alive;
		h & level;
		h & name;
		h & secondarySkills;
		h & specialSKills;
	}
};

typedef std::map<SlotID, CStackInstance*> TSlots;
typedef std::map<SlotID, std::pair<CreatureID, TQuantity>> TSimpleSlots;

typedef std::pair<const CCreature*, SlotID> TPairCreatureSlot;
typedef std::map<const CCreature*, SlotID> TMapCreatureSlot;

struct DLL_LINKAGE CreatureSlotComparer
{
	bool operator()(const TPairCreatureSlot & lhs, const TPairCreatureSlot & rhs);
};

typedef std::priority_queue<
	TPairCreatureSlot,
	std::vector<TPairCreatureSlot>,
	CreatureSlotComparer
> TCreatureQueue;

class IArmyDescriptor
{
public:
	virtual void clear() = 0;
	virtual bool setCreature(SlotID slot, CreatureID cre, TQuantity count) = 0;
};

//simplified version of CCreatureSet
class DLL_LINKAGE CSimpleArmy : public IArmyDescriptor
{
public:
	TSimpleSlots army;
	void clear() override;
	bool setCreature(SlotID slot, CreatureID cre, TQuantity count) override;
	operator bool() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & army;
	}
};

enum class EArmyFormation : uint8_t
{
	LOOSE,
	TIGHT
};

class DLL_LINKAGE CCreatureSet : public IArmyDescriptor //seven combined creatures
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
	void addToSlot(const SlotID & slot, CStackInstance * stack, bool allowMerging = true); //Adds stack to slot. Slot must be empty or with same type creature
	void clear() override;
	void setFormation(bool tight);
	CArmedInstance *castToArmyObj();

	//basic operations
	void putStack(const SlotID & slot, CStackInstance * stack); //adds new stack to the army, slot must be empty
	void setStackCount(const SlotID & slot, TQuantity count); //stack must exist!
	CStackInstance * detachStack(const SlotID & slot); //removes stack from army but doesn't destroy it (so it can be moved somewhere else or safely deleted)
	void setStackType(const SlotID & slot, const CreatureID & type);
	void giveStackExp(TExpType exp);
	void setStackExp(const SlotID & slot, TExpType exp);

	//derivative
	void eraseStack(const SlotID & slot); //slot must be occupied
	void joinStack(const SlotID & slot, CStackInstance * stack); //adds new stack to the existing stack of the same type
	void changeStackCount(const SlotID & slot, TQuantity toAdd); //stack must exist!
	bool setCreature (SlotID slot, CreatureID type, TQuantity quantity) override; //replaces creature in stack; slots 0 to 6, if quantity=0 erases stack
	void setToArmy(CSimpleArmy &src); //erases all our army and moves stacks from src to us; src MUST NOT be an armed object! WARNING: use it wisely. Or better do not use at all.

	const CStackInstance & getStack(const SlotID & slot) const; //stack must exist
	const CStackInstance * getStackPtr(const SlotID & slot) const; //if stack doesn't exist, returns nullptr
	const CCreature * getCreature(const SlotID & slot) const; //workaround of map issue;
	int getStackCount(const SlotID & slot) const;
	TExpType getStackExperience(const SlotID & slot) const;
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

	bool mergableStacks(std::pair<SlotID, SlotID> & out, const SlotID & preferable = SlotID()) const; //looks for two same stacks, returns slot positions;
	bool validTypes(bool allowUnrandomized = false) const; //checks if all types of creatures are set properly
	bool slotEmpty(const SlotID & slot) const;
	int stacksCount() const;
	virtual bool needsLastStack() const; //true if last stack cannot be taken
	ui64 getArmyStrength() const; //sum of AI values of creatures
	ui64 getPower(const SlotID & slot) const; //value of specific stack
	std::string getRoughAmount(const SlotID & slot, int mode = 0) const; //rough size of specific stack
	std::string getArmyDescription() const;
	bool hasStackAtSlot(const SlotID & slot) const;

	bool contains(const CStackInstance *stack) const;
	bool canBeMergedWith(const CCreatureSet &cs, bool allowMergingStacks = true) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & stacks;
		h & formation;
	}

	void serializeJson(JsonSerializeFormat & handler, const std::string & fieldName, const std::optional<int> fixedSize = std::nullopt);

	operator bool() const
	{
		return !stacks.empty();
	}
	void sweep();
};

VCMI_LIB_NAMESPACE_END
