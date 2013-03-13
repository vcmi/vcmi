#pragma once

namespace Gfx
{

struct Point
{
	si32 x;
	si32 y;

	Point() {};
	Point(si32 _x, si32 _y) : x(_x), y(_y) {};

	bool operator==(const Point &p) const
	{
		return (x == p.x) && (y == p.y);
	}
	bool operator!=(const Point &p) const
	{
		return !(*this == p);
	}
	Point& operator+=(const Point &p)
	{
		x += p.x;
		y += p.y;
		return *this;
	}
	Point& operator-=(const Point &p)
	{
		x -= p.x;
		y -= p.y;
		return *this;
	}
};

struct Rect : Point
{
	ui32 w;
	ui32 h;

	Rect() : w(0), h(0) {};
	Rect(const Point & lt) : Point(lt), w(0), h(0) {};
	Rect(const Point & lt, const Point & sz) : Point(lt), w(sz.x), h(sz.y) {};
	Rect(si32 _x, si32 _y, ui32 _w, ui32 _h) : Point(_x, _y), w(_w), h(_h) {};

	Rect& operator=(const Point &p)
	{
		x = p.x;
		y = p.y;
		return *this;
	}

	si32 leftX()	const { return x;     };
	si32 centerX()	const { return x+w/2; };
	si32 rightX()	const { return x+w;   };
	si32 topY() 	const { return y;     };
	si32 centerY()	const { return y+h/2; }
	si32 bottomY()	const { return y+h;   };

	//top left corner of this rect
	Point topLeft() const {	return *this; }

	//top right corner of this rect
	Point topRight() const { return Point(x+w, y); }

	//bottom left corner of this rect
	Point bottomLeft() const { return Point(x, y+h); }

	//bottom right corner of this rect
	Point bottomRight() const { return Point(x+w, y+h); }

	void addOffs_copySize(const Rect &p);

	Rect Rect::operator&(const Rect &p) const; //rect intersection
};

/* Color transform matrix for: grayscale, clone, bloodlust, etc */
typedef float ColorMatrix[3][3];

}
