/*
 * ArtifactLocation.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../ConstTransitivePtr.h"
#include "../constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;
class CStackInstance;
class CArmedInstance;
class CArtifactSet;
class CBonusSystemNode;
struct ArtSlotInfo;

using TArtHolder = std::variant<ConstTransitivePtr<CGHeroInstance>, ConstTransitivePtr<CStackInstance>>;

struct ArtifactLocation
{
	TArtHolder artHolder;//TODO: identify holder by id
	ArtifactPosition slot = ArtifactPosition::PRE_FIRST;

	ArtifactLocation()
		: artHolder(ConstTransitivePtr<CGHeroInstance>())
	{
	}
	template<typename T>
	ArtifactLocation(const T * ArtHolder, ArtifactPosition Slot)
		: artHolder(const_cast<T *>(ArtHolder)) //we are allowed here to const cast -> change will go through one of our packages... do not abuse!
		, slot(Slot)
	{
	}
	ArtifactLocation(TArtHolder ArtHolder, const ArtifactPosition & Slot)
		: artHolder(std::move(std::move(ArtHolder)))
		, slot(Slot)
	{
	}

	template <typename T>
	bool isHolder(const T *t) const
	{
		if(auto ptrToT = std::get<ConstTransitivePtr<T>>(artHolder))
		{
			return ptrToT == t;
		}
		return false;
	}

	DLL_LINKAGE void removeArtifact(); // BE CAREFUL, this operation modifies holder (gs)

	DLL_LINKAGE const CArmedInstance *relatedObj() const; //hero or the stack owner
	DLL_LINKAGE PlayerColor owningPlayer() const;
	DLL_LINKAGE CArtifactSet *getHolderArtSet();
	DLL_LINKAGE CBonusSystemNode *getHolderNode();
	DLL_LINKAGE CArtifactSet *getHolderArtSet() const;
	DLL_LINKAGE const CBonusSystemNode *getHolderNode() const;

	DLL_LINKAGE const CArtifactInstance *getArt() const;
	DLL_LINKAGE CArtifactInstance *getArt();
	DLL_LINKAGE const ArtSlotInfo *getSlot() const;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & artHolder;
		h & slot;
	}
};

VCMI_LIB_NAMESPACE_END
