/*
* ObjectDistributor.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#include "../Zone.h"
#include "../RmgObject.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGObjectInstance;
class ObjectTemplate;

class ObjectDistributor : public Modificator
{
	void distributeLimitedObjects();
	void distributeSeerHuts();
	void distributePrisons();

public:
	MODIFICATOR(ObjectDistributor);

	void process() override;
	void init() override;
};

VCMI_LIB_NAMESPACE_END