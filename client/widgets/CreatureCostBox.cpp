/*
 * CreatureCostBox.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CreatureCostBox.h"

#include "../widgets/Images.h"
#include "../widgets/TextControls.h"
#include "../GameEngine.h"

CreatureCostBox::CreatureCostBox(Rect position, std::string titleText)
{
	OBJECT_CONSTRUCTION;

	setRedrawParent(true);
	pos = position + pos.topLeft();

	title = std::make_shared<CLabel>(pos.w/2, 10, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, titleText);
}

void CreatureCostBox::set(TResources res)
{
	for(auto & item : resources)
		item.second.first->setText(std::to_string(res[item.first]));
}

void CreatureCostBox::createItems(TResources res)
{
	resources.clear();

	OBJECT_CONSTRUCTION;

	TResources::nziterator iter(res);
	while(iter.valid())
	{
		auto image = std::make_shared<CAnimImage>(AnimationPath::builtin("RESOURCE"), iter->resType.getNum());
		auto text = std::make_shared<CLabel>(15, 43, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, "0");

		resources.insert(std::make_pair(iter->resType, std::make_pair(text, image)));
		iter++;
	}

	if(!resources.empty())
	{
		int curx = pos.w / 2;
		int spacing = 48;
		int resourcesCount = static_cast<int>(resources.size());
		if (resources.size() > 2)
		{
			spacing = 32;
			curx -= (15 + 16 * (resourcesCount - 1));
		}
		else
		{
			curx -= ((16 * resourcesCount) + (8 * (resourcesCount - 1)));
		}
		//reverse to display gold as first resource
		for(auto & currentRes : boost::adaptors::reverse(resources))
		{
			currentRes.second.first->moveBy(Point(curx + 2, 22));
			currentRes.second.second->moveBy(Point(curx, 22));
			curx += spacing;
		}
	}
	redraw();
}
