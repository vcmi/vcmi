#ifndef __INT3_H__
#define __INT3_H__
#include <map>
#include <vector>
#include <cmath>

/*
 * int3.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CCreature;

//a few typedefs for CCreatureSet
typedef si32 TSlot, TQuantity;
typedef ui32 TCreature;
typedef std::pair<TCreature, TQuantity> TStack;
typedef std::map<TSlot, TStack> TSlots;

class CCreatureSet //seven combined creatures
{
public:
	TSlots slots; //slots[slot_id]=> pair(creature_id,creature_quantity)
	ui8 formation; //false - wide, true - tight

	int getCreature (TSlot slot) const //workaround of map issue
	{
		std::map<TSlot, TStack>::const_iterator i = slots.find(slot);
		if (i != slots.end())
			return i->second.first;
		else
			return -1;
	}
	int getAmount (TSlot slot) const
	{
		std::map<TSlot, TStack>::const_iterator i = slots.find(slot);
		if (i != slots.end())
			return i->second.second;
		else
			return -1;
	}
	bool setCreature (TSlot slot, TCreature type, TQuantity quantity) //slots 0 to 6
	{
		slots[slot] = TStack(type, quantity);  //brutal force
		if (quantity == 0)
			slots.erase(slot);
		if (slots.size() > 7) return false;
		else return true;
	}
	TSlot getSlotFor(TCreature creature, ui32 slotsAmount=7) const //returns -1 if no slot available
	{	
		for(TSlots::const_iterator i=slots.begin(); i!=slots.end(); ++i)
		{
			if(i->second.first == creature)
			{
				return i->first; //if there is already such creature we return its slot id
			}
		}
		for(ui32 i=0; i<slotsAmount; i++)
		{
			if(slots.find(i) == slots.end())
			{
				return i; //return first free slot
			}
		}
		return -1; //no slot available
	}
	bool mergableStacks(std::pair<TSlot, TSlot> &out, TSlot preferable = -1) //looks for two same stacks, returns slot positions
	{
		//try to match creature to our preferred stack
		if(preferable >= 0  &&  slots.find(preferable) != slots.end())
		{
			TCreature id = slots[preferable].first;
			for(TSlots::const_iterator j=slots.begin(); j!=slots.end(); ++j)
			{
				if(id == j->second.first)
				{
					out.first = preferable;
					out.second = j->first;
					return true;
				}
			}
		}

		for(TSlots::const_iterator i=slots.begin(); i!=slots.end(); ++i)
		{
			for(TSlots::const_iterator j=slots.begin(); j!=slots.end(); ++j)
			{
				if(i->second.first == j->second.first)
				{
					out.first = i->first;
					out.second = j->first;
					return true;
				}
			}
		}
		return false;
	}
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & slots & formation;
	}
	void sweep()
	{
		for(TSlots::iterator i=slots.begin(); i!=slots.end(); ++i)
		{
			if(!i->second.second)
			{
				slots.erase(i);
				sweep();
				break;
			}
		}
	}
};

class int3
{
public:
	si32 x,y,z;
	inline int3():x(0),y(0),z(0){}; //c-tor, x/y/z initialized to 0
	inline int3(const si32 & X, const si32 & Y, const si32 & Z):x(X),y(Y),z(Z){}; //c-tor
	inline int3(const int3 & val) : x(val.x), y(val.y), z(val.z){} //copy c-tor
	inline int3 operator=(const int3 & val) {x = val.x; y = val.y; z = val.z; return *this;} //assignemt operator
	~int3() {} // d-tor - does nothing
	inline int3 operator+(const int3 & i) const //returns int3 with coordinates increased by corresponding coordinate of given int3
		{return int3(x+i.x,y+i.y,z+i.z);}
	inline int3 operator+(const si32 i) const //returns int3 with coordinates increased by given numer
		{return int3(x+i,y+i,z+i);}
	inline int3 operator-(const int3 & i) const //returns int3 with coordinates decreased by corresponding coordinate of given int3
		{return int3(x-i.x,y-i.y,z-i.z);}
	inline int3 operator-(const si32 i) const //returns int3 with coordinates decreased by given numer
		{return int3(x-i,y-i,z-i);}
	inline int3 operator-() const //returns opposite position
		{return int3(-x,-y,-z);}
	inline double dist2d(const int3 other) const //distance (z coord is not used)
		{return std::sqrt((double)(x-other.x)*(x-other.x) + (y-other.y)*(y-other.y));}
	inline void operator+=(const int3 & i)
	{
		x+=i.x;
		y+=i.y;
		z+=i.z;
	}
	inline void operator+=(const si32 & i)
	{
		x+=i;
		y+=i;
		z+=i;
	}
	inline void operator-=(const int3 & i)
	{
		x-=i.x;
		y-=i.y;
		z-=i.z;
	}
	inline void operator-=(const si32 & i)
	{
		x+=i;
		y+=i;
		z+=i;
	}
	inline bool operator==(const int3 & i) const
		{return (x==i.x) && (y==i.y) && (z==i.z);}
	inline bool operator!=(const int3 & i) const
		{return !(*this==i);}
	inline bool operator<(const int3 & i) const
	{
		if (z<i.z)
			return true;
		if (z>i.z)
			return false;
		if (y<i.y)
			return true;
		if (y>i.y)
			return false;
		if (x<i.x)
			return true;
		if (x>i.x)
			return false;
		return false;
	}
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & x & y & z;
	}
};
inline std::istream & operator>>(std::istream & str, int3 & dest)
{
	str>>dest.x>>dest.y>>dest.z;
	return str;
}
inline std::ostream & operator<<(std::ostream & str, const int3 & sth)
{
	return str<<sth.x<<' '<<sth.y<<' '<<sth.z;
}

#endif // __INT3_H__
