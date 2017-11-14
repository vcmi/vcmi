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

#include "widgets/Images.h"
#include "../lib/ResourceSet.h"

class CreatureCostBox : public CIntObject
{
public:
	void set(TResources res);
	CreatureCostBox(Rect position, std::string title);
	void createItems(TResources res);
private:
	std::map<int, std::pair<CLabel *, CAnimImage * > > resources;
};
