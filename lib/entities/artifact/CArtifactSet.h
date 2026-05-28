/*
 * CArtifactSet.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "ArtBearer.h"
#include "ArtSlotInfo.h"
#include "../../serializer/Serializeable.h"

VCMI_LIB_NAMESPACE_BEGIN

class CArtifactInstance;
class CMap;

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
	ArtifactPosition getScrollPos(const SpellID & aid, bool onlyWorn = true) const;
	ArtifactPosition getArtPos(const CArtifactInstance * art) const;
	const CArtifactInstance * getArtByInstanceId(const ArtifactInstanceID & artInstId) const;
	bool hasArt(const ArtifactID & aid, bool onlyWorn = false, bool searchCombinedParts = false) const;
	bool hasScroll(const SpellID & aid, bool onlyWorn = false) const;
	bool isPositionFree(const ArtifactPosition & pos, bool onlyLockCheck = false) const;

	virtual IGameInfoCallback * getCallback() const = 0;
	virtual ArtBearer bearerType() const = 0;
	virtual ArtPlacementMap putArtifact(const ArtifactPosition & slot, const CArtifactInstance * art);
	virtual void removeArtifact(const ArtifactPosition & slot);
	CArtifactSet(IGameInfoCallback * cb);
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

VCMI_LIB_NAMESPACE_END
