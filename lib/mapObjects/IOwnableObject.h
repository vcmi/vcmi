/*
* IOwnableObject.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class ResourceSet;
class CreatureID;

class DLL_LINKAGE IOwnableObject
{
public:
	/// Fixed daily income of this object
	/// May not include random or periodical (e.g. weekly) income sources
	virtual ResourceSet dailyIncome() const = 0;

	/// List of creatures that are provided by this building
	/// For use in town dwellings growth bonus and for portal of summoning
	virtual std::vector<CreatureID> providedCreatures() const = 0;

	virtual ~IOwnableObject() = default;
};

VCMI_LIB_NAMESPACE_END
