/*
 * ResourceSet.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "Rect.h"

VCMI_LIB_NAMESPACE_BEGIN

/// Returns rect union - rect that covers both this rect and provided rect
Rect Rect::include(const Rect & other) const
{
	Point topLeft{
		std::min(this->left(), other.left()),
		std::min(this->top(), other.top())
	};

	Point bottomRight{
		std::max(this->right(), other.right()),
		std::max(this->bottom(), other.bottom())
	};

	return Rect(topLeft, bottomRight - topLeft);
}

Rect Rect::createCentered( const Point & around, const Point & dimensions )
{
	return Rect(around - dimensions/2, dimensions);
}

Rect Rect::createAround(const Rect &r, int width)
{
	return Rect(r.x - width, r.y - width, r.w + width * 2, r.h + width * 2);
}

Rect Rect::createCentered( const Rect & rect, const Point & dimensions)
{
	return createCentered(rect.center(), dimensions);
}

bool Rect::intersectionTest(const Rect & other) const
{
	// this rect is above other rect
	if(this->bottom() < other.top())
		return false;

	// this rect is below other rect
	if(this->top() > other.bottom() )
		return false;

	// this rect is to the left of other rect
	if(this->right() < other.left())
		return false;

	// this rect is to the right of other rect
	if(this->left() > other.right())
		return false;

	return true;
}

/// Algorithm to test whether line segment between points line1-line2 will intersect with
/// rectangle specified by top-left and bottom-right points
/// Note that in order to avoid floating point rounding errors algorithm uses integers with no divisions
bool Rect::intersectionTest(const Point & line1, const Point & line2) const
{
	// check whether segment is located to the left of our rect
	if (line1.x < left() && line2.x < left())
		return false;

	// check whether segment is located to the right of our rect
	if (line1.x > right() && line2.x > right())
		return false;

	// check whether segment is located on top of our rect
	if (line1.y < top() && line2.y < top())
		return false;

	// check whether segment is located below of our rect
	if (line1.y > bottom() && line2.y > bottom())
		return false;

	Point vector { line2.x - line1.x, line2.y - line1.y};

	// compute position of corners relative to our line
	int tlTest = vector.y*topLeft().x - vector.x*topLeft().y + (line2.x*line1.y-line1.x*line2.y);
	int trTest = vector.y*bottomRight().x - vector.x*topLeft().y + (line2.x*line1.y-line1.x*line2.y);
	int blTest = vector.y*topLeft().x - vector.x*bottomRight().y + (line2.x*line1.y-line1.x*line2.y);
	int brTest = vector.y*bottomRight().x - vector.x*bottomRight().y + (line2.x*line1.y-line1.x*line2.y);

	// if all points are on the left of our line then there is no intersection
	if ( tlTest > 0 && trTest > 0 && blTest > 0 && brTest > 0 )
		return false;

	// if all points are on the right of our line then there is no intersection
	if ( tlTest < 0 && trTest < 0 && blTest < 0 && brTest < 0 )
		return false;

	// if all previous checks failed, this means that there is an intersection between line and AABB
	return true;
}


Rect Rect::intersect(const Rect & other) const
{
	if(intersectionTest(other))
	{
		Point topLeft{
			std::max(this->left(), other.left()),
			std::max(this->top(), other.top())
		};

		Point bottomRight{
			std::min(this->right(), other.right()),
			std::min(this->bottom(), other.bottom())
		};

		return Rect(topLeft, bottomRight - topLeft);
	}
	else
	{
		return Rect();
	}
}

VCMI_LIB_NAMESPACE_END
