/*
 * FactionService.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

class FactionID;
class Faction;

class DLL_LINKAGE FactionService
{
public:
	virtual ~FactionService() = default;
	virtual const Faction * getFaction(const FactionID & factionID) const = 0;
};
