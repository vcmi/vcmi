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

#include "HeroBonus.h"
#include "GameConstants.h"
#include "IHandlerBase.h"

VCMI_LIB_NAMESPACE_BEGIN

class CArtHandler;
class CArtifact;
class CGHeroInstance;
struct ArtifactLocation;
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

class DLL_LINKAGE CArtifact : public Artifact, public CBonusSystemNode //container for artifacts
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
	std::unique_ptr<std::vector<CArtifact *> > constituents; // Artifacts IDs a combined artifact consists of, or nullptr.
	std::vector<CArtifact *> constituentOf; // Reverse map of constituents - combined arts that include this art
	EartClass aClass = ART_SPECIAL;
	CreatureID warMachine;

	int32_t getIndex() const override;
	int32_t getIconIndex() const override;
	std::string getJsonKey() const override;
	void registerIcons(const IconRegistar & cb) const override;
	ArtifactID getId() const override;
	virtual const IBonusBearer * accessBonuses() const override;

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

	virtual void levelUpArtifact (CArtifactInstance * art){};

	virtual bool canBeDisassembled() const;
	virtual bool canBePutAt(const CArtifactSet * artSet, ArtifactPosition slot, bool assumeDestRemoved = false) const;
	void updateFrom(const JsonNode & data);
	void serializeJson(JsonSerializeFormat & handler);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CBonusSystemNode&>(*this);
		h & image;
		h & large;
		h & advMapDef;
		h & iconIndex;
		h & price;
		h & possibleSlots;
		h & constituents;
		h & constituentOf;
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

class DLL_LINKAGE CGrowingArtifact : public CArtifact //for example commander artifacts getting bonuses after battle
{
public:
	std::vector <std::pair <ui16, Bonus> > bonusesPerLevel; //bonus given each n levels
	std::vector <std::pair <ui16, Bonus> > thresholdBonuses; //after certain level they will be added once

	void levelUpArtifact(CArtifactInstance * art) override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArtifact&>(*this);
		h & bonusesPerLevel;
		h & thresholdBonuses;
	}
};

class DLL_LINKAGE CArtifactInstance : public CBonusSystemNode
{
protected:
	void init();
	CArtifactInstance(CArtifact *Art);
public:
	CArtifactInstance();

	ConstTransitivePtr<CArtifact> artType;
	ArtifactInstanceID id;

	//CArtifactInstance(int aid);

	std::string nodeName() const override;
	void deserializationFix();
	void setType(CArtifact *Art);

	std::string getEffectiveDescription(const CGHeroInstance *hero = nullptr) const;
	ArtifactPosition firstAvailableSlot(const CArtifactSet *h) const;
	ArtifactPosition firstBackpackSlot(const CArtifactSet *h) const;
	SpellID getGivenSpellID() const; //to be used with scrolls (and similar arts), -1 if none

	bool canBePutAt(const ArtifactLocation & al, bool assumeDestRemoved = false) const;  //forwards to the above one
	virtual bool canBeDisassembled() const;
	virtual void putAt(ArtifactLocation al);
	virtual void removeFrom(ArtifactLocation al);
	/// Checks if this a part of this artifact: artifact instance is a part
	/// of itself, additionally truth is returned for constituents of combined arts
	virtual bool isPart(const CArtifactInstance *supposedPart) const;

	std::vector<const CArtifact *> assemblyPossibilities(const CArtifactSet * h, bool equipped) const;
	void move(const ArtifactLocation & src,const ArtifactLocation & dst);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CBonusSystemNode&>(*this);
		h & artType;
		h & id;
		BONUS_TREE_DESERIALIZATION_FIX
	}

	static CArtifactInstance * createScroll(const SpellID & sid);
	static CArtifactInstance *createNewArtifactInstance(CArtifact *Art);
	static CArtifactInstance * createNewArtifactInstance(const ArtifactID & aid);

	/**
	 * Creates an artifact instance.
	 *
	 * @param aid the id of the artifact
	 * @param spellID optional. the id of a spell if a spell scroll object should be created
	 * @return the created artifact instance
	 */
	static CArtifactInstance * createArtifact(CMap * map, const ArtifactID & aid, int spellID = -1);
};

class DLL_LINKAGE CCombinedArtifactInstance : public CArtifactInstance
{
	CCombinedArtifactInstance(CArtifact *Art);
public:
	struct ConstituentInfo
	{
		ConstTransitivePtr<CArtifactInstance> art;
		ArtifactPosition slot;
		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & art;
			h & slot;
		}

		bool operator==(const ConstituentInfo &rhs) const;
		ConstituentInfo(CArtifactInstance * art = nullptr, const ArtifactPosition & slot = ArtifactPosition::PRE_FIRST);
	};

	std::vector<ConstituentInfo> constituentsInfo;

	void putAt(ArtifactLocation al) override;
	void removeFrom(ArtifactLocation al) override;
	bool isPart(const CArtifactInstance *supposedPart) const override;

	void createConstituents();
	void addAsConstituent(CArtifactInstance * art, const ArtifactPosition & slot);
	CArtifactInstance * figureMainConstituent(const ArtifactLocation & al); //main constituent is replaced with us (combined art), not lock

	CCombinedArtifactInstance() = default;

	void deserializationFix();

	friend class CArtifactInstance;
	friend struct AssembledArtifact;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArtifactInstance&>(*this);
		h & constituentsInfo;
		BONUS_TREE_DESERIALIZATION_FIX
	}
};

class DLL_LINKAGE CArtHandler : public CHandlerBase<ArtifactID, Artifact, CArtifact, ArtifactService>
{
public:
	std::vector<CArtifact*> treasures, minors, majors, relics; //tmp vectors!!! do not touch if you don't know what you are doing!!!

	std::vector<CArtifact *> allowedArtifacts;
	std::set<ArtifactID> growingArtifacts;

	void addBonuses(CArtifact *art, const JsonNode &bonusList);

	void fillList(std::vector<CArtifact*> &listToBeFilled, CArtifact::EartClass artifactClass); //fills given empty list with allowed artifacts of given class. No side effects

	boost::optional<std::vector<CArtifact*>&> listFromClass(CArtifact::EartClass artifactClass);

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
	void loadGrowingArt(CGrowingArtifact * art, const JsonNode & node) const;

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

	ArtSlotInfo & retrieveNewArtSlot(const ArtifactPosition & slot);
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
	/// Search for constituents of assemblies in backpack which do not have an ArtifactPosition
	const CArtifactInstance * getHiddenArt(const ArtifactID & aid) const;
	const CCombinedArtifactInstance * getAssemblyByConstituent(const ArtifactID & aid) const;
	/// Checks if hero possess artifact of given id (either in backack or worn)
	bool hasArt(const ArtifactID & aid, bool onlyWorn = false, bool searchBackpackAssemblies = false, bool allowLocked = true) const;
	bool hasArtBackpack(const ArtifactID & aid) const;
	bool isPositionFree(const ArtifactPosition & pos, bool onlyLockCheck = false) const;
	unsigned getArtPosCount(const ArtifactID & aid, bool onlyWorn = true, bool searchBackpackAssemblies = true, bool allowLocked = true) const;

	virtual ArtBearer::ArtBearer bearerType() const = 0;
	virtual void putArtifact(ArtifactPosition pos, CArtifactInstance * art) = 0;
	virtual ~CArtifactSet();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & artifactsInBackpack;
		h & artifactsWorn;
	}

	void artDeserializationFix(CBonusSystemNode *node);

	void serializeJsonArtifacts(JsonSerializeFormat & handler, const std::string & fieldName, CMap * map);
protected:
	std::pair<const CCombinedArtifactInstance *, const CArtifactInstance *> searchForConstituent(const ArtifactID & aid) const;

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
	void putArtifact(ArtifactPosition pos, CArtifactInstance * art) override;
	void removeArtifact(ArtifactPosition pos);
	ArtBearer::ArtBearer bearerType() const override;

protected:
	ArtBearer::ArtBearer Bearer;
};

namespace ArtifactUtils
{
	// Calculates where an artifact gets placed when it gets transferred from one hero to another.
	DLL_LINKAGE ArtifactPosition getArtifactDstPosition(const ArtifactID & aid, const CArtifactSet * target);
	// TODO: Make this constexpr when the toolset is upgraded
	DLL_LINKAGE const std::vector<ArtifactPosition::EArtifactPosition> & unmovableSlots();
	DLL_LINKAGE const std::vector<ArtifactPosition::EArtifactPosition> & constituentWornSlots();
	DLL_LINKAGE bool isArtRemovable(const std::pair<ArtifactPosition, ArtSlotInfo> & slot);
	DLL_LINKAGE bool checkSpellbookIsNeeded(const CGHeroInstance * heroPtr, const ArtifactID & artID, const ArtifactPosition & slot);
	DLL_LINKAGE bool isSlotBackpack(const ArtifactPosition & slot);
	DLL_LINKAGE bool isSlotEquipment(const ArtifactPosition & slot);
}

VCMI_LIB_NAMESPACE_END
