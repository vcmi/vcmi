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
#include "windows/CAdvmapInterface.h"
#include "gui/CGuiHandler.h"


void CreatureCostBox::set(TResources res)
{
	for(auto & item : resources)
		item.second.first->setText(boost::lexical_cast<std::string>(res[item.first]));
}

CreatureCostBox::CreatureCostBox(Rect position, std::string title)
{
	type |= REDRAW_PARENT;
	pos = position + pos;
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	new CLabel(pos.w/2, 10, FONT_SMALL, CENTER, Colors::WHITE, title);
}

void CreatureCostBox::createItems(TResources res)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	for(auto & curr : resources)
	{
		delete curr.second.first;
		delete curr.second.second;
	}
	resources.clear();

	TResources::nziterator iter(res);
	while (iter.valid())
	{
		CAnimImage * image = new CAnimImage("RESOURCE", iter->resType);
		CLabel * text = new CLabel(15, 43, FONT_SMALL, CENTER, Colors::WHITE, "0");

		resources.insert(std::make_pair(iter->resType, std::make_pair(text, image)));
		iter++;
	}

	if (!resources.empty())
	{
		int curx = pos.w / 2 - (16 * resources.size()) - (8 * (resources.size() - 1));
		//reverse to display gold as first resource
		for (auto & res : boost::adaptors::reverse(resources))
		{
			res.second.first->moveBy(Point(curx, 22));
			res.second.second->moveBy(Point(curx, 22));
			curx += 48;
		}
	}
	redraw();
}
