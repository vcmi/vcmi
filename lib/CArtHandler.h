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
#include "GameConstants.h"
#include "IHandlerBase.h"

VCMI_LIB_NAMESPACE_BEGIN

class CArtHandler;
class CGHeroInstance;
class CArtifactSet;
class CArtifactInstance;
class CRandomGenerator;
class CMap;
class JsonSerializeFormat;

#define ART_BEARER_LIST \
	ART_BEARER(HERO)\
	ART_BEARER(CREATURE)\
	ART_BEARER(COMMANDER)

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
	CCombinedArtifact() = default;
public:
	std::unique_ptr<std::vector<CArtifact*>> constituents; // Artifacts IDs a combined artifact consists of, or nullptr.
	std::vector<CArtifact*> partOf; // Reverse map of constituents - combined arts that include this art

	bool isCombined() const;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & constituents;
		h & partOf;
	}
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
public:
	std::vector <std::pair<ui16, Bonus>> bonusesPerLevel; // Bonus given each n levels
	std::vector <std::pair<ui16, Bonus>> thresholdBonuses; // After certain level they will be added once

	bool isGrowing() const;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & bonusesPerLevel;
		h & thresholdBonuses;
	}
};

// Container for artifacts. Not for instances.
class DLL_LINKAGE CArtifact
	: public Artifact, public CBonusSystemNode, public CCombinedArtifact, public CScrollArtifact, public CGrowingArtifact
{
	ArtifactID id;

	std::string modScope;
	std::string identifier;

public:
	enum EartClass {ART_SPECIAL=1, ART_TREASURE=2, ART_MINOR=4, ART_MAJOR=8, ART_RELIC=16}; //artifact classes

	std::string image;
	std::string large; // big image for custom artifacts, used in drag & drop
	std::string advMapDef; //used for adventure map object
	si32 iconIndex = ArtifactID::NONE;
	ui32 price = 0;
	std::map<ArtBearer::ArtBearer, std::vector<ArtifactPosition> > possibleSlots; //Bearer Type => ids of slots where artifact can be placed
	EartClass aClass = ART_SPECIAL;
	CreatureID warMachine;

	int32_t getIndex() const override;
	int32_t getIconIndex() const override;
	std::string getJsonKey() const override;
	void registerIcons(const IconRegistar & cb) const override;
	ArtifactID getId() const override;
	virtual const IBonusBearer * getBonusBearer() const override;

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

	virtual bool canBePutAt(const CArtifactSet * artSet, ArtifactPosition slot = ArtifactPosition::FIRST_AVAILABLE,
		bool assumeDestRemoved = false) const;
	void updateFrom(const JsonNode & data);
	void serializeJson(JsonSerializeFormat & handler);

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<CBonusSystemNode&>(*this);
		h & static_cast<CCombinedArtifact&>(*this);
		h & static_cast<CGrowingArtifact&>(*this);
		h & image;
		h & large;
		h & advMapDef;
		h & iconIndex;
		h & price;
		h & possibleSlots;
		h & aClass;
		h & id;
		h & modScope;
		h & identifier;
		h & warMachine;
	}

	CArtifact();
	~CArtifact();

	friend class CArtHandler;
};

class DLL_LINKAGE CArtHandler : public CHandlerBase<ArtifactID, Artifact, CArtifact, ArtifactService>
{
public:
	std::vector<CArtifact*> treasures, minors, majors, relics; //tmp vectors!!! do not touch if you don't know what you are doing!!!

	std::vector<CArtifact *> allowedArtifacts;
	std::set<ArtifactID> growingArtifacts;

	void addBonuses(CArtifact *art, const JsonNode &bonusList);

	void fillList(std::vector<CArtifact*> &listToBeFilled, CArtifact::EartClass artifactClass); //fills given empty list with allowed artifacts of given class. No side effects

	static CArtifact::EartClass stringToClass(const std::string & className); //TODO: rework EartClass to make this a constructor

	/// Gets a artifact ID randomly and removes the selected artifact from this handler.
	ArtifactID pickRandomArtifact(CRandomGenerator & rand, int flags);
	ArtifactID pickRandomArtifact(CRandomGenerator & rand, std::function<bool(ArtifactID)> accepts);
	ArtifactID pickRandomArtifact(CRandomGenerator & rand, int flags, std::function<bool(ArtifactID)> accepts);

	bool legalArtifact(const ArtifactID & id);
	void initAllowedArtifactsList(const std::vector<bool> &allowed); //allowed[art_id] -> 0 if not allowed, 1 if allowed
	static void makeItCreatureArt(CArtifact * a, bool onlyCreature = true);
	static void makeItCommanderArt(CArtifact * a, bool onlyCommander = true);

	~CArtHandler();

	std::vector<JsonNode> loadLegacyData() override;

	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;
	void afterLoadFinalization() override;

	std::vector<bool> getDefaultAllowed() const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & objects;
		h & allowedArtifacts;
		h & treasures;
		h & minors;
		h & majors;
		h & relics;
		h & growingArtifacts;
	}

protected:
	const std::vector<std::string> & getTypeNames() const override;
	CArtifact * loadFromJson(const std::string & scope, const JsonNode & json, const std::string & identifier, size_t index) override;

private:
	void addSlot(CArtifact * art, const std::string & slotID) const;
	void loadSlots(CArtifact * art, const JsonNode & node) const;
	void loadClass(CArtifact * art, const JsonNode & node) const;
	void loadType(CArtifact * art, const JsonNode & node) const;
	void loadComponents(CArtifact * art, const JsonNode & node);

	void erasePickedArt(const ArtifactID & id);
};

struct DLL_LINKAGE ArtSlotInfo
{
	ConstTransitivePtr<CArtifactInstance> artifact;
	ui8 locked; //if locked, then artifact points to the combined artifact

	ArtSlotInfo() : locked(false) {}
	const CArtifactInstance * getArt() const;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & artifact;
		h & locked;
	}
};

class DLL_LINKAGE CArtifactSet
{
public:
	std::vector<ArtSlotInfo> artifactsInBackpack; //hero's artifacts from bag
	std::map<ArtifactPosition, ArtSlotInfo> artifactsWorn; //map<position,artifact_id>; positions: 0 - head; 1 - shoulders; 2 - neck; 3 - right hand; 4 - left hand; 5 - torso; 6 - right ring; 7 - left ring; 8 - feet; 9 - misc1; 10 - misc2; 11 - misc3; 12 - misc4; 13 - mach1; 14 - mach2; 15 - mach3; 16 - mach4; 17 - spellbook; 18 - misc5
	std::vector<ArtSlotInfo> artifactsTransitionPos; // Used as transition position for dragAndDrop artifact exchange

	void setNewArtSlot(const ArtifactPosition & slot, CArtifactInstance * art, bool locked);
	void eraseArtSlot(const ArtifactPosition & slot);

	const ArtSlotInfo * getSlot(const ArtifactPosition & pos) const;
	const CArtifactInstance * getArt(const ArtifactPosition & pos, bool excludeLocked = true) const; //nullptr - no artifact
	CArtifactInstance * getArt(const ArtifactPosition & pos, bool excludeLocked = true); //nullptr - no artifact
	/// Looks for equipped artifact with given ID and returns its slot ID or -1 if none
	/// (if more than one such artifact lower ID is returned)
	ArtifactPosition getArtPos(const ArtifactID & aid, bool onlyWorn = true, bool allowLocked = true) const;
	ArtifactPosition getArtPos(const CArtifactInstance *art) const;
	ArtifactPosition getArtBackpackPos(const ArtifactID & aid) const;
	std::vector<ArtifactPosition> getAllArtPositions(const ArtifactID & aid, bool onlyWorn, bool allowLocked, bool getAll) const;
	std::vector<ArtifactPosition> getBackpackArtPositions(const ArtifactID & aid) const;
	const CArtifactInstance * getArtByInstanceId(const ArtifactInstanceID & artInstId) const;
	const ArtifactPosition getSlotByInstance(const CArtifactInstance * artInst) const;
	/// Search for constituents of assemblies in backpack which do not have an ArtifactPosition
	const CArtifactInstance * getHiddenArt(const ArtifactID & aid) const;
	const CArtifactInstance * getAssemblyByConstituent(const ArtifactID & aid) const;
	/// Checks if hero possess artifact of given id (either in backack or worn)
	bool hasArt(const ArtifactID & aid, bool onlyWorn = false, bool searchBackpackAssemblies = false, bool allowLocked = true) const;
	bool hasArtBackpack(const ArtifactID & aid) const;
	bool isPositionFree(const ArtifactPosition & pos, bool onlyLockCheck = false) const;
	unsigned getArtPosCount(const ArtifactID & aid, bool onlyWorn = true, bool searchBackpackAssemblies = true, bool allowLocked = true) const;

	virtual ArtBearer::ArtBearer bearerType() const = 0;
	virtual void putArtifact(ArtifactPosition slot, CArtifactInstance * art);
	virtual void removeArtifact(ArtifactPosition slot);
	virtual ~CArtifactSet();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & artifactsInBackpack;
		h & artifactsWorn;
	}

	void artDeserializationFix(CBonusSystemNode *node);

	void serializeJsonArtifacts(JsonSerializeFormat & handler, const std::string & fieldName, CMap * map);
protected:
	std::pair<const CArtifactInstance *, const CArtifactInstance *> searchForConstituent(const ArtifactID & aid) const;

private:
	void serializeJsonHero(JsonSerializeFormat & handler, CMap * map);
	void serializeJsonCreature(JsonSerializeFormat & handler, CMap * map);
	void serializeJsonCommander(JsonSerializeFormat & handler, CMap * map);

	void serializeJsonSlot(JsonSerializeFormat & handler, const ArtifactPosition & slot, CMap * map);//normal slots
};

// Used to try on artifacts before the claimed changes have been applied
class DLL_LINKAGE CArtifactFittingSet : public CArtifactSet
{
public:
	CArtifactFittingSet(ArtBearer::ArtBearer Bearer);
	ArtBearer::ArtBearer bearerType() const override;

protected:
	ArtBearer::ArtBearer Bearer;
};

VCMI_LIB_NAMESPACE_END
