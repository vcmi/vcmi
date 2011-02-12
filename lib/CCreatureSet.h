#ifndef __CCREATURESET_H__
#define __CCREATURESET_H__

#include "../global.h"
#include <map>
#include "HeroBonus.h"

class CCreature;
class CGHeroInstance;
class CArmedInstance;


class DLL_EXPORT CStackBasicDescriptor
{
public:
	const CCreature *type;
	TQuantity count;

	CStackBasicDescriptor();
	CStackBasicDescriptor(TCreature id, TQuantity Count);
	CStackBasicDescriptor(const CCreature *c, TQuantity Count);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & type & count;
	}
};

class DLL_EXPORT CStackInstance : public CBonusSystemNode, public CStackBasicDescriptor
{
	const CArmedInstance *_armyObj; //stack must be part of some army, army must be part of some object
public:
	int idRand; //hlp variable used during loading game -> "id" placeholder for randomization

	const CArmedInstance * const & armyObj; //stack must be part of some army, army must be part of some object
	ui32 experience; //TODO: handle
	//TODO: stack artifacts

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CBonusSystemNode&>(*this);
		h & static_cast<CStackBasicDescriptor&>(*this);
		h & _armyObj & experience;
	}

	//overrides CBonusSystemNode
	//void getParents(TCNodes &out, const CBonusSystemNode *source = NULL) const;  //retrieves list of parent nodes (nodes to inherit bonuses from), source is the prinary asker
	std::string bonusToString(Bonus *bonus, bool description) const; // how would bonus description look for this particular type of node

	int getQuantityID() const;
	std::string getQuantityTXT(bool capitalized = true) const;
	int getExpRank() const;
	void init();
	CStackInstance();
	CStackInstance(TCreature id, TQuantity count);
	CStackInstance(const CCreature *cre, TQuantity count);
	~CStackInstance();

	void setType(int creID);
	void setType(const CCreature *c);
	void setArmyObj(const CArmedInstance *ArmyObj);
	void giveStackExp(expType exp);
	bool valid(bool allowUnrandomized) const;
	virtual std::string nodeName() const OVERRIDE;
	void deserializationFix();
};

DLL_EXPORT std::ostream & operator<<(std::ostream & str, const CStackInstance & sth);

typedef std::map<TSlot, CStackInstance*> TSlots;
typedef std::map<TSlot, CStackBasicDescriptor> TSimpleSlots;

class IArmyDescriptor
{
public:
	virtual void clear() = 0;
	virtual bool setCreature(TSlot slot, TCreature cre, TQuantity count) = 0;
};

//simplified version of CCreatureSet
class DLL_EXPORT CSimpleArmy : public IArmyDescriptor
{
public:
	TSimpleSlots army;
	void clear() OVERRIDE;
	bool setCreature(TSlot slot, TCreature cre, TQuantity count) OVERRIDE;
	operator bool() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & army;
	}
};

class DLL_EXPORT CCreatureSet : public IArmyDescriptor //seven combined creatures
{
	CCreatureSet(const CCreatureSet&);;
	CCreatureSet &operator=(const CCreatureSet&);
public:
	TSlots stacks; //slots[slot_id]->> pair(creature_id,creature_quantity)
	ui8 formation; //false - wide, true - tight

	CCreatureSet();
	virtual ~CCreatureSet();
	virtual void armyChanged();

	const CStackInstance &operator[](TSlot slot) const; 

	const TSlots &Slots() const {return stacks;}

	void addToSlot(TSlot slot, TCreature cre, TQuantity count, bool allowMerging = true); //Adds stack to slot. Slot must be empty or with same type creature
	void addToSlot(TSlot slot, CStackInstance *stack, bool allowMerging = true); //Adds stack to slot. Slot must be empty or with same type creature
	void clear() OVERRIDE;
	void setFormation(bool tight);
	CArmedInstance *castToArmyObj();

	//basic operations
	void putStack(TSlot slot, CStackInstance *stack); //adds new stack to the army, slot must be empty
	void setStackCount(TSlot slot, TQuantity count); //stack must exist!
	CStackInstance *detachStack(TSlot slot); //removes stack from army but doesn't destroy it (so it can be moved somewhere else or safely deleted)
	void setStackType(TSlot slot, const CCreature *type);
	void giveStackExp(expType exp);

	//derivative 
	void eraseStack(TSlot slot); //slot must be occupied
	void joinStack(TSlot slot, CStackInstance * stack); //adds new stack to the existing stack of the same type
	void changeStackCount(TSlot slot, TQuantity toAdd); //stack must exist!
	bool setCreature (TSlot slot, TCreature type, TQuantity quantity) OVERRIDE; //replaces creature in stack; slots 0 to 6, if quantity=0 erases stack
	void setToArmy(CSimpleArmy &src); //erases all our army and moves stacks from src to us; src MUST NOT be an armed object! WARNING: use it wisely. Or better do not use at all.
	
	const CStackInstance& getStack(TSlot slot) const; 
	const CCreature* getCreature(TSlot slot) const; //workaround of map issue;
	int getStackCount (TSlot slot) const;
	TSlot findStack(const CStackInstance *stack) const; //-1 if none
	TSlot getSlotFor(TCreature creature, ui32 slotsAmount=ARMY_SIZE) const; //returns -1 if no slot available
	TSlot getSlotFor(const CCreature *c, ui32 slotsAmount=ARMY_SIZE) const; //returns -1 if no slot available
	bool mergableStacks(std::pair<TSlot, TSlot> &out, TSlot preferable = -1) const; //looks for two same stacks, returns slot positions;
	bool validTypes(bool allowUnrandomized = false) const; //checks if all types of creatures are set properly
	bool slotEmpty(TSlot slot) const;
	int stacksCount() const;
	virtual bool needsLastStack() const; //true if last stack cannot be taken
	int getArmyStrength() const; //sum of AI values of creatures
	ui64 getPower (TSlot slot) const; //value of specific stack
	std::string getRoughAmount (TSlot slot) const; //rough size of specific stack
	bool hasStackAtSlot(TSlot slot) const;
	
	bool contains(const CStackInstance *stack) const;
	bool canBeMergedWith(const CCreatureSet &cs, bool allowMergingStacks = true) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & stacks & formation;
	}
	operator bool() const
	{
		return stacks.size() > 0;
	}
	void sweep();
};

#endif // __CCREATURESET_H__
