#ifndef __CCREATURESET_H__
#define __CCREATURESET_H__

#include "../global.h"
#include <map>

class CCreature;
class CGHeroInstance;

//a few typedefs for CCreatureSet
typedef si32 TSlot;
typedef si32 TQuantity; 
typedef ui32 TCreature; //creature id


class DLL_EXPORT CStackInstance
{
public:
	int idRand; //hlp variable used during loading game -> "id" placeholder for randomization
	const CCreature *type;
	TQuantity count;
	ui32 experience; //TODO: handle
	//TODO: stack artifacts

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h/* & owner*/ & type & count & experience;
	}

	int getQuantityID() const;
	void init();
	CStackInstance();
	CStackInstance(TCreature id, TQuantity count);
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

	void addToSlot(TSlot slot, TCreature cre, TQuantity count); //Adds stack to slot. Slot must be empty or with same type creature
	void addToSlot(TSlot slot, const CStackInstance &stack); //Adds stack to slot. Slot must be empty or with same type creature

	const CCreature* getCreature(TSlot slot) const; //workaround of map issue;
	int getAmount (TSlot slot) const;
	bool setCreature (TSlot slot, TCreature type, TQuantity quantity); //slots 0 to 6
	TSlot getSlotFor(TCreature creature, ui32 slotsAmount=7) const; //returns -1 if no slot available
	bool mergableStacks(std::pair<TSlot, TSlot> &out, TSlot preferable = -1); //looks for two same stacks, returns slot positions;
	bool validTypes(bool allowUnrandomized = false) const; //checks if all types of creatures are set properly

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
