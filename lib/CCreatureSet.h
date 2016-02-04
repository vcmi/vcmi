#pragma once


#include "HeroBonus.h"
#include "GameConstants.h"
#include "CArtHandler.h"

/*
 * CCreatureSet.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CCreature;
class CGHeroInstance;
class CArmedInstance;
class CCreatureArtifactSet;

class DLL_LINKAGE CStackBasicDescriptor
{
public:
	const CCreature *type;
	TQuantity count;

	CStackBasicDescriptor();
	CStackBasicDescriptor(CreatureID id, TQuantity Count);
	CStackBasicDescriptor(const CCreature *c, TQuantity Count);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & type & count;
	}
};

class DLL_LINKAGE CStackInstance : public CBonusSystemNode, public CStackBasicDescriptor, public CArtifactSet
{
protected:
	const CArmedInstance *_armyObj; //stack must be part of some army, army must be part of some object
public:
	// hlp variable used during loading map, when object (hero or town) have creatures that must have same alignment.
	// idRand < 0 -> normal, non-random creature
	// idRand / 2 -> level
	// idRand % 2 -> upgrade number
	int idRand;

	const CArmedInstance * const & armyObj; //stack must be part of some army, army must be part of some object
	TExpType experience;//commander needs same amount of exp as hero

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CBonusSystemNode&>(*this);
		h & static_cast<CStackBasicDescriptor&>(*this);
		h & static_cast<CArtifactSet&>(*this);
		h & _armyObj & experience;
		BONUS_TREE_DESERIALIZATION_FIX
	}

	//overrides CBonusSystemNode
	std::string bonusToString(const Bonus *bonus, bool description) const override; // how would bonus description look for this particular type of node
	std::string bonusToGraphics(const Bonus *bonus) const; //file name of graphics from StackSkills , in future possibly others

	virtual ui64 getPower() const;
	int getQuantityID() const;
	std::string getQuantityTXT(bool capitalized = true) const;
	virtual int getExpRank() const;
	virtual int getLevel() const; //different for regular stack and commander
	si32 magicResistance() const override;
	CreatureID getCreatureID() const; //-1 if not available
	std::string getName() const; //plural or singular
	virtual void init();
	CStackInstance();
	CStackInstance(CreatureID id, TQuantity count);
	CStackInstance(const CCreature *cre, TQuantity count);
	~CStackInstance();

	void setType(CreatureID creID);
	void setType(const CCreature *c);
	void setArmyObj(const CArmedInstance *ArmyObj);
	virtual void giveStackExp(TExpType exp);
	bool valid(bool allowUnrandomized) const;
	ArtBearer::ArtBearer bearerType() const override; //from CArtifactSet
	virtual std::string nodeName() const override; //from CBonusSystemnode
	void deserializationFix();
};

class DLL_LINKAGE CCommanderInstance : public CStackInstance
{
public:
	//TODO: what if Commander is not a part of creature set?

	//commander class is determined by its base creature
	ui8 alive;
	ui8 level; //required only to count callbacks
	std::string name; // each Commander has different name
	std::vector <ui8> secondarySkills; //ID -> level
	std::set <ui8> specialSKills;
	//std::vector <CArtifactInstance *> arts;
	void init() override;
	CCommanderInstance();
	CCommanderInstance (CreatureID id);
	~CCommanderInstance();
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
		h & alive & level & name & secondarySkills & specialSKills;
	}
};

DLL_LINKAGE std::ostream & operator<<(std::ostream & str, const CStackInstance & sth);

typedef std::map<SlotID, CStackInstance*> TSlots;
typedef std::map<SlotID, CStackBasicDescriptor> TSimpleSlots;

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

class DLL_LINKAGE CCreatureSet : public IArmyDescriptor //seven combined creatures
{
	CCreatureSet(const CCreatureSet&);
	CCreatureSet &operator=(const CCreatureSet&);
public:
	TSlots stacks; //slots[slot_id]->> pair(creature_id,creature_quantity)
	ui8 formation; //false - wide, true - tight

	CCreatureSet();
	virtual ~CCreatureSet();
	virtual void armyChanged();

	const CStackInstance &operator[](SlotID slot) const;

	const TSlots &Slots() const {return stacks;}

	void addToSlot(SlotID slot, CreatureID cre, TQuantity count, bool allowMerging = true); //Adds stack to slot. Slot must be empty or with same type creature
	void addToSlot(SlotID slot, CStackInstance *stack, bool allowMerging = true); //Adds stack to slot. Slot must be empty or with same type creature
	void clear() override;
	void setFormation(bool tight);
	CArmedInstance *castToArmyObj();

	//basic operations
	void putStack(SlotID slot, CStackInstance *stack); //adds new stack to the army, slot must be empty
	void setStackCount(SlotID slot, TQuantity count); //stack must exist!
	CStackInstance *detachStack(SlotID slot); //removes stack from army but doesn't destroy it (so it can be moved somewhere else or safely deleted)
	void setStackType(SlotID slot, const CCreature *type);
	void giveStackExp(TExpType exp);
	void setStackExp(SlotID slot, TExpType exp);

	//derivative
	void eraseStack(SlotID slot); //slot must be occupied
	void joinStack(SlotID slot, CStackInstance * stack); //adds new stack to the existing stack of the same type
	void changeStackCount(SlotID slot, TQuantity toAdd); //stack must exist!
	bool setCreature (SlotID slot, CreatureID type, TQuantity quantity) override; //replaces creature in stack; slots 0 to 6, if quantity=0 erases stack
	void setToArmy(CSimpleArmy &src); //erases all our army and moves stacks from src to us; src MUST NOT be an armed object! WARNING: use it wisely. Or better do not use at all.

	const CStackInstance& getStack(SlotID slot) const; //stack must exist
	const CStackInstance* getStackPtr(SlotID slot) const; //if stack doesn't exist, returns nullptr
	const CCreature* getCreature(SlotID slot) const; //workaround of map issue;
	int getStackCount (SlotID slot) const;
	TExpType getStackExperience(SlotID slot) const;
	SlotID findStack(const CStackInstance *stack) const; //-1 if none
	SlotID getSlotFor(CreatureID creature, ui32 slotsAmount = GameConstants::ARMY_SIZE) const; //returns -1 if no slot available
	SlotID getSlotFor(const CCreature *c, ui32 slotsAmount = GameConstants::ARMY_SIZE) const; //returns -1 if no slot available
	SlotID getFreeSlot(ui32 slotsAmount = GameConstants::ARMY_SIZE) const;
	bool mergableStacks(std::pair<SlotID, SlotID> &out, SlotID preferable = SlotID()) const; //looks for two same stacks, returns slot positions;
	bool validTypes(bool allowUnrandomized = false) const; //checks if all types of creatures are set properly
	bool slotEmpty(SlotID slot) const;
	int stacksCount() const;
	virtual bool needsLastStack() const; //true if last stack cannot be taken
	ui64 getArmyStrength() const; //sum of AI values of creatures
	ui64 getPower (SlotID slot) const; //value of specific stack
	std::string getRoughAmount(SlotID slot, int mode = 0) const; //rough size of specific stack
	std::string getArmyDescription() const;
	bool hasStackAtSlot(SlotID slot) const;

	bool contains(const CStackInstance *stack) const;
	bool canBeMergedWith(const CCreatureSet &cs, bool allowMergingStacks = true) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & stacks & formation;
	}
	operator bool() const
	{
		return !stacks.empty();
	}
	void sweep();
};
