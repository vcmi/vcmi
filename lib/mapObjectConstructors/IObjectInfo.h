/*
 * IObjectInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE IObjectInfo
{
public:
	struct CArmyStructure
	{
		ui32 totalStrength;
		ui32 shootersStrength;
		ui32 flyersStrength;
		ui32 walkersStrength;

		CArmyStructure() :
			totalStrength(0),
			shootersStrength(0),
			flyersStrength(0),
			walkersStrength(0)
		{}

		bool operator <(const CArmyStructure & other) const
		{
			return this->totalStrength < other.totalStrength;
		}
	};

	/// Returns possible composition of guards. Actual guards would be
	/// somewhere between these two values
	virtual CArmyStructure minGuards() const { return CArmyStructure(); }
	virtual CArmyStructure maxGuards() const { return CArmyStructure(); }

	virtual bool givesResources() const { return false; }

	virtual bool givesExperience() const { return false; }
	virtual bool givesMana() const { return false; }
	virtual bool givesMovement() const { return false; }

	virtual bool givesPrimarySkills() const { return false; }
	virtual bool givesSecondarySkills() const { return false; }

	virtual bool givesArtifacts() const { return false; }
	virtual bool givesCreatures() const { return false; }
	virtual bool givesSpells() const { return false; }

	virtual bool givesBonuses() const { return false; }

	virtual ~IObjectInfo() = default;
};

VCMI_LIB_NAMESPACE_END
