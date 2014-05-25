#pragma once

/*
* int3.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/


/// Class which consists of three integer values. Represents position on adventure map.
class int3
{
public:
	si32 x, y, z;

	int3(); //c-tor, x/y/z initialized to 0
	explicit int3(const si32 i); //c-tor, x/y/z initialized to i
	int3(const si32 X, const si32 Y, const si32 Z); //c-tor, x/y/z initialized to X/Y/Z
	int3(const int3 & copy);

	int3 & operator=(const int3 & copy);

	int3 operator-() const;

	int3 operator+(const int3 & i) const;
	int3 operator-(const int3 & i) const;
	int3 operator+(const si32 i) const; //returns int3 with coordinates increased by given number
	int3 operator-(const si32 i) const; //returns int3 with coordinates decreased by given number

	int3 & operator+=(const int3 & i);
	int3 & operator-=(const int3 & i);
	int3 & operator+=(const si32 i); //increases coordinates by given number
	int3 & operator-=(const si32 i); //decreases coordinates by given number

	bool operator==(const int3 & i) const;
	bool operator!=(const int3 & i) const;

	bool operator<(const int3 & i) const;

	si32 dist2dSQ(const int3 & other) const; //returns squared distance on Oxy plane (z coord is not used)
	double dist2d(const int3 & other) const; //returns distance on Oxy plane

	bool areNeighbours(const int3 & other) const;

	std::string operator ()() const; //returns "(x y z)" string

	bool valid() const;

	template <typename Handler>
	void serialize(Handler &h, const int version);
};

inline int3::int3() : x(0), y(0), z(0) {} // I think that x, y, z should be left uninitialized.
inline int3::int3(const si32 i) : x(i), y(i), z(i) {}
inline int3::int3(const si32 X, const si32 Y, const si32 Z) : x(X), y(Y), z(Z) {}
inline int3::int3(const int3 & c) : x(c.x), y(c.y), z(c.z) {} // Should be set to default (C++11)?

inline int3 & int3::operator=(const int3 & c) // Should be set to default (C++11)?
{
	x = c.x;
	y = c.y;
	z = c.z;

	return *this;
}

inline int3 int3::operator-() const { return int3(-x, -y, -z); }

inline int3 int3::operator+(const int3 & i) const { return int3(x + i.x, y + i.y, z + i.z); }
inline int3 int3::operator-(const int3 & i) const { return int3(x - i.x, y - i.y, z - i.z); }
inline int3 int3::operator+(const si32 i) const  { return int3(x + i, y + i, z + i); }
inline int3 int3::operator-(const si32 i) const { return int3(x - i, y - i, z - i); }

inline int3 & int3::operator+=(const int3 & i)
{
	x += i.x;
	y += i.y;
	z += i.z;
	return *this;
}
inline int3 & int3::operator-=(const int3 & i)
{
	x -= i.x;
	y -= i.y;
	z -= i.z;
	return *this;
}
inline int3 & int3::operator+=(const si32 i)
{
	x += i;
	y += i;
	z += i;
	return *this;
}
inline int3 & int3::operator-=(const si32 i)
{
	x -= i;
	y -= i;
	z -= i;
	return *this;
}

inline bool int3::operator==(const int3 & i) const { return (x == i.x && y == i.y && z == i.z); }
inline bool int3::operator!=(const int3 & i) const { return (x != i.x || y != i.y || z != i.z); }

inline bool int3::operator<(const int3 & i) const
{
	if (z < i.z)
		return true;
	if (z > i.z)
		return false;
	if (y < i.y)
		return true;
	if (y > i.y)
		return false;
	if (x < i.x)
		return true;
	if (x > i.x)
		return false;
	return false;
}

inline si32 int3::dist2dSQ(const int3 &o) const
{
	const si32 dx = (x - o.x);
	const si32 dy = (y - o.y);
	return dx*dx + dy*dy;
}
inline double int3::dist2d(const int3 &o) const
{
	return std::sqrt((double)dist2dSQ(o));
}

inline bool int3::areNeighbours(const int3 &o) const
{
	return (dist2dSQ(o) < 4) && (z == o.z);
}

inline std::string int3::operator ()() const //Change to int3::toString()?
{
	std::string result("(");
	result += boost::lexical_cast<std::string>(x); result += ' ';
	result += boost::lexical_cast<std::string>(y); result += ' ';
	result += boost::lexical_cast<std::string>(z); result += ')';
	return result;
}

template <typename Handler>
inline void int3::serialize(Handler &h, const int version) { h & x & y & z; }

inline bool int3::valid() const //Change name to isValid?
{
	return z >= 0; //minimal condition that needs to be fulfilled for tiles in the map
}

inline std::istream & operator>>(std::istream & str, int3 & dest)
{
	return str >> dest.x >> dest.y >> dest.z;
}
inline std::ostream & operator<<(std::ostream & str, const int3 & sth)
{
	return str << sth.x << ' ' << sth.y << ' ' << sth.z;
}

//Change to normal function?
struct ShashInt3
{
	size_t operator()(int3 const& pos) const
	{
		size_t ret = std::hash<int>()(pos.x);
		vstd::hash_combine(ret, pos.y);
		vstd::hash_combine(ret, pos.z);
		return ret;
	}
};