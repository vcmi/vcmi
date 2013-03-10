#pragma once

namespace Gfx
{

struct Point
{
	si32 x;
	si32 y;

	inline Point() {};
	inline Point(si32 _x, si32 _y) : x(_x), y(_y) {};
};

struct Rect
{
	Point lt;
	Point rb;

	inline ui32 width() { return rb.x - lt.x; };
	inline ui32 height() { return rb.y - lt.y; };
};

/* Color transform matrix for: grayscale, clone, bloodlust, etc */
typedef float ColorMatrix[3][3];

}
