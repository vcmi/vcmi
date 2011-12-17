#pragma once

#include "../../lib/int3.h"
#include <SDL_events.h>

/*
 * SPoint.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

// A point with x/y coordinate, used mostly for graphic rendering
struct SPoint
{
	int x, y;

	//constructors
	SPoint()
	{
		x = y = 0;
	};
	SPoint(int X, int Y)
		:x(X),y(Y)
	{};
	SPoint(const int3 &a)
		:x(a.x),y(a.y)
	{}
	SPoint(const SDL_MouseMotionEvent &a)
		:x(a.x),y(a.y)
	{}

	template<typename T>
	SPoint operator+(const T &b) const
	{
		return SPoint(x+b.x,y+b.y);
	}

	template<typename T>
	SPoint operator*(const T &mul) const
	{
		return SPoint(x*mul, y*mul);
	}

	template<typename T>
	SPoint& operator+=(const T &b)
	{
		x += b.x;
		y += b.y;
		return *this;
	}

	template<typename T>
	SPoint operator-(const T &b) const
	{
		return SPoint(x - b.x, y - b.y);
	}

	template<typename T>
	SPoint& operator-=(const T &b)
	{
		x -= b.x;
		y -= b.y;
		return *this;
	}
	bool operator<(const SPoint &b) const //product order
	{
		return x < b.x   &&   y < b.y;
	}
	template<typename T> SPoint& operator=(const T &t)
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
};