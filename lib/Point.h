/*
 * Point.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN
class int3;

// A point with x/y coordinate, used mostly for graphic rendering
class Point
{
public:
	int x, y;

	//constructors
	constexpr Point() : x(0), y(0)
	{
	}

	constexpr Point(int X, int Y)
		: x(X)
		, y(Y)
	{
	}

	constexpr static Point makeInvalid()
	{
		return Point(std::numeric_limits<int>::min(), std::numeric_limits<int>::min());
	}

	explicit DLL_LINKAGE Point(const int3 &a);

	template<typename T>
	constexpr Point operator+(const T &b) const
	{
		return Point(x+b.x,y+b.y);
	}

	template<typename T>
	constexpr Point operator/(const T &div) const
	{
		return Point(x/div, y/div);
	}

	template<typename T>
	constexpr Point operator*(const T &mul) const
	{
		return Point(x*mul, y*mul);
	}

	constexpr Point operator*(const Point &b) const
	{
		return Point(x*b.x,y*b.y);
	}

	template<typename T>
	constexpr Point& operator+=(const T &b)
	{
		x += b.x;
		y += b.y;
		return *this;
	}

	constexpr Point operator-() const
	{
		return Point(-x, -y);
	}

	template<typename T>
	constexpr Point operator-(const T &b) const
	{
		return Point(x - b.x, y - b.y);
	}

	template<typename T>
	constexpr Point& operator-=(const T &b)
	{
		x -= b.x;
		y -= b.y;
		return *this;
	}

	template<typename T> constexpr Point& operator=(const T &t)
	{
		x = t.x;
		y = t.y;
		return *this;
	}
	template<typename T> constexpr bool operator==(const T &t) const
	{
		return x == t.x  &&  y == t.y;
	}
	template<typename T> constexpr bool operator!=(const T &t) const
	{
		return !(*this == t);
	}

	constexpr bool isValid() const
	{
		return x > std::numeric_limits<int>::min() && y > std::numeric_limits<int>::min();
	}

	constexpr int lengthSquared() const
	{
		return x * x + y * y;
	}

	int length() const
	{
		return std::sqrt(lengthSquared());
	}

	double angle() const
	{
		return std::atan2(y, x) * 180.0 / M_PI;
	}

	template <typename Handler>
	void serialize(Handler &h, const int version)
	{
		h & x;
		h & y;
	}
};

VCMI_LIB_NAMESPACE_END
