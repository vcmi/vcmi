#pragma once

#include <SDL_video.h>
#include "SPoint.h"

/*
 * SRect.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

/// Rectangle class, which have a position and a size
struct SRect : public SDL_Rect
{
	SRect()//default c-tor
	{
		x = y = w = h = -1;
	}
	SRect(int X, int Y, int W, int H) //c-tor
	{
		x = X;
		y = Y;
		w = W;
		h = H;
	}
	SRect(const SPoint & position, const SPoint & size) //c-tor
	{
		x = position.x;
		y = position.y;
		w = size.x;
		h = size.y;
	}
	SRect(const SDL_Rect & r) //c-tor
	{
		x = r.x;
		y = r.y;
		w = r.w;
		h = r.h;
	}
	explicit SRect(const SDL_Surface * const &surf)
	{
		x = y = 0;
		w = surf->w;
		h = surf->h;
	}

	SRect centerIn(const SRect &r);
	static SRect createCentered(int w, int h);
	static SRect around(const SRect &r, int width = 1); //creates rect around another

	bool isIn(int qx, int qy) const //determines if given point lies inside rect
	{
		if (qx > x   &&   qx<x+w   &&   qy>y   &&   qy<y+h)
			return true;
		return false;
	}
	bool isIn(const SPoint & q) const //determines if given point lies inside rect
	{
		return isIn(q.x,q.y);
	}
	SPoint topLeft() const //top left corner of this rect
	{
		return SPoint(x,y);
	}
	SPoint topRight() const //top right corner of this rect
	{
		return SPoint(x+w,y);
	}
	SPoint bottomLeft() const //bottom left corner of this rect
	{
		return SPoint(x,y+h);
	}
	SPoint bottomRight() const //bottom right corner of this rect
	{
		return SPoint(x+w,y+h);
	}
	SRect operator+(const SRect &p) const //moves this rect by p's rect position
	{
		return SRect(x+p.x,y+p.y,w,h);
	}
	SRect operator+(const SPoint &p) const //moves this rect by p's point position
	{
		return SRect(x+p.x,y+p.y,w,h);
	}
	SRect& operator=(const SPoint &p) //assignment operator
	{
		x = p.x;
		y = p.y;
		return *this;
	}
	SRect& operator=(const SRect &p) //assignment operator
	{
		x = p.x;
		y = p.y;
		w = p.w;
		h = p.h;
		return *this;
	}
	SRect& operator+=(const SRect &p) //works as operator+
	{
		x += p.x;
		y += p.y;
		return *this;
	}
	SRect& operator+=(const SPoint &p) //works as operator+
	{
		x += p.x;
		y += p.y;
		return *this;
	}
	SRect& operator-=(const SRect &p) //works as operator+
	{
		x -= p.x;
		y -= p.y;
		return *this;
	}
	SRect& operator-=(const SPoint &p) //works as operator+
	{
		x -= p.x;
		y -= p.y;
		return *this;
	}
	template<typename T> SRect operator-(const T &t)
	{
		return SRect(x - t.x, y - t.y, w, h);
	}
	SRect operator&(const SRect &p) const //rect intersection
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
			SRect ret;
			ret.x = std::max(this->x, p.x);
			ret.y = std::max(this->y, p.y);
			SPoint bR; //bottomRight point of returned rect
			bR.x = std::min(this->w+this->x, p.w+p.x);
			bR.y = std::min(this->h+this->y, p.h+p.y);
			ret.w = bR.x - ret.x;
			ret.h = bR.y - ret.y;
			return ret;
		}
		else
		{
			return SRect();
		}
	}
	SRect operator|(const SRect &p) const //union of two rects
	{
		SRect ret;
		ret.x =  std::min(p.x, this->x);
		ret.y =  std::min(p.y, this->y);
		int x2 = std::max(p.x+p.w, this->x+this->w);
		int y2 = std::max(p.y+p.h, this->y+this->h);
		ret.w = x2 -ret.x;
		ret.h = y2 -ret.y;
		return ret;
	}
};