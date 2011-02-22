#ifndef __INT3_H__
#define __INT3_H__

/*
 * int3.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include <cmath>

/// Class which consists of three integer values. Represents position on adventure map.
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

struct ShashInt3
{
	size_t operator()(int3 const& pos) const
	{
		size_t ret = ::boost::hash<int>()(pos.x);
		boost::hash_combine(ret, pos.y);
		boost::hash_combine(ret, pos.z);
		return ret;
	}
};

#endif // __INT3_H__
