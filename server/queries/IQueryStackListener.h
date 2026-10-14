/*
 * IQueryStackListener.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/constants/EntityIdentifiers.h"

class IQueryStackListener
{
public:
	virtual ~IQueryStackListener() = default;

	// Called after a player's query stack changes and local query lifecycle
	// handling for that operation is complete.
	virtual void onQueryStackChanged(PlayerColor player) = 0;
};
