#include "StdInc.h"
#include "Basic.h"

namespace Gfx
{

void Rect::addOffs_copySize(const Rect & r)
{
	x += r.x;
	y += r.y;
	w = r.w;
	h = r.h;
}


Rect Rect::operator&(const Rect &p) const //rect intersection
{
	bool intersect = true;

	if (
			leftX() > p.rightX() //rect p is on the left hand side of this
		||	rightX() < p.leftX() //rect p is on the right hand side of this
		||	topY() > p.bottomY() //rect p is above *this
		||	bottomY() < p.topY() //rect p is below *this
		)
	{
		return Rect();
	}
	else
	{
		Rect ret;
		ret.x = std::max(this->x, p.x);
		ret.y = std::max(this->y, p.y);

		//bottomRight point of returned rect
		Point bR(
				std::min(rightX(),  p.rightX()),
				std::min(bottomY(), p.bottomY())
			);
		ret.w = bR.x - ret.x;
		ret.h = bR.y - ret.y;
		return ret;
	}
}

}
