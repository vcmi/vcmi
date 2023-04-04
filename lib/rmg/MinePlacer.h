/*
* MinePlacer.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once
#include "Zone.h"

VCMI_LIB_NAMESPACE_BEGIN

class ObjectManager;

class MinePlacer: public Modificator
{
public:
	MODIFICATOR(MinePlacer);

	void process() override;
	void init() override;

protected:
	bool placeMines(ObjectManager & manager);

};

VCMI_LIB_NAMESPACE_END
