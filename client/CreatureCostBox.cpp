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
#include "widgets/Images.h"
#include "widgets/TextControls.h"
#include "gui/CGuiHandler.h"

CreatureCostBox::CreatureCostBox(Rect position, std::string titleText)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	type |= REDRAW_PARENT;
	pos = position + pos;

	title = std::make_shared<CLabel>(pos.w/2, 10, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, titleText);
}

void CreatureCostBox::set(TResources res)
{
	for(auto & item : resources)
		item.second.first->setText(boost::lexical_cast<std::string>(res[item.first]));
}

void CreatureCostBox::createItems(TResources res)
{
	resources.clear();

	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	TResources::nziterator iter(res);
	while(iter.valid())
	{
		ImagePtr image = std::make_shared<CAnimImage>("RESOURCE", iter->resType);
		LabelPtr text = std::make_shared<CLabel>(15, 43, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, "0");

		resources.insert(std::make_pair(iter->resType, std::make_pair(text, image)));
		iter++;
	}

	if(!resources.empty())
	{
		int curx = pos.w / 2 - (16 * (int)resources.size()) - (8 * ((int)resources.size() - 1));
		//reverse to display gold as first resource
		for(auto & currentRes : boost::adaptors::reverse(resources))
		{
			currentRes.second.first->moveBy(Point(curx, 22));
			currentRes.second.second->moveBy(Point(curx, 22));
			curx += 48;
		}
	}
	redraw();
}
