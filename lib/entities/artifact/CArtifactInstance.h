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
#include "GameCallbackHolder.h"

VCMI_LIB_NAMESPACE_BEGIN

struct ArtifactLocation;
class CGameState;
class CArtifactSet;

class DLL_LINKAGE CCombinedArtifactInstance : public GameCallbackHolder
{
protected:
	using GameCallbackHolder::GameCallbackHolder;
public:
	using ArtPlacementMap = std::map<const CArtifactInstance*, ArtifactPosition>;

	struct PartInfo : public GameCallbackHolder
	{
		explicit PartInfo(IGameCallback * cb);
		PartInfo(const CArtifactInstance * artifact, ArtifactPosition slot);

		ArtifactInstanceID artifactID;
		ArtifactPosition slot;

		const CArtifactInstance * getArtifact() const;

		template <typename Handler>
		void serialize(Handler & h);
	};
	void addPart(const CArtifactInstance * art, const ArtifactPosition & slot);
	// Checks if supposed part inst is part of this combined art inst
	bool isPart(const CArtifactInstance * supposedPart) const;
	bool hasParts() const;
	const std::vector<PartInfo> & getPartsInfo() const;
	void addPlacementMap(const ArtPlacementMap & placementMap);

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

class DLL_LINKAGE CArtifactInstance final
	: public CBonusSystemNode, public CCombinedArtifactInstance, public CScrollArtifactInstance, public CGrowingArtifactInstance
{
	ArtifactInstanceID id;
	ArtifactID artTypeID;

public:
	CArtifactInstance(IGameCallback *cb, const CArtifact * art);
	CArtifactInstance(IGameCallback *cb);
	void setType(const CArtifact * art);
	std::string nodeName() const override;
	ArtifactID getTypeId() const;
	const CArtifact * getType() const;
	ArtifactInstanceID getId() const;
	void setId(ArtifactInstanceID id);

	static void saveCompatibilityFixArtifactID(std::shared_ptr<CArtifactInstance> self);

	bool canBePutAt(const CArtifactSet * artSet, ArtifactPosition slot = ArtifactPosition::FIRST_AVAILABLE,
		bool assumeDestRemoved = false) const;
	bool isCombined() const;
	bool isScroll() const;
	
	void attachToBonusSystem(CGameState & gs);

	template <typename Handler> void serialize(Handler & h)
	{
		h & static_cast<CBonusSystemNode&>(*this);
		h & static_cast<CCombinedArtifactInstance&>(*this);
		h & artTypeID;
		h & id;

		if(!h.saving && h.loadingGamestate)
			setType(artTypeID.toArtifact());

	}
};

template <typename Handler>
void CCombinedArtifactInstance::PartInfo::serialize(Handler & h)
{
	if (h.saving || h.hasFeature(Handler::Version::NO_RAW_POINTERS_IN_SERIALIZER))
	{
		h & artifactID;
	}
	else
	{
		std::shared_ptr<CArtifactInstance> pointer;
		h & pointer;
		if (pointer->getId() == ArtifactInstanceID())
			CArtifactInstance::saveCompatibilityFixArtifactID(pointer);
		artifactID = pointer->getId();
	}
	h & slot;
}

VCMI_LIB_NAMESPACE_END
