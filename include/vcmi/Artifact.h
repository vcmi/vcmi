/*
 * Artifact.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "Entity.h"

class ArtifactID;
class CreatureID;

class DLL_LINKAGE Artifact : public EntityWithBonuses<ArtifactID>
{
public:
	virtual bool isBig() const = 0;
	virtual bool isTradable() const = 0;
	virtual const std::string & getDescription() const = 0;
	virtual const std::string & getEventText() const = 0;
	virtual uint32_t getPrice() const = 0;
	virtual CreatureID getWarMachine() const = 0;
};
