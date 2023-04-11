/*
 * CreatureCostBox.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/ResourceSet.h"
#include "../gui/CIntObject.h"

class CLabel;
class CAnimImage;

class CreatureCostBox : public CIntObject
{
public:
	void set(TResources res);
	CreatureCostBox(Rect position, std::string titleText);
	void createItems(TResources res);
private:
	using LabelPtr = std::shared_ptr<CLabel>;
	using ImagePtr = std::shared_ptr<CAnimImage>;

	LabelPtr title;
	std::map<GameResID, std::pair<LabelPtr, ImagePtr>> resources;
};
