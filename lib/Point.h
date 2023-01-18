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
	Point()
	{
		x = y = 0;
	};

	Point(int X, int Y)
		:x(X),y(Y)
	{};

	Point(const int3 &a);

	template<typename T>
	Point operator+(const T &b) const
	{
		return Point(x+b.x,y+b.y);
	}

	template<typename T>
	Point operator/(const T &div) const
	{
		return Point(x/div, y/div);
	}

	template<typename T>
	Point operator*(const T &mul) const
	{
		return Point(x*mul, y*mul);
	}

	template<typename T>
	Point& operator+=(const T &b)
	{
		x += b.x;
		y += b.y;
		return *this;
	}

	template<typename T>
	Point operator-(const T &b) const
	{
		return Point(x - b.x, y - b.y);
	}

	template<typename T>
	Point& operator-=(const T &b)
	{
		x -= b.x;
		y -= b.y;
		return *this;
	}

	template<typename T> Point& operator=(const T &t)
	{
		x = t.x;
		y = t.y;
		return *this;
	}
	template<typename T> bool operator==(const T &t) const
	{
		return x == t.x  &&  y == t.y;
	}
	template<typename T> bool operator!=(const T &t) const
	{
		return !(*this == t);
	}

	template <typename Handler>
	void serialize(Handler &h, const int version)
	{
		h & x;
		h & y;
	}
};

VCMI_LIB_NAMESPACE_END
