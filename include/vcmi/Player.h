/*
 * Player.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class PlayerColor;
class TeamID;
class IBonusBearer;

class DLL_LINKAGE Player
{
public:
	virtual PlayerColor getColor() const = 0;
	virtual TeamID getTeam() const = 0;
	virtual bool isHuman() const = 0;
	virtual const IBonusBearer * accessBonuses() const = 0;
	virtual int getResourceAmount(int type) const = 0;
};

VCMI_LIB_NAMESPACE_END
