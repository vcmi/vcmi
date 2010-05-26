#ifndef __CCREATURESET_H__
#define __CCREATURESET_H__

#include "../global.h"
#include <map>
#include "HeroBonus.h"

class CCreature;
class CGHeroInstance;
class CArmedInstance;

//a few typedefs for CCreatureSet
typedef si32 TSlot;
typedef si32 TQuantity; 
typedef ui32 TCreature; //creature id

const int ARMY_SIZE = 7;


class DLL_EXPORT CStackInstance : public CBonusSystemNode
{
public:
	int idRand; //hlp variable used during loading game -> "id" placeholder for randomization

	const CArmedInstance *armyObj; //stack must be part of some army, army must be part of some object
	const CCreature *type;
	TQuantity count;
	ui32 experience; //TODO: handle
	//TODO: stack artifacts

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CBonusSystemNode&>(*this);
		h & armyObj & type & count & experience;
	}

	//overrides CBonusSystemNode
	void getParents(TCNodes &out, const CBonusSystemNode *source = NULL) const;  //retrieves list of parent nodes (nodes to inherit bonuses from), source is the prinary asker

	int getQuantityID() const;
	void init();
	CStackInstance();
	CStackInstance(TCreature id, TQuantity count, const CArmedInstance *ArmyObj = NULL);
	CStackInstance(const CCreature *cre, TQuantity count);
	void setType(int creID);
};


typedef std::map<TSlot, CStackInstance> TSlots;



class DLL_EXPORT CCreatureSet //seven combined creatures
{
public:
	TSlots slots; //slots[slot_id]=> pair(creature_id,creature_quantity)
	ui8 formation; //false - wide, true - tight

	const CStackInstance operator[](TSlot slot) const; 

	const TSlots &Slots() const {return slots;}

	void addToSlot(TSlot slot, TCreature cre, TQuantity count, bool allowMerging = true); //Adds stack to slot. Slot must be empty or with same type creature
	void addToSlot(TSlot slot, const CStackInstance &stack, bool allowMerging = true); //Adds stack to slot. Slot must be empty or with same type creature
	void addStack(TSlot slot, const CStackInstance &stack); //adds new stack to the army, slot must be empty
	bool setCreature (TSlot slot, TCreature type, TQuantity quantity); //slots 0 to 6, if quantity=0, erases stack
	void clear();
	void setFormation(bool tight);
	void setStackCount(TSlot slot, TQuantity count); //stack must exist!
	void eraseStack(TSlot slot);
	
	const CStackInstance& getStack(TSlot slot) const; 
	const CCreature* getCreature(TSlot slot) const; //workaround of map issue;
	int getAmount (TSlot slot) const;
	TSlot getSlotFor(TCreature creature, ui32 slotsAmount=ARMY_SIZE) const; //returns -1 if no slot available
	bool mergableStacks(std::pair<TSlot, TSlot> &out, TSlot preferable = -1) const; //looks for two same stacks, returns slot positions;
	bool validTypes(bool allowUnrandomized = false) const; //checks if all types of creatures are set properly
	bool slotEmpty(TSlot slot) const;
	int stacksCount() const;
	virtual bool needsLastStack() const; //true if last stack cannot be taken
	int getArmyStrength() const; //sum of AI values of creatures
	ui64 getPower (TSlot slot) const; //value of specific stack
	std::string getRoughAmount (TSlot slot) const; //rough size of specific stack
	
	bool contains(const CStackInstance *stack) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & slots & formation;
	}
	operator bool() const
	{
		return slots.size() > 0;
	}
	void sweep();
};

#endif // __CCREATURESET_H__
