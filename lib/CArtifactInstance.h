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

VCMI_LIB_NAMESPACE_BEGIN

struct ArtifactLocation;

class DLL_LINKAGE CCombinedArtifactInstance
{
protected:
	CCombinedArtifactInstance() = default;
public:
	struct PartInfo
	{
		ConstTransitivePtr<CArtifactInstance> art;
		ArtifactPosition slot;
		template <typename Handler> void serialize(Handler & h, const int version)
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
	std::vector<PartInfo> & getPartsInfo();
	const std::vector<PartInfo> & getPartsInfo() const;

	template <typename Handler> void serialize(Handler & h, const int version)
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
public:
	ConstTransitivePtr<CArtifact> artType;

	CArtifactInstance(CArtifact * art);
	CArtifactInstance();
	void setType(CArtifact * art);
	std::string nodeName() const override;
	std::string getDescription() const;
	ArtifactID getTypeId() const;
	ArtifactInstanceID getId() const;
	void setId(ArtifactInstanceID id);

	bool canBePutAt(const ArtifactLocation & al, bool assumeDestRemoved = false) const;
	bool isCombined() const;
	void putAt(const ArtifactLocation & al);
	void removeFrom(const ArtifactLocation & al);
	void move(const ArtifactLocation & src, const ArtifactLocation & dst);
	
	void deserializationFix();
	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<CBonusSystemNode&>(*this);
		h & static_cast<CCombinedArtifactInstance&>(*this);
		h & artType;
		h & id;
		BONUS_TREE_DESERIALIZATION_FIX
	}
};

VCMI_LIB_NAMESPACE_END
