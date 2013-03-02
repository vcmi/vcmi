#pragma once


#include "../lib/HeroBonus.h"
#include "../lib/ConstTransitivePtr.h"
#include "JsonNode.h"
#include "GameConstants.h"

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
	si32 iconIndex; //TODO: handle automatically

	const std::string &Name() const; //getter
	const std::string &Description() const; //getter
	const std::string &EventText() const;

	bool isBig () const;

	int getArtClassSerial() const; //0 - treasure, 1 - minor, 2 - major, 3 - relic, 4 - spell scroll, 5 - other
	std::string nodeName() const override;
	void addNewBonus(Bonus *b) override;

	virtual void levelUpArtifact (CArtifactInstance * art){};

	ui32 price;
	bmap<ArtBearer::ArtBearer, std::vector<ArtifactPosition> > possibleSlots; //Bearer Type => ids of slots where artifact can be placed
	std::unique_ptr<std::vector<CArtifact *> > constituents; // Artifacts IDs a combined artifact consists of, or NULL.
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

	void levelUpArtifact (CArtifactInstance * art);

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

	std::string nodeName() const OVERRIDE;
	void deserializationFix();
	void setType(CArtifact *Art);

	ArtifactPosition firstAvailableSlot(const CArtifactSet *h) const;
	ArtifactPosition firstBackpackSlot(const CArtifactSet *h) const;
	SpellID getGivenSpellID() const; //to be used with scrolls (and similar arts), -1 if none

	virtual bool canBePutAt(const CArtifactSet *artSet, ArtifactPosition slot, bool assumeDestRemoved = false) const;
	bool canBePutAt(const ArtifactLocation al, bool assumeDestRemoved = false) const;  //forwards to the above one
	virtual bool canBeDisassembled() const;
	virtual void putAt(ArtifactLocation al);
	virtual void removeFrom(ArtifactLocation al);
	virtual bool isPart(const CArtifactInstance *supposedPart) const; //checks if this a part of this artifact: artifact instance is a part of itself, additionally truth is returned for consituents of combined arts

	std::vector<const CArtifact *> assemblyPossibilities(const CArtifactSet *h) const;
	void move(ArtifactLocation src, ArtifactLocation dst);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CBonusSystemNode&>(*this);
		h & artType & id;
		BONUS_TREE_DESERIALIZATION_FIX
	}

	static CArtifactInstance *createScroll(const CSpell *s);
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
		ConstituentInfo(CArtifactInstance *art = NULL, ArtifactPosition slot = ArtifactPosition::PRE_FIRST);
	};

	std::vector<ConstituentInfo> constituentsInfo;

	bool canBePutAt(const CArtifactSet *artSet, ArtifactPosition slot, bool assumeDestRemoved = false) const OVERRIDE;
	bool canBeDisassembled() const OVERRIDE;
	void putAt(ArtifactLocation al) OVERRIDE;
	void removeFrom(ArtifactLocation al) OVERRIDE;
	bool isPart(const CArtifactInstance *supposedPart) const OVERRIDE;

	void createConstituents();
	void addAsConstituent(CArtifactInstance *art, ArtifactPosition slot);
	CArtifactInstance *figureMainConstituent(const ArtifactLocation al); //main constituent is replcaed with us (combined art), not lock

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

class DLL_LINKAGE CArtHandler //handles artifacts
{
	void giveArtBonus(ArtifactID aid, Bonus::BonusType type, int val, int subtype = -1, Bonus::ValueType valType = Bonus::BASE_NUMBER, shared_ptr<ILimiter> limiter = shared_ptr<ILimiter>(), int additionalinfo = 0);
	void giveArtBonus(ArtifactID aid, Bonus::BonusType type, int val, int subtype, shared_ptr<IPropagator> propagator, int additionalinfo = 0);
	void giveArtBonus(ArtifactID aid, Bonus *bonus);
public:
	std::vector<CArtifact*> treasures, minors, majors, relics; //tmp vectors!!! do not touch if you don't know what you are doing!!!

	std::vector< ConstTransitivePtr<CArtifact> > artifacts;
	std::vector<CArtifact *> allowedArtifacts;
	std::set<ArtifactID> bigArtifacts; // Artifacts that cannot be moved to backpack, e.g. war machines.
	std::set<ArtifactID> growingArtifacts;

	void loadArtifacts(bool onlyTxt);
	/// load artifact from json structure
	void load(std::string objectID, const JsonNode & node);
	/// load one artifact from json config
	CArtifact * loadArtifact(const JsonNode & node);

	void loadArtifactJson(CArtifact * art, const JsonNode & node);

	void addBonuses(CArtifact *art, const JsonNode &bonusList);

	void fillList(std::vector<CArtifact*> &listToBeFilled, CArtifact::EartClass artifactClass); //fills given empty list with allowed artifacts of gibven class. No side effects

	boost::optional<std::vector<CArtifact*>&> listFromClass(CArtifact::EartClass artifactClass);
	void erasePickedArt(ArtifactID id);
	ArtifactID getRandomArt (int flags);
	ArtifactID getArtSync (ui32 rand, int flags, bool erasePicked = false);
	bool legalArtifact(ArtifactID id);
	void getAllowedArts(std::vector<ConstTransitivePtr<CArtifact> > &out, std::vector<CArtifact*> *arts, int flag);
	void getAllowed(std::vector<ConstTransitivePtr<CArtifact> > &out, int flags);
	bool isBigArtifact (ArtifactID artID) const {return bigArtifacts.find(artID) != bigArtifacts.end();}
	void initAllowedArtifactsList(const std::vector<bool> &allowed); //allowed[art_id] -> 0 if not allowed, 1 if allowed
	static ArtifactID creatureToMachineID(CreatureID id);
	static CreatureID machineIDToCreature(ArtifactID id);
	void makeItCreatureArt (CArtifact * a, bool onlyCreature = true);
	void makeItCreatureArt (ArtifactID aid, bool onlyCreature = true);
	void makeItCommanderArt (CArtifact * a, bool onlyCommander = true);
	void makeItCommanderArt (ArtifactID aid, bool onlyCommander = true);
	CArtHandler();
	~CArtHandler();

	/**
	 * Gets a list of default allowed artifacts.
	 *
	 * @return a list of allowed artifacts, the index is the artifact id
	 */
	std::vector<bool> getDefaultAllowedArtifacts() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & artifacts & allowedArtifacts & treasures & minors & majors & relics
			& growingArtifacts;
		//if(!h.saving) sortArts();
	}
};

struct DLL_LINKAGE ArtSlotInfo
{
	ConstTransitivePtr<CArtifactInstance> artifact;
	ui8 locked; //if locked, then artifact points to the combined artifact

	ArtSlotInfo()
	{
		locked = false;
	}
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & artifact & locked;
	}
};

class DLL_LINKAGE CArtifactSet
{
public:
	std::vector<ArtSlotInfo> artifactsInBackpack; //hero's artifacts from bag
	bmap<ArtifactPosition, ArtSlotInfo> artifactsWorn; //map<position,artifact_id>; positions: 0 - head; 1 - shoulders; 2 - neck; 3 - right hand; 4 - left hand; 5 - torso; 6 - right ring; 7 - left ring; 8 - feet; 9 - misc1; 10 - misc2; 11 - misc3; 12 - misc4; 13 - mach1; 14 - mach2; 15 - mach3; 16 - mach4; 17 - spellbook; 18 - misc5

	ArtSlotInfo &retreiveNewArtSlot(ArtifactPosition slot);
	void setNewArtSlot(ArtifactPosition slot, CArtifactInstance *art, bool locked);
	void eraseArtSlot(ArtifactPosition slot);

	const ArtSlotInfo *getSlot(ArtifactPosition pos) const;
	const CArtifactInstance* getArt(ArtifactPosition pos, bool excludeLocked = true) const; //NULL - no artifact
	CArtifactInstance* getArt(ArtifactPosition pos, bool excludeLocked = true); //NULL - no artifact
	ArtifactPosition getArtPos(int aid, bool onlyWorn = true) const; //looks for equipped artifact with given ID and returns its slot ID or -1 if none(if more than one such artifact lower ID is returned)
	ArtifactPosition getArtPos(const CArtifactInstance *art) const;
	const CArtifactInstance *getArtByInstanceId(ArtifactInstanceID artInstId) const;
	bool hasArt(ui32 aid, bool onlyWorn = false) const; //checks if hero possess artifact of given id (either in backack or worn)
	bool isPositionFree(ArtifactPosition pos, bool onlyLockCheck = false) const;
	si32 getArtTypeId(ArtifactPosition pos) const;

	virtual ArtBearer::ArtBearer bearerType() const = 0;
	virtual ~CArtifactSet();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & artifactsInBackpack & artifactsWorn;
	}

	void artDeserializationFix(CBonusSystemNode *node);
};
