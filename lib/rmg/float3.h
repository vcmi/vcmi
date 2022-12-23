/*
 * float3.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

// FIXME: Class doesn't contain three float values. Update name and description.
/// Class which consists of three float values. Represents position virtual RMG (0;1) area.
class float3
{
public:
	float x, y;
	si32 z;

	float3() : x(0), y(0), z(0) {}
	float3(const float X, const float Y, const si32 Z): x(X), y(Y), z(Z) {}
	float3(const float3 & copy) : x(copy.x), y(copy.y), z(copy.z) {}
	float3 & operator=(const float3 & copy) { x = copy.x; y = copy.y; z = copy.z; return *this; }

	// returns float3 with coordinates increased by corresponding coordinate of given float3
	float3 operator+(const float3 & i) const { return float3(x + i.x, y + i.y, z + i.z); }
	// returns float3 with coordinates increased by given numer
	float3 operator+(const float i) const { return float3(x + i, y + i, z + (si32)i); }
	// returns float3 with coordinates decreased by corresponding coordinate of given float3
	float3 operator-(const float3 & i) const { return float3(x - i.x, y - i.y, z - i.z); }
	// returns float3 with coordinates decreased by given numer
	float3 operator-(const float i) const { return float3(x - i, y - i, z - (si32)i); }

	// returns float3 with plane coordinates decreased by given numer
	float3 operator*(const float i) const {return float3(x * i, y * i, z);}
	// returns float3 with plane coordinates decreased by given numer
	float3 operator/(const float i) const {return float3(x / i, y / i, z);}

	// returns opposite position
	float3 operator-() const { return float3(-x, -y, -z); }

	// returns squared distance on Oxy plane (z coord is not used)
	double dist2dSQ(const float3 & o) const
	{
		const double dx = (x - o.x);
		const double dy = (y - o.y);
		return dx*dx + dy*dy;
	}
	double mag() const
	{
		return sqrt(x*x + y*y);
	}
	float3 unitVector() const
	{
		return float3(x, y, z) / (float)mag();
	}
	// returns distance on Oxy plane (z coord is not used)
	double dist2d(const float3 &other) const { return std::sqrt(dist2dSQ(other)); }

	bool areNeighbours(const float3 &other) const { return (dist2dSQ(other) < 4.0) && z == other.z; }

	float3& operator+=(const float3 & i)
	{
		x += i.x;
		y += i.y;
		z += i.z;

		return *this;
	}
	float3& operator+=(const float & i)
	{
		x += i;
		y += i;
		z += (si32)i;

		return *this;
	}

	float3& operator-=(const float3 & i)
	{
		x -= i.x;
		y -= i.y;
		z -= i.z;

		return *this;
	}
	float3& operator-=(const float & i)
	{
		x += i;
		y += i;
		z += (si32)i;

		return *this;
	}

	// scale on plane
	float3& operator*=(const float & i)
	{
		x *= i;
		y *= i;

		return *this;
	}
	// scale on plane
	float3& operator/=(const float & i)
	{
		x /= i;
		y /= i;

		return *this;
	}

	bool operator==(const float3 & i) const { return (x == i.x) && (y == i.y) && (z == i.z); }
	bool operator!=(const float3 & i) const { return (x != i.x) || (y != i.y) || (z != i.z); }

	bool operator<(const float3 & i) const
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

	std::string toString() const
	{
		return	"(" + boost::lexical_cast<std::string>(x) +
				" " + boost::lexical_cast<std::string>(y) +
				" " + boost::lexical_cast<std::string>(z) + ")";
	}

	bool valid() const
	{
		return z >= 0; //minimal condition that needs to be fulfilled for tiles in the map
	}

	template <typename Handler> void serialize(Handler &h, const float version)
	{
		h & x;
		h & y;
		h & z;
	}
};

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

VCMI_LIB_NAMESPACE_END
