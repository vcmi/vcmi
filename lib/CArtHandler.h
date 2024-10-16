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

#include <vcmi/Artifact.h>
#include <vcmi/ArtifactService.h>

#include "bonuses/Bonus.h"
#include "bonuses/CBonusSystemNode.h"
#include "ConstTransitivePtr.h"
#include "GameConstants.h"
#include "IHandlerBase.h"
#include "serializer/Serializeable.h"

VCMI_LIB_NAMESPACE_BEGIN

class CArtHandler;
class CGHeroInstance;
class CArtifactSet;
class CArtifactInstance;
class JsonSerializeFormat;

#define ART_BEARER_LIST \
	ART_BEARER(HERO)\
	ART_BEARER(CREATURE)\
	ART_BEARER(COMMANDER)\
	ART_BEARER(ALTAR)

namespace ArtBearer
{
	enum ArtBearer
	{
#define ART_BEARER(x) x,
		ART_BEARER_LIST
#undef ART_BEARER
	};
}

class DLL_LINKAGE CCombinedArtifact
{
protected:
	CCombinedArtifact() : fused(false) {};

	std::vector<const CArtifact*> constituents; // Artifacts IDs a combined artifact consists of, or nullptr.
	std::set<const CArtifact*> partOf; // Reverse map of constituents - combined arts that include this art
	bool fused;

public:
	bool isCombined() const;
	const std::vector<const CArtifact*> & getConstituents() const;
	const std::set<const CArtifact*> & getPartOf() const;
	void setFused(bool isFused);
	bool isFused() const;
	bool hasParts() const;
};

class DLL_LINKAGE CScrollArtifact
{
protected:
	CScrollArtifact() = default;
public:
	bool isScroll() const;
};

class DLL_LINKAGE CGrowingArtifact
{
protected:
	CGrowingArtifact() = default;

	std::vector <std::pair<ui16, Bonus>> bonusesPerLevel; // Bonus given each n levels
	std::vector <std::pair<ui16, Bonus>> thresholdBonuses; // After certain level they will be added once
public:
	bool isGrowing() const;

	std::vector <std::pair<ui16, Bonus>> & getBonusesPerLevel();
	const std::vector <std::pair<ui16, Bonus>> & getBonusesPerLevel() const;
	std::vector <std::pair<ui16, Bonus>> & getThresholdBonuses();
	const std::vector <std::pair<ui16, Bonus>> & getThresholdBonuses() const;
};

// Container for artifacts. Not for instances.
class DLL_LINKAGE CArtifact
	: public Artifact, public CBonusSystemNode, public CCombinedArtifact, public CScrollArtifact, public CGrowingArtifact
{
	ArtifactID id;
	std::string image;
	std::string large; // big image for custom artifacts, used in drag & drop
	std::string advMapDef; // used for adventure map object
	std::string modScope;
	std::string identifier;
	int32_t iconIndex;
	uint32_t price;
	CreatureID warMachine;
	// Bearer Type => ids of slots where artifact can be placed
	std::map<ArtBearer::ArtBearer, std::vector<ArtifactPosition>> possibleSlots;

public:
	enum EartClass {ART_SPECIAL=1, ART_TREASURE=2, ART_MINOR=4, ART_MAJOR=8, ART_RELIC=16}; //artifact classes

	EartClass aClass = ART_SPECIAL;
	bool onlyOnWaterMap;

	int32_t getIndex() const override;
	int32_t getIconIndex() const override;
	std::string getJsonKey() const override;
	std::string getModScope() const override;
	void registerIcons(const IconRegistar & cb) const override;
	ArtifactID getId() const override;
	const IBonusBearer * getBonusBearer() const override;

	std::string getDescriptionTranslated() const override;
	std::string getEventTranslated() const override;
	std::string getNameTranslated() const override;

	std::string getDescriptionTextID() const override;
	std::string getEventTextID() const override;
	std::string getNameTextID() const override;

	uint32_t getPrice() const override;
	CreatureID getWarMachine() const override;
	bool isBig() const override;
	bool isTradable() const override;

	int getArtClassSerial() const; //0 - treasure, 1 - minor, 2 - major, 3 - relic, 4 - spell scroll, 5 - other
	std::string nodeName() const override;
	void addNewBonus(const std::shared_ptr<Bonus>& b) override;
	const std::map<ArtBearer::ArtBearer, std::vector<ArtifactPosition>> & getPossibleSlots() const;

	virtual bool canBePutAt(const CArtifactSet * artSet, ArtifactPosition slot = ArtifactPosition::FIRST_AVAILABLE,
		bool assumeDestRemoved = false) const;
	void updateFrom(const JsonNode & data);
	// Is used for testing purposes only
	void setImage(int32_t iconIndex, std::string image, std::string large);

	CArtifact();
	~CArtifact();

	friend class CArtHandler;
};

class DLL_LINKAGE CArtHandler : public CHandlerBase<ArtifactID, Artifact, CArtifact, ArtifactService>
{
public:
	void addBonuses(CArtifact *art, const JsonNode &bonusList);

	static CArtifact::EartClass stringToClass(const std::string & className); //TODO: rework EartClass to make this a constructor

	bool legalArtifact(const ArtifactID & id) const;
	static void makeItCreatureArt(CArtifact * a, bool onlyCreature = true);
	static void makeItCommanderArt(CArtifact * a, bool onlyCommander = true);

	~CArtHandler();

	std::vector<JsonNode> loadLegacyData() override;

	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;
	void afterLoadFinalization() override;

	std::set<ArtifactID> getDefaultAllowed() const;

protected:
	const std::vector<std::string> & getTypeNames() const override;
	std::shared_ptr<CArtifact> loadFromJson(const std::string & scope, const JsonNode & json, const std::string & identifier, size_t index) override;

private:
	void addSlot(CArtifact * art, const std::string & slotID) const;
	void loadSlots(CArtifact * art, const JsonNode & node) const;
	void loadClass(CArtifact * art, const JsonNode & node) const;
	void loadType(CArtifact * art, const JsonNode & node) const;
	void loadComponents(CArtifact * art, const JsonNode & node);
};

struct DLL_LINKAGE ArtSlotInfo
{
	CArtifactInstance * artifact;
	bool locked; //if locked, then artifact points to the combined artifact

	ArtSlotInfo() : artifact(nullptr), locked(false) {}
	const CArtifactInstance * getArt() const;

	template <typename Handler> void serialize(Handler & h)
	{
		h & artifact;
		h & locked;
	}
};

class DLL_LINKAGE CArtifactSet : public virtual Serializeable
{
public:
	using ArtPlacementMap = std::map<CArtifactInstance*, ArtifactPosition>;

	std::vector<ArtSlotInfo> artifactsInBackpack; //hero's artifacts from bag
	std::map<ArtifactPosition, ArtSlotInfo> artifactsWorn; //map<position,artifact_id>; positions: 0 - head; 1 - shoulders; 2 - neck; 3 - right hand; 4 - left hand; 5 - torso; 6 - right ring; 7 - left ring; 8 - feet; 9 - misc1; 10 - misc2; 11 - misc3; 12 - misc4; 13 - mach1; 14 - mach2; 15 - mach3; 16 - mach4; 17 - spellbook; 18 - misc5
	ArtSlotInfo artifactsTransitionPos; // Used as transition position for dragAndDrop artifact exchange

	const ArtSlotInfo * getSlot(const ArtifactPosition & pos) const;
	void lockSlot(const ArtifactPosition & pos);
	CArtifactInstance * getArt(const ArtifactPosition & pos, bool excludeLocked = true) const;
	/// Looks for first artifact with given ID
	ArtifactPosition getArtPos(const ArtifactID & aid, bool onlyWorn = true, bool allowLocked = true) const;
	ArtifactPosition getArtPos(const CArtifactInstance * art) const;
	const CArtifactInstance * getArtByInstanceId(const ArtifactInstanceID & artInstId) const;
	bool hasArt(const ArtifactID & aid, bool onlyWorn = false, bool searchCombinedParts = false) const;
	bool isPositionFree(const ArtifactPosition & pos, bool onlyLockCheck = false) const;

	virtual ArtBearer::ArtBearer bearerType() const = 0;
	virtual ArtPlacementMap putArtifact(const ArtifactPosition & slot, CArtifactInstance * art);
	virtual void removeArtifact(const ArtifactPosition & slot);
	virtual ~CArtifactSet() = default;

	template <typename Handler> void serialize(Handler &h)
	{
		h & artifactsInBackpack;
		h & artifactsWorn;
	}

	void artDeserializationFix(CBonusSystemNode *node);

	void serializeJsonArtifacts(JsonSerializeFormat & handler, const std::string & fieldName);
	const CArtifactInstance * getCombinedArtWithPart(const ArtifactID & partId) const;

private:
	void serializeJsonHero(JsonSerializeFormat & handler);
	void serializeJsonCreature(JsonSerializeFormat & handler);
	void serializeJsonCommander(JsonSerializeFormat & handler);

	void serializeJsonSlot(JsonSerializeFormat & handler, const ArtifactPosition & slot);//normal slots
};

// Used to try on artifacts before the claimed changes have been applied
class DLL_LINKAGE CArtifactFittingSet : public CArtifactSet
{
public:
	CArtifactFittingSet(ArtBearer::ArtBearer Bearer);
	explicit CArtifactFittingSet(const CArtifactSet & artSet);
	ArtBearer::ArtBearer bearerType() const override;

protected:
	ArtBearer::ArtBearer bearer;
};

VCMI_LIB_NAMESPACE_END
