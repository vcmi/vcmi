/*
 * CompoundMapObjectID.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN

struct DLL_LINKAGE CompoundMapObjectID
{
	si32 primaryID;
	si32 secondaryID;

	CompoundMapObjectID() : primaryID(0), secondaryID(0) {}
	CompoundMapObjectID(si32 primID, si32 secID) : primaryID(primID), secondaryID(secID) {};

	bool operator<(const CompoundMapObjectID& other) const
	{
		if(this->primaryID != other.primaryID)
			return this->primaryID < other.primaryID;
		else
			return this->secondaryID < other.secondaryID;
	}

	bool operator==(const CompoundMapObjectID& other) const
	{
		return (this->primaryID == other.primaryID) && (this->secondaryID == other.secondaryID);
	}
};

VCMI_LIB_NAMESPACE_END