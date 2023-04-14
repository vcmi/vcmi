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

VCMI_LIB_NAMESPACE_BEGIN

class ArtifactID;
class CreatureID;

class DLL_LINKAGE Artifact : public EntityWithBonuses<ArtifactID>
{
public:

	virtual bool isBig() const = 0;
	virtual bool isTradable() const = 0;
	virtual uint32_t getPrice() const = 0;
	virtual CreatureID getWarMachine() const = 0;

	virtual std::string getDescriptionTranslated() const = 0;
	virtual std::string getEventTranslated() const = 0;

	virtual std::string getDescriptionTextID() const = 0;
	virtual std::string getEventTextID() const = 0;
};

VCMI_LIB_NAMESPACE_END
