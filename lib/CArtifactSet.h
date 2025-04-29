/*
 * CArtHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CArtHandler.h"
#include "GameCallbackHolder.h"
#include "constants/EntityIdentifiers.h"

#include "CArtifactInstance.h"

VCMI_LIB_NAMESPACE_BEGIN

struct DLL_LINKAGE ArtSlotInfo : public GameCallbackHolder
{
	ArtifactInstanceID artifactID;
	bool locked = false; //if locked, then artifact points to the combined artifact

	explicit ArtSlotInfo(IGameCallback * cb);
	ArtSlotInfo(const CArtifactInstance * artifact, bool locked);

	const CArtifactInstance * getArt() const;
	ArtifactInstanceID getID() const;

	template <typename Handler> void serialize(Handler & h)
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
		h & locked;
	}
};

class DLL_LINKAGE CArtifactSet : public virtual Serializeable
{

public:
	using ArtPlacementMap = std::map<const CArtifactInstance*, ArtifactPosition>;

	std::vector<ArtSlotInfo> artifactsInBackpack; //hero's artifacts from bag
	std::map<ArtifactPosition, ArtSlotInfo> artifactsWorn; //map<position,artifact_id>; positions: 0 - head; 1 - shoulders; 2 - neck; 3 - right hand; 4 - left hand; 5 - torso; 6 - right ring; 7 - left ring; 8 - feet; 9 - misc1; 10 - misc2; 11 - misc3; 12 - misc4; 13 - mach1; 14 - mach2; 15 - mach3; 16 - mach4; 17 - spellbook; 18 - misc5
	ArtSlotInfo artifactsTransitionPos; // Used as transition position for dragAndDrop artifact exchange

	const ArtSlotInfo * getSlot(const ArtifactPosition & pos) const;
	void lockSlot(const ArtifactPosition & pos);
	const CArtifactInstance * getArt(const ArtifactPosition & pos, bool excludeLocked = true) const;
	ArtifactInstanceID getArtID(const ArtifactPosition & pos, bool excludeLocked = true) const;
	/// Looks for first artifact with given ID
	ArtifactPosition getArtPos(const ArtifactID & aid, bool onlyWorn = true, bool allowLocked = true) const;
	ArtifactPosition getArtPos(const CArtifactInstance * art) const;
	const CArtifactInstance * getArtByInstanceId(const ArtifactInstanceID & artInstId) const;
	bool hasArt(const ArtifactID & aid, bool onlyWorn = false, bool searchCombinedParts = false) const;
	bool isPositionFree(const ArtifactPosition & pos, bool onlyLockCheck = false) const;

	virtual IGameCallback * getCallback() const = 0;
	virtual ArtBearer::ArtBearer bearerType() const = 0;
	virtual ArtPlacementMap putArtifact(const ArtifactPosition & slot, const CArtifactInstance * art);
	virtual void removeArtifact(const ArtifactPosition & slot);
	CArtifactSet(IGameCallback * cb);
	virtual ~CArtifactSet() = default;

	template <typename Handler> void serialize(Handler &h)
	{
		h & artifactsInBackpack;
		h & artifactsWorn;
	}

	void artDeserializationFix(CGameState & gs, CBonusSystemNode *node);

	void serializeJsonArtifacts(JsonSerializeFormat & handler, const std::string & fieldName, CMap * map);
	const CArtifactInstance * getCombinedArtWithPart(const ArtifactID & partId) const;

private:
	void serializeJsonHero(JsonSerializeFormat & handler, CMap * map);
	void serializeJsonCreature(JsonSerializeFormat & handler);
	void serializeJsonCommander(JsonSerializeFormat & handler);

	void serializeJsonSlot(JsonSerializeFormat & handler, const ArtifactPosition & slot, CMap * map);//normal slots
};

// Used to try on artifacts before the claimed changes have been applied
class DLL_LINKAGE CArtifactFittingSet : public CArtifactSet, public GameCallbackHolder
{
	IGameCallback * getCallback() const final { return cb; }
public:
	CArtifactFittingSet(IGameCallback *cb, ArtBearer::ArtBearer Bearer);
	explicit CArtifactFittingSet(const CArtifactSet & artSet);
	ArtBearer::ArtBearer bearerType() const override;

protected:
	ArtBearer::ArtBearer bearer;
};

VCMI_LIB_NAMESPACE_END
