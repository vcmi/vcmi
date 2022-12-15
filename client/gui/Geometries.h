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

#include <SDL_video.h>
#include "../../lib/int3.h"

enum class ETextAlignment {TOPLEFT, CENTER, BOTTOMRIGHT};

struct SDL_MouseMotionEvent;

// A point with x/y coordinate, used mostly for graphic rendering
struct Point
{
	int x, y;

	//constructors
	Point()
	{
		x = y = 0;
	};
	Point(int X, int Y)
		:x(X),y(Y)
	{};
	Point(const int3 &a)
		:x(a.x),y(a.y)
	{}
	Point(const SDL_MouseMotionEvent &a);

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
	bool operator<(const Point &b) const //product order
	{
		return x < b.x   &&   y < b.y;
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
};

/// Rectangle class, which have a position and a size
struct Rect : public SDL_Rect
{
	Rect()//default c-tor
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
	Rect(const SDL_Rect & r)
	{
		x = r.x;
		y = r.y;
		w = r.w;
		h = r.h;
	}
	Rect(const Rect& r) : Rect(static_cast<const SDL_Rect&>(r))
	{}
	explicit Rect(const SDL_Surface * const &surf)
	{
		x = y = 0;
		w = surf->w;
		h = surf->h;
	}

	Rect centerIn(const Rect &r);
	static Rect createCentered(int w, int h);
	static Rect around(const Rect &r, int width = 1); //creates rect around another

	bool isIn(int qx, int qy) const //determines if given point lies inside rect
	{
		if (qx > x   &&   qx<x+w   &&   qy>y   &&   qy<y+h)
			return true;
		return false;
	}
	bool isIn(const Point & q) const //determines if given point lies inside rect
	{
		return isIn(q.x,q.y);
	}
	Point topLeft() const //top left corner of this rect
	{
		return Point(x,y);
	}
	Point topRight() const //top right corner of this rect
	{
		return Point(x+w,y);
	}
	Point bottomLeft() const //bottom left corner of this rect
	{
		return Point(x,y+h);
	}
	Point bottomRight() const //bottom right corner of this rect
	{
		return Point(x+w,y+h);
	}
	Rect operator+(const Rect &p) const //moves this rect by p's rect position
	{
		return Rect(x+p.x,y+p.y,w,h);
	}
	Rect operator+(const Point &p) const //moves this rect by p's point position
	{
		return Rect(x+p.x,y+p.y,w,h);
	}
	Rect& operator=(const Point &p) //assignment operator
	{
		x = p.x;
		y = p.y;
		return *this;
	}
	Rect& operator=(const Rect &p) //assignment operator
	{
		x = p.x;
		y = p.y;
		w = p.w;
		h = p.h;
		return *this;
	}
	Rect& operator+=(const Rect &p) //works as operator+
	{
		x += p.x;
		y += p.y;
		return *this;
	}
	Rect& operator+=(const Point &p) //works as operator+
	{
		x += p.x;
		y += p.y;
		return *this;
	}
	Rect& operator-=(const Rect &p) //works as operator+
	{
		x -= p.x;
		y -= p.y;
		return *this;
	}
	Rect& operator-=(const Point &p) //works as operator+
	{
		x -= p.x;
		y -= p.y;
		return *this;
	}
	template<typename T> Rect operator-(const T &t)
	{
		return Rect(x - t.x, y - t.y, w, h);
	}
	Rect operator&(const Rect &p) const //rect intersection
	{
		bool intersect = true;

		if(p.topLeft().y < y && p.bottomLeft().y < y) //rect p is above *this
		{
			intersect = false;
		}
		else if(p.topLeft().y > y+h && p.bottomLeft().y > y+h) //rect p is below *this
		{
			intersect = false;
		}
		else if(p.topLeft().x > x+w && p.topRight().x > x+w) //rect p is on the right hand side of this
		{
			intersect = false;
		}
		else if(p.topLeft().x < x && p.topRight().x < x) //rect p is on the left hand side of this
		{
			intersect = false;
		}

		if(intersect)
		{
			Rect ret;
			ret.x = std::max(this->x, p.x);
			ret.y = std::max(this->y, p.y);
			Point bR; //bottomRight point of returned rect
			bR.x = std::min(this->w+this->x, p.w+p.x);
			bR.y = std::min(this->h+this->y, p.h+p.y);
			ret.w = bR.x - ret.x;
			ret.h = bR.y - ret.y;
			return ret;
		}
		else
		{
			return Rect();
		}
	}
	Rect operator|(const Rect &p) const //union of two rects
	{
		Rect ret;
		ret.x =  std::min(p.x, this->x);
		ret.y =  std::min(p.y, this->y);
		int x2 = std::max(p.x+p.w, this->x+this->w);
		int y2 = std::max(p.y+p.h, this->y+this->h);
		ret.w = x2 -ret.x;
		ret.h = y2 -ret.y;
		return ret;
	}
};
