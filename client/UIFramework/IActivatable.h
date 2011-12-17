#pragma once

/*
 * IActivatable.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

// Defines a activate/deactive method
class IActivatable
{
public:
	virtual void activate()=0;
	virtual void deactivate()=0;
	virtual ~IActivatable(){}; //d-tor
};