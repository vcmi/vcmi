#pragma once

/*
 * float3.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

/// Class which consists of three float values. Represents position virtual RMG (0;1) area.
class float3
{
public:
	float x, y;
	si32 z;
	inline float3():x(0),y(0),z(0){}; //c-tor, x/y/z initialized to 0
	inline float3(const float X, const float Y, const si32 Z):x(X),y(Y),z(Z){}; //c-tor
	inline float3(const float3 & val) : x(val.x), y(val.y), z(val.z){} //copy c-tor
	inline float3 & operator=(const float3 & val) {x = val.x; y = val.y; z = val.z; return *this;} //assignemt operator
	~float3() {} // d-tor - does nothing
	inline float3 operator+(const float3 & i) const //returns float3 with coordinates increased by corresponding coordinate of given float3
		{return float3(x+i.x,y+i.y,z+i.z);}
	inline float3 operator+(const float i) const //returns float3 with coordinates increased by given numer
		{return float3(x+i,y+i,z+i);}
	inline float3 operator-(const float3 & i) const //returns float3 with coordinates decreased by corresponding coordinate of given float3
		{return float3(x-i.x,y-i.y,z-i.z);}
	inline float3 operator-(const float i) const //returns float3 with coordinates decreased by given numer
		{return float3(x-i,y-i,z-i);}
	inline float3 operator*(const float i) const //returns float3 with plane coordinates decreased by given numer
		{return float3(x*i, y*i, z);}
	inline float3 operator/(const float i) const //returns float3 with plane coordinates decreased by given numer
		{return float3(x/i, y/i, z);}
	inline float3 operator-() const //returns opposite position
		{return float3(-x,-y,-z);}
	inline double dist2d(const float3 &other) const //distance (z coord is not used)
		{return std::sqrt((double)(x-other.x)*(x-other.x) + (y-other.y)*(y-other.y));}
	inline bool areNeighbours(const float3 &other) const
		{return dist2d(other) < 2. && z == other.z;}
	inline void operator+=(const float3 & i)
	{
		x+=i.x;
		y+=i.y;
		z+=i.z;
	}
	inline void operator+=(const float & i)
	{
		x+=i;
		y+=i;
		z+=i;
	}
	inline void operator-=(const float3 & i)
	{
		x-=i.x;
		y-=i.y;
		z-=i.z;
	}
	inline void operator-=(const float & i)
	{
		x+=i;
		y+=i;
		z+=i;
	}
	inline void operator*=(const float & i) //scale on plane
	{
		x*=i;
		y*=i;
	}
	inline void operator/=(const float & i) //scale on plane
	{
		x/=i;
		y/=i;
	}

	inline bool operator==(const float3 & i) const
		{return (x==i.x) && (y==i.y) && (z==i.z);}
	inline bool operator!=(const float3 & i) const
		{return !(*this==i);}
	inline bool operator<(const float3 & i) const
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
	inline std::string operator ()() const
	{
		return	"(" + boost::lexical_cast<std::string>(x) +
				" " + boost::lexical_cast<std::string>(y) +
				" " + boost::lexical_cast<std::string>(z) + ")";
	}
	inline bool valid() const
	{
		return z >= 0; //minimal condition that needs to be fulfilled for tiles in the map
	}
	template <typename Handler> void serialize(Handler &h, const float version)
	{
		h & x & y & z;
	}
	
};
inline std::istream & operator>>(std::istream & str, float3 & dest)
{
	str>>dest.x>>dest.y>>dest.z;
	return str;
}
inline std::ostream & operator<<(std::ostream & str, const float3 & sth)
{
	return str<<sth.x<<' '<<sth.y<<' '<<sth.z;
}

struct Shashfloat3
{
	size_t operator()(float3 const& pos) const
	{
		size_t ret = std::hash<float>()(pos.x);
		vstd::hash_combine(ret, pos.y);
		vstd::hash_combine(ret, pos.z);
		return ret;
	}
};
