/*
 * CArtifactInstance.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "bonuses/CBonusSystemNode.h"
#include "GameConstants.h"
#include "CArtHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

struct ArtifactLocation;

class DLL_LINKAGE CCombinedArtifactInstance
{
protected:
	CCombinedArtifactInstance() = default;
public:
	struct PartInfo
	{
		CArtifactInstance * art;
		ArtifactPosition slot;
		template <typename Handler> void serialize(Handler & h)
		{
			h & art;
			h & slot;
		}
		PartInfo(CArtifactInstance * art = nullptr, const ArtifactPosition & slot = ArtifactPosition::PRE_FIRST)
			: art(art), slot(slot) {};
	};
	void addPart(CArtifactInstance * art, const ArtifactPosition & slot);
	// Checks if supposed part inst is part of this combined art inst
	bool isPart(const CArtifactInstance * supposedPart) const;
	bool hasParts() const;
	const std::vector<PartInfo> & getPartsInfo() const;
	void addPlacementMap(const CArtifactSet::ArtPlacementMap & placementMap);

	template <typename Handler> void serialize(Handler & h)
	{
		h & partsInfo;
	}
protected:
	std::vector<PartInfo> partsInfo;
};

class DLL_LINKAGE CScrollArtifactInstance
{
protected:
	CScrollArtifactInstance() = default;
public:
	SpellID getScrollSpellID() const;
};

class DLL_LINKAGE CGrowingArtifactInstance
{
protected:
	CGrowingArtifactInstance() = default;
public:
	void growingUp();
};

class DLL_LINKAGE CArtifactInstance
	: public CBonusSystemNode, public CCombinedArtifactInstance, public CScrollArtifactInstance, public CGrowingArtifactInstance
{
protected:
	void init();

	ArtifactInstanceID id;
	ArtifactID artTypeID;
public:

	CArtifactInstance(const CArtifact * art);
	CArtifactInstance();
	void setType(const CArtifact * art);
	std::string nodeName() const override;
	ArtifactID getTypeId() const;
	const CArtifact * getType() const;
	ArtifactInstanceID getId() const;
	void setId(ArtifactInstanceID id);

	bool canBePutAt(const CArtifactSet * artSet, ArtifactPosition slot = ArtifactPosition::FIRST_AVAILABLE,
		bool assumeDestRemoved = false) const;
	bool isCombined() const;
	bool isScroll() const;
	
	void deserializationFix();
	template <typename Handler> void serialize(Handler & h)
	{
		h & static_cast<CBonusSystemNode&>(*this);
		h & static_cast<CCombinedArtifactInstance&>(*this);
		h & artTypeID;
		h & id;
		BONUS_TREE_DESERIALIZATION_FIX
	}
};

VCMI_LIB_NAMESPACE_END
