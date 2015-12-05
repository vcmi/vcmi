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

	//c-tor: x, y, z initialized to 0
	int3() : x(0), y(0), z(0) {} // I think that x, y, z should be left uninitialized.
	//c-tor: x, y, z initialized to i
	explicit int3(const si32 i) : x(i), y(i), z(i) {}
	//c-tor: x, y, z initialized to X, Y, Z
	int3(const si32 X, const si32 Y, const si32 Z) : x(X), y(Y), z(Z) {}
	int3(const int3 & c) : x(c.x), y(c.y), z(c.z) {} // Should be set to default (C++11)?

	int3 & operator=(const int3 & c) // Should be set to default (C++11)?
	{
		x = c.x;
		y = c.y;
		z = c.z;

		return *this;
	}
	int3 operator-() const { return int3(-x, -y, -z); }

	int3 operator+(const int3 & i) const { return int3(x + i.x, y + i.y, z + i.z); }
	int3 operator-(const int3 & i) const { return int3(x - i.x, y - i.y, z - i.z); }
	//returns int3 with coordinates increased by given number
	int3 operator+(const si32 i) const { return int3(x + i, y + i, z + i); }
	//returns int3 with coordinates decreased by given number
	int3 operator-(const si32 i) const { return int3(x - i, y - i, z - i); }

	int3 & operator+=(const int3 & i)
	{
		x += i.x;
		y += i.y;
		z += i.z;
		return *this;
	}
	int3 & operator-=(const int3 & i)
	{
		x -= i.x;
		y -= i.y;
		z -= i.z;
		return *this;
	}
	
	//increases all coordinates by given number
	int3 & operator+=(const si32 i)
	{
		x += i;
		y += i;
		z += i;
		return *this;
	}
	//decreases all coordinates by given number
	int3 & operator-=(const si32 i)
	{
		x -= i;
		y -= i;
		z -= i;
		return *this;
	}

	bool operator==(const int3 & i) const { return (x == i.x && y == i.y && z == i.z); }
	bool operator!=(const int3 & i) const { return (x != i.x || y != i.y || z != i.z); }

	bool operator<(const int3 & i) const
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

	//returns squared distance on Oxy plane (z coord is not used)
	ui32 dist2dSQ(const int3 & o) const
	{
		const si32 dx = (x - o.x);
		const si32 dy = (y - o.y);
		return (ui32)(dx*dx) + (ui32)(dy*dy);
	}
	//returns distance on Oxy plane (z coord is not used)
	double dist2d(const int3 & o) const
	{
		return std::sqrt((double)dist2dSQ(o));
	}
	//manhattan distance used for patrol radius (z coord is not used)
	double mandist2d(const int3 & o) const
	{
		return abs(o.x - x) + abs(o.y - y);
	}

	bool areNeighbours(const int3 & o) const
	{
		return (dist2dSQ(o) < 4) && (z == o.z);
	}

	//returns "(x y z)" string
	std::string operator ()() const //Change to int3::toString()?
	{
		std::string result("(");
		result += boost::lexical_cast<std::string>(x); result += ' ';
		result += boost::lexical_cast<std::string>(y); result += ' ';
		result += boost::lexical_cast<std::string>(z); result += ')';
		return result;
	}

	bool valid() const //Should be named "isValid"?
	{
		return z >= 0; //minimal condition that needs to be fulfilled for tiles in the map
	}

	template <typename Handler>
	void serialize(Handler &h, const int version)
	{
		h & x & y & z;
	}

	static std::array<int3, 8> getDirs()
	{
		return { { int3(0,1,0),int3(0,-1,0),int3(-1,0,0),int3(+1,0,0),
			int3(1,1,0),int3(-1,1,0),int3(1,-1,0),int3(-1,-1,0) } };
	}
};

inline std::ostream & operator<<(std::ostream & str, const int3 & sth)
{
	return str << sth.x << ' ' << sth.y << ' ' << sth.z;
}
inline std::istream & operator>>(std::istream & str, int3 & dest)
{
	return str >> dest.x >> dest.y >> dest.z;
}

//Why not normal function?
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

template<typename Container>
int3 findClosestTile (Container & container, int3 dest)
{
	static_assert(std::is_same<typename Container::value_type, int3>::value,
		"findClosestTile requires <int3> container.");

	int3 result(-1, -1, -1);
	ui32 distance = std::numeric_limits<ui32>::max();
	for (const int3& tile : container)
	{
		const ui32 currentDistance = dest.dist2dSQ(tile);
		if (currentDistance < distance)
		{
			result = tile;
			distance = currentDistance;
		}
	}
	return result;
}
