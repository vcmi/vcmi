#pragma once


#include "../lib/HeroBonus.h"
//#include "../lib/ConstTransitivePtr.h"
//#include "JsonNode.h"
#include "GameConstants.h"
#include "IHandlerBase.h"

/*
 * CArtHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CArtHandler;
class CDefHandler;
class CArtifact;
class CGHeroInstance;
struct ArtifactLocation;
class CArtifactSet;
class CArtifactInstance;
class CRandomGenerator;

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

class DLL_LINKAGE CArtifact : public CBonusSystemNode //container for artifacts
{
protected:
	std::string name, description; //set if custom
	std::string eventText; //short story displayed upon picking
public:
	enum EartClass {ART_SPECIAL=1, ART_TREASURE=2, ART_MINOR=4, ART_MAJOR=8, ART_RELIC=16}; //artifact classes

	std::string image;
	std::string large; // big image for cutom artifacts, used in drag & drop
	std::string advMapDef; //used for adventure map object
	si32 iconIndex;

	const std::string &Name() const; //getter
	const std::string &Description() const; //getter
	const std::string &EventText() const;

	bool isBig () const;
	bool isTradable () const;

	int getArtClassSerial() const; //0 - treasure, 1 - minor, 2 - major, 3 - relic, 4 - spell scroll, 5 - other
	std::string nodeName() const override;
	void addNewBonus(Bonus *b) override;

	virtual void levelUpArtifact (CArtifactInstance * art){};

	ui32 price;
	std::map<ArtBearer::ArtBearer, std::vector<ArtifactPosition> > possibleSlots; //Bearer Type => ids of slots where artifact can be placed
	std::unique_ptr<std::vector<CArtifact *> > constituents; // Artifacts IDs a combined artifact consists of, or nullptr.
	std::vector<CArtifact *> constituentOf; // Reverse map of constituents - combined arts that include this art
	EartClass aClass;
	ArtifactID id;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CBonusSystemNode&>(*this);
		h & name & description & eventText & image & large & advMapDef & iconIndex &
			price & possibleSlots & constituents & constituentOf & aClass & id;
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

	void levelUpArtifact (CArtifactInstance * art) override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArtifact&>(*this);
		h & bonusesPerLevel & thresholdBonuses;
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

	virtual bool canBePutAt(const CArtifactSet *artSet, ArtifactPosition slot, bool assumeDestRemoved = false) const;
	bool canBePutAt(const ArtifactLocation & al, bool assumeDestRemoved = false) const;  //forwards to the above one
	virtual bool canBeDisassembled() const;
	virtual void putAt(ArtifactLocation al);
	virtual void removeFrom(ArtifactLocation al);
	/// Checks if this a part of this artifact: artifact instance is a part
	/// of itself, additionally truth is returned for constituents of combined arts
	virtual bool isPart(const CArtifactInstance *supposedPart) const;

	std::vector<const CArtifact *> assemblyPossibilities(const CArtifactSet *h) const;
	void move(ArtifactLocation src, ArtifactLocation dst);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CBonusSystemNode&>(*this);
		h & artType & id;
		BONUS_TREE_DESERIALIZATION_FIX
	}

	static CArtifactInstance *createScroll(const CSpell *s);
	static CArtifactInstance *createScroll(SpellID sid);
	static CArtifactInstance *createNewArtifactInstance(CArtifact *Art);
	static CArtifactInstance *createNewArtifactInstance(int aid);
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
			h & art & slot;
		}

		bool operator==(const ConstituentInfo &rhs) const;
		ConstituentInfo(CArtifactInstance *art = nullptr, ArtifactPosition slot = ArtifactPosition::PRE_FIRST);
	};

	std::vector<ConstituentInfo> constituentsInfo;

	bool canBePutAt(const CArtifactSet *artSet, ArtifactPosition slot, bool assumeDestRemoved = false) const override;
	bool canBeDisassembled() const override;
	void putAt(ArtifactLocation al) override;
	void removeFrom(ArtifactLocation al) override;
	bool isPart(const CArtifactInstance *supposedPart) const override;

	void createConstituents();
	void addAsConstituent(CArtifactInstance *art, ArtifactPosition slot);
	CArtifactInstance *figureMainConstituent(const ArtifactLocation al); //main constituent is replaced with us (combined art), not lock

	CCombinedArtifactInstance();

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

class DLL_LINKAGE CArtHandler : public IHandlerBase //handles artifacts
{
public:
	std::vector<CArtifact*> treasures, minors, majors, relics; //tmp vectors!!! do not touch if you don't know what you are doing!!!

	std::vector< ConstTransitivePtr<CArtifact> > artifacts;
	std::vector<CArtifact *> allowedArtifacts;
	std::set<ArtifactID> bigArtifacts; // Artifacts that cannot be moved to backpack, e.g. war machines.
	std::set<ArtifactID> growingArtifacts;

	void addBonuses(CArtifact *art, const JsonNode &bonusList);

	void fillList(std::vector<CArtifact*> &listToBeFilled, CArtifact::EartClass artifactClass); //fills given empty list with allowed artifacts of gibven class. No side effects

	boost::optional<std::vector<CArtifact*>&> listFromClass(CArtifact::EartClass artifactClass);

	ArtifactPosition stringToSlot(std::string slotName);
	CArtifact::EartClass stringToClass(std::string className);

	/// Gets a artifact ID randomly and removes the selected artifact from this handler.
	ArtifactID pickRandomArtifact(CRandomGenerator & rand, int flags);
	ArtifactID pickRandomArtifact(CRandomGenerator & rand, std::function<bool(ArtifactID)> accepts);
	ArtifactID pickRandomArtifact(CRandomGenerator & rand, int flags, std::function<bool(ArtifactID)> accepts);

	bool legalArtifact(ArtifactID id);
	//void getAllowedArts(std::vector<ConstTransitivePtr<CArtifact> > &out, std::vector<CArtifact*> *arts, int flag);
	//void getAllowed(std::vector<ConstTransitivePtr<CArtifact> > &out, int flags);
	bool isBigArtifact (ArtifactID artID) const {return bigArtifacts.find(artID) != bigArtifacts.end();}
	bool isTradableArtifact (ArtifactID id) const;
	void initAllowedArtifactsList(const std::vector<bool> &allowed); //allowed[art_id] -> 0 if not allowed, 1 if allowed
	static ArtifactID creatureToMachineID(CreatureID id);
	static CreatureID machineIDToCreature(ArtifactID id);
	void makeItCreatureArt (CArtifact * a, bool onlyCreature = true);
	void makeItCreatureArt (ArtifactID aid, bool onlyCreature = true);
	void makeItCommanderArt (CArtifact * a, bool onlyCommander = true);
	void makeItCommanderArt (ArtifactID aid, bool onlyCommander = true);

	CArtHandler();
	~CArtHandler();

	std::vector<JsonNode> loadLegacyData(size_t dataSize) override;

	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;
	void afterLoadFinalization() override;

	std::vector<bool> getDefaultAllowed() const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & artifacts & allowedArtifacts & treasures & minors & majors & relics
			& growingArtifacts;
		//if(!h.saving) sortArts();
	}

private:
	CArtifact * loadFromJson(const JsonNode & node);

	void addSlot(CArtifact * art, const std::string & slotID);
	void loadSlots(CArtifact * art, const JsonNode & node);
	void loadClass(CArtifact * art, const JsonNode & node);
	void loadType(CArtifact * art, const JsonNode & node);
	void loadComponents(CArtifact * art, const JsonNode & node);
	void loadGrowingArt(CGrowingArtifact * art, const JsonNode & node);

	void giveArtBonus(ArtifactID aid, Bonus::BonusType type, int val, int subtype = -1, Bonus::ValueType valType = Bonus::BASE_NUMBER, std::shared_ptr<ILimiter> limiter = std::shared_ptr<ILimiter>(), int additionalinfo = 0);
	void giveArtBonus(ArtifactID aid, Bonus::BonusType type, int val, int subtype, std::shared_ptr<IPropagator> propagator, int additionalinfo = 0);
	void giveArtBonus(ArtifactID aid, Bonus *bonus);

	void erasePickedArt(ArtifactID id);
};

struct DLL_LINKAGE ArtSlotInfo
{
	ConstTransitivePtr<CArtifactInstance> artifact;
	ui8 locked; //if locked, then artifact points to the combined artifact

	ArtSlotInfo() : locked(false) {}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & artifact & locked;
	}
};

class DLL_LINKAGE CArtifactSet
{
public:
	std::vector<ArtSlotInfo> artifactsInBackpack; //hero's artifacts from bag
	std::map<ArtifactPosition, ArtSlotInfo> artifactsWorn; //map<position,artifact_id>; positions: 0 - head; 1 - shoulders; 2 - neck; 3 - right hand; 4 - left hand; 5 - torso; 6 - right ring; 7 - left ring; 8 - feet; 9 - misc1; 10 - misc2; 11 - misc3; 12 - misc4; 13 - mach1; 14 - mach2; 15 - mach3; 16 - mach4; 17 - spellbook; 18 - misc5

	ArtSlotInfo &retreiveNewArtSlot(ArtifactPosition slot);
	void setNewArtSlot(ArtifactPosition slot, CArtifactInstance *art, bool locked);
	void eraseArtSlot(ArtifactPosition slot);

	const ArtSlotInfo *getSlot(ArtifactPosition pos) const;
	const CArtifactInstance* getArt(ArtifactPosition pos, bool excludeLocked = true) const; //nullptr - no artifact
	CArtifactInstance* getArt(ArtifactPosition pos, bool excludeLocked = true); //nullptr - no artifact
	/// Looks for equipped artifact with given ID and returns its slot ID or -1 if none
	/// (if more than one such artifact lower ID is returned)
	ArtifactPosition getArtPos(int aid, bool onlyWorn = true) const;
	ArtifactPosition getArtPos(const CArtifactInstance *art) const;
	const CArtifactInstance *getArtByInstanceId(ArtifactInstanceID artInstId) const;
	/// Search for constituents of assemblies in backpack which do not have an ArtifactPosition
	const CArtifactInstance *getHiddenArt(int aid) const;
	const CCombinedArtifactInstance *getAssemblyByConstituent(int aid) const;
	/// Checks if hero possess artifact of given id (either in backack or worn)
	bool hasArt(ui32 aid, bool onlyWorn = false, bool searchBackpackAssemblies = false) const;
	bool isPositionFree(ArtifactPosition pos, bool onlyLockCheck = false) const;
	si32 getArtTypeId(ArtifactPosition pos) const;

	virtual ArtBearer::ArtBearer bearerType() const = 0;
	virtual ~CArtifactSet();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & artifactsInBackpack & artifactsWorn;
	}

	void artDeserializationFix(CBonusSystemNode *node);

protected:
	std::pair<const CCombinedArtifactInstance *, const CArtifactInstance *> searchForConstituent(int aid) const;
};
