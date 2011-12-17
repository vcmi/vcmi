#pragma once

/*
 * IUpdateable.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class IUpdateable
{
public:
	virtual void update()=0;
	virtual ~IUpdateable(){}; //d-tor
};