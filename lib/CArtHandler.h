#pragma once


#include "../lib/HeroBonus.h"
#include "../lib/ConstTransitivePtr.h"

/*
 * CArtHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
class CDefHandler;
class CArtifact;
class CGHeroInstance;
struct ArtifactLocation;
class CArtifactSet;
class CArtifactInstance;

namespace ArtifactPosition
{
	enum ArtifactPosition
	{
		PRE_FIRST = -1, 
		HEAD, SHOULDERS, NECK, RIGHT_HAND, LEFT_HAND, TORSO, RIGHT_RING, LEFT_RING, FEET, MISC1, MISC2, MISC3, MISC4,
		MACH1, MACH2, MACH3, MACH4, SPELLBOOK, MISC5, 
		AFTER_LAST,
		//cres
		CREATURE_SLOT = 0,
		COMMANDER1 = 0, COMMANDER2, COMMANDER3, COMMANDER4, COMMANDER5, COMMANDER6, COMMANDER_AFTER_LAST
	};
}

namespace ArtifactID
{
	enum ArtifactID
	{
		SPELLBOOK = 0,
		SPELL_SCROLL = 1,
		GRAIL = 2,
		CATAPULT = 3,
		BALLISTA = 4,
		AMMO_CART = 5,
		FIRST_AID_TENT = 6,
		CENTAUR_AXE = 7,
		BLACKSHARD_OF_THE_DEAD_KNIGHT = 8,
		CORNUCOPIA = 140
	};
}

namespace ArtBearer
{
	enum
	{
		HERO, CREATURE, COMMANDER
	};
}

class DLL_LINKAGE CArtifact : public CBonusSystemNode //container for artifacts
{
protected:
	std::string name, description; //set if custom
public:
	enum EartClass {ART_SPECIAL=1, ART_TREASURE=2, ART_MINOR=4, ART_MAJOR=8, ART_RELIC=16}; //artifact classes
	const std::string &Name() const; //getter
	const std::string &Description() const; //getter
	bool isBig () const;
	void setDescription (std::string desc);

	int getArtClassSerial() const; //0 - treasure, 1 - minor, 2 - major, 3 - relic, 4 - spell scroll, 5 - other
	std::string nodeName() const OVERRIDE;

	virtual void levelUpArtifact (CArtifactInstance * art){};

	ui32 price;
	bmap<ui8, std::vector<ui16> > possibleSlots; //Bearer Type => ids of slots where artifact can be placed
	std::vector<ui32> * constituents; // Artifacts IDs a combined artifact consists of, or NULL.
	std::vector<ui32> * constituentOf; // Reverse map of constituents.
	EartClass aClass;
	si32 id;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CBonusSystemNode&>(*this);
		h & name & description & price & possibleSlots & constituents & constituentOf & aClass & id;
	}

	CArtifact();
	~CArtifact();

	//override
	//void getParents(TCNodes &out, const CBonusSystemNode *root = NULL) const;
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
	TArtifactInstanceID id;

	//CArtifactInstance(int aid);

	std::string nodeName() const OVERRIDE;
	void deserializationFix();
	void setType(CArtifact *Art);

	int firstAvailableSlot(const CArtifactSet *h) const;
	int firstBackpackSlot(const CArtifactSet *h) const;
	int getGivenSpellID() const; //to be used with scrolls (and similar arts), -1 if none

	virtual bool canBePutAt(const CArtifactSet *artSet, int slot, bool assumeDestRemoved = false) const;
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
		si16 slot;
		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & art & slot;
		}

		bool operator==(const ConstituentInfo &rhs) const;
		ConstituentInfo(CArtifactInstance *art = NULL, ui16 slot = -1);
	};

	std::vector<ConstituentInfo> constituentsInfo;

	bool canBePutAt(const CArtifactSet *artSet, int slot, bool assumeDestRemoved = false) const OVERRIDE;
	bool canBeDisassembled() const OVERRIDE;
	void putAt(ArtifactLocation al) OVERRIDE;
	void removeFrom(ArtifactLocation al) OVERRIDE;
	bool isPart(const CArtifactInstance *supposedPart) const OVERRIDE;

	void createConstituents();
	void addAsConstituent(CArtifactInstance *art, int slot);
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
	void giveArtBonus(TArtifactID aid, Bonus::BonusType type, int val, int subtype = -1, int valType = Bonus::BASE_NUMBER, shared_ptr<ILimiter> limiter = shared_ptr<ILimiter>(), int additionalinfo = 0);
	void giveArtBonus(TArtifactID aid, Bonus::BonusType type, int val, int subtype, shared_ptr<IPropagator> propagator, int additionalinfo = 0);
	void giveArtBonus(TArtifactID aid, Bonus *bonus);
public:
	std::vector<CArtifact*> treasures, minors, majors, relics;
	std::vector< ConstTransitivePtr<CArtifact> > artifacts;
	std::vector<CArtifact *> allowedArtifacts;
	std::set<ui32> bigArtifacts; // Artifacts that cannot be moved to backpack, e.g. war machines.
	std::set<ui32> growingArtifacts;

	void loadArtifacts(bool onlyTxt);
	void sortArts();
	void addBonuses();
	void clear();
	void clearHlpLists();
	ui16 getRandomArt (int flags);
	ui16 getArtSync (ui32 rand, int flags);
	void getAllowedArts(std::vector<ConstTransitivePtr<CArtifact> > &out, std::vector<CArtifact*> *arts, int flag);
	void getAllowed(std::vector<ConstTransitivePtr<CArtifact> > &out, int flags);
	void erasePickedArt (TArtifactInstanceID id);
	bool isBigArtifact (TArtifactID artID) const {return bigArtifacts.find(artID) != bigArtifacts.end();}
	void initAllowedArtifactsList(const std::vector<ui8> &allowed); //allowed[art_id] -> 0 if not allowed, 1 if allowed
	static int convertMachineID(int id, bool creToArt);
	void makeItCreatureArt (TArtifactInstanceID aid, bool onlyCreature = true);
	void makeItCommanderArt (TArtifactInstanceID aid, bool onlyCommander = true);
	CArtHandler();
	~CArtHandler();

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
	bmap<ui16, ArtSlotInfo> artifactsWorn; //map<position,artifact_id>; positions: 0 - head; 1 - shoulders; 2 - neck; 3 - right hand; 4 - left hand; 5 - torso; 6 - right ring; 7 - left ring; 8 - feet; 9 - misc1; 10 - misc2; 11 - misc3; 12 - misc4; 13 - mach1; 14 - mach2; 15 - mach3; 16 - mach4; 17 - spellbook; 18 - misc5

	ArtSlotInfo &retreiveNewArtSlot(ui16 slot);
	void setNewArtSlot(ui16 slot, CArtifactInstance *art, bool locked);
	void eraseArtSlot(ui16 slot);

	const ArtSlotInfo *getSlot(ui16 pos) const;
	const CArtifactInstance* getArt(ui16 pos, bool excludeLocked = true) const; //NULL - no artifact
	CArtifactInstance* getArt(ui16 pos, bool excludeLocked = true); //NULL - no artifact
	si32 getArtPos(int aid, bool onlyWorn = true) const; //looks for equipped artifact with given ID and returns its slot ID or -1 if none(if more than one such artifact lower ID is returned)
	si32 getArtPos(const CArtifactInstance *art) const;
	const CArtifactInstance *getArtByInstanceId(TArtifactInstanceID artInstId) const;
	bool hasArt(ui32 aid, bool onlyWorn = false) const; //checks if hero possess artifact of given id (either in backack or worn)
	bool isPositionFree(ui16 pos, bool onlyLockCheck = false) const;
	si32 getArtTypeId(ui16 pos) const;

	virtual ui8 bearerType() const = 0;
	virtual ~CArtifactSet();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & artifactsInBackpack & artifactsWorn;
	}
	
	void artDeserializationFix(CBonusSystemNode *node);
};
