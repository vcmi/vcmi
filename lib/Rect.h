/*
 * Geometries.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "Point.h"

VCMI_LIB_NAMESPACE_BEGIN

/// Rectangle class, which have a position and a size
class Rect
{
public:
	int x;
	int y;
	int w;
	int h;

	Rect()
	{
		x = y = w = h = -1;
	}
	Rect(int X, int Y, int W, int H)
	{
		x = X;
		y = Y;
		w = W;
		h = H;
	}
	Rect(const Point & position, const Point & size)
	{
		x = position.x;
		y = position.y;
		w = size.x;
		h = size.y;
	}
	Rect(const Rect& r) = default;

	DLL_LINKAGE static Rect createCentered( const Point & around, const Point & size );
	DLL_LINKAGE static Rect createCentered( const Rect  & target, const Point & size );
	DLL_LINKAGE static Rect createAround(const Rect &r, int borderWidth);

	bool isInside(int qx, int qy) const
	{
		if (qx > x && qx<x+w && qy>y && qy<y+h)
			return true;
		return false;
	}
	bool isInside(const Point & q) const
	{
		return isInside(q.x,q.y);
	}
	int top() const
	{
		return y;
	}
	int bottom() const
	{
		return y+h;
	}
	int left() const
	{
		return x;
	}
	int right() const
	{
		return x+w;
	}

	Point topLeft() const
	{
		return Point(x,y);
	}
	Point topRight() const
	{
		return Point(x+w,y);
	}
	Point bottomLeft() const
	{
		return Point(x,y+h);
	}
	Point bottomRight() const
	{
		return Point(x+w,y+h);
	}
	Point center() const
	{
		return Point(x+w/2,y+h/2);
	}
	Point dimensions() const
	{
		return Point(w,h);
	}

	void moveTo(const Point & dest)
	{
		x = dest.x;
		y = dest.y;
	}

	Rect operator+(const Point &p) const
	{
		return Rect(x+p.x,y+p.y,w,h);
	}

	Rect& operator=(const Rect &p)
	{
		x = p.x;
		y = p.y;
		w = p.w;
		h = p.h;
		return *this;
	}

	Rect& operator+=(const Point &p)
	{
		x += p.x;
		y += p.y;
		return *this;
	}

	Rect& operator-=(const Point &p)
	{
		x -= p.x;
		y -= p.y;
		return *this;
	}

	/// returns true if this rect intersects with another rect
	DLL_LINKAGE bool intersectionTest(const Rect & other) const;

	/// returns true if this rect intersects with line specified by two points
	DLL_LINKAGE bool intersectionTest(const Point & line1, const Point & line2) const;

	/// Returns rect that represents intersection of two rects
	DLL_LINKAGE Rect intersect(const Rect & other) const;

	/// Returns rect union - rect that covers both this rect and provided rect
	DLL_LINKAGE Rect include(const Rect & other) const;

	template <typename Handler>
	void serialize(Handler &h, const int version)
	{
		h & x;
		h & y;
	}
};

VCMI_LIB_NAMESPACE_END
