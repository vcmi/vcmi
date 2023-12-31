/*
 * int3.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

/// Class which consists of three integer values. Represents position on adventure map.
class int3
{
public:
	si32 x, y, z;

	//c-tor: x, y, z initialized to 0
	constexpr int3() : x(0), y(0), z(0) {} // I think that x, y, z should be left uninitialized.
	//c-tor: x, y, z initialized to i
	explicit constexpr int3(const si32 i) : x(i), y(i), z(i) {}
	//c-tor: x, y, z initialized to X, Y, Z
	constexpr int3(const si32 X, const si32 Y, const si32 Z) : x(X), y(Y), z(Z) {}
	constexpr int3(const int3 & c) = default;

	constexpr int3 & operator=(const int3 & c) = default;
	constexpr int3 operator-() const { return int3(-x, -y, -z); }

	constexpr int3 operator+(const int3 & i) const { return int3(x + i.x, y + i.y, z + i.z); }
	constexpr int3 operator-(const int3 & i) const { return int3(x - i.x, y - i.y, z - i.z); }
	//returns int3 with coordinates increased by given number
	constexpr int3 operator+(const si32 i) const { return int3(x + i, y + i, z + i); }
	//returns int3 with coordinates decreased by given number
	constexpr int3 operator-(const si32 i) const { return int3(x - i, y - i, z - i); }

	//returns int3 with coordinates multiplied by given number
	constexpr int3 operator*(const double i) const { return int3((int)(x * i), (int)(y * i), (int)(z * i)); }
	//returns int3 with coordinates divided by given number
	constexpr int3 operator/(const double i) const { return int3((int)(x / i), (int)(y / i), (int)(z / i)); }

	//returns int3 with coordinates multiplied by given number
	constexpr int3 operator*(const si32 i) const { return int3(x * i, y * i, z * i); }
	//returns int3 with coordinates divided by given number
	constexpr int3 operator/(const si32 i) const { return int3(x / i, y / i, z / i); }

	constexpr int3 & operator+=(const int3 & i)
	{
		x += i.x;
		y += i.y;
		z += i.z;
		return *this;
	}
	constexpr int3 & operator-=(const int3 & i)
	{
		x -= i.x;
		y -= i.y;
		z -= i.z;
		return *this;
	}

	//increases all coordinates by given number
	constexpr int3 & operator+=(const si32 i)
	{
		x += i;
		y += i;
		z += i;
		return *this;
	}
	//decreases all coordinates by given number
	constexpr int3 & operator-=(const si32 i)
	{
		x -= i;
		y -= i;
		z -= i;
		return *this;
	}

	constexpr bool operator==(const int3 & i) const { return (x == i.x && y == i.y && z == i.z); }
	constexpr bool operator!=(const int3 & i) const { return (x != i.x || y != i.y || z != i.z); }

	constexpr bool operator<(const int3 & i) const
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

	enum EDistanceFormula
	{
		DIST_2D = 0,
		DIST_MANHATTAN, // patrol distance
		DIST_CHEBYSHEV, // ambient sound distance
		DIST_2DSQ
	};

	ui32 dist(const int3 & o, EDistanceFormula formula) const
	{
		switch(formula)
		{
		case DIST_2D:
			return std::round(dist2d(o));
		case DIST_MANHATTAN:
			return static_cast<ui32>(mandist2d(o));
		case DIST_CHEBYSHEV:
			return static_cast<ui32>(chebdist2d(o));
		case DIST_2DSQ:
			return dist2dSQ(o);
		default:
			return 0;
		}
	}

	//returns squared distance on Oxy plane (z coord is not used)
	constexpr ui32 dist2dSQ(const int3 & o) const
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
	constexpr double mandist2d(const int3 & o) const
	{
		return vstd::abs(o.x - x) + vstd::abs(o.y - y);
	}
	//chebyshev distance used for ambient sounds (z coord is not used)
	constexpr double chebdist2d(const int3 & o) const
	{
		return std::max(vstd::abs(o.x - x), vstd::abs(o.y - y));
	}

	constexpr bool areNeighbours(const int3 & o) const
	{
		return (dist2dSQ(o) < 4) && (z == o.z);
	}

	//returns "(x y z)" string
	std::string toString() const
	{
		//Performance is important here
		std::string result = "(" +
				std::to_string(x) + " " +
				std::to_string(y) + " " +
				std::to_string(z) + ")";

		return result;
	}

	constexpr bool valid() const //Should be named "isValid"?
	{
		return z >= 0; //minimal condition that needs to be fulfilled for tiles in the map
	}

	template <typename Handler>
	void serialize(Handler &h, const int version)
	{
		h & x;
		h & y;
		h & z;
	}

	constexpr static std::array<int3, 8> getDirs()
	{
		return { { int3(0,1,0),int3(0,-1,0),int3(-1,0,0),int3(+1,0,0),
			int3(1,1,0),int3(-1,1,0),int3(1,-1,0),int3(-1,-1,0) } };
	}

	// Solution by ChatGPT

	// Assume values up to +- 1000
    friend std::size_t hash_value(const int3& v) {
        // Since the range is [-1000, 1000], offsetting by 1000 maps it to [0, 2000]
        std::size_t hx = v.x + 1000;
        std::size_t hy = v.y + 1000;
        std::size_t hz = v.z + 1000;

        // Combine the hash values, multiplying them by prime numbers
        return ((hx * 4000037u) ^ (hy * 2003u)) + hz;
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

VCMI_LIB_NAMESPACE_END

template<>
struct std::hash<VCMI_LIB_WRAP_NAMESPACE(int3)> {
	std::size_t operator()(VCMI_LIB_WRAP_NAMESPACE(int3) const& pos) const noexcept {
		return hash_value(pos);
	}
};