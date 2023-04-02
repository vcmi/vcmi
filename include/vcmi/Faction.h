/*
 * Faction.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "Entity.h"

VCMI_LIB_NAMESPACE_BEGIN

class FactionID;
enum class ETerrainId;
template<typename T> class Identifier;

class DLL_LINKAGE Faction : public EntityT<FactionID>
{
public:
	virtual bool hasTown() const = 0;
	virtual Identifier<ETerrainId> getNativeTerrain() const = 0;
};

VCMI_LIB_NAMESPACE_END
