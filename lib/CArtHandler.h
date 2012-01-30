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
class CStackInstance;
//class CCreatureArtifactSet;
struct ArtifactLocation;

namespace ArtifactPosition
{
	enum ArtifactPosition
	{
		PRE_FIRST = -1, 
		HEAD, SHOULDERS, NECK, RIGHT_HAND, LEFT_HAND, TORSO, RIGHT_RING, LEFT_RING, FEET, MISC1, MISC2, MISC3, MISC4,
		MACH1, MACH2, MACH3, MACH4, SPELLBOOK, MISC5, 
		AFTER_LAST
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

	int getArtClassSerial() const; //0 - treasure, 1 - minor, 2 - major, 3 - relic, 4 - spell scroll, 5 - other
	std::string nodeName() const OVERRIDE;

	ui32 price;
	std::vector<ui16> possibleSlots; //ids of slots where artifact can be placed
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

class DLL_LINKAGE CArtifactInstance : public CBonusSystemNode
{
protected:
	void init();
	CArtifactInstance(CArtifact *Art);
public:
	CArtifactInstance();

	ConstTransitivePtr<CArtifact> artType; 
	si32 id; //id of the instance

	//CArtifactInstance(int aid);

	virtual std::string nodeName() const OVERRIDE;
	void deserializationFix();
	void setType(CArtifact *Art);

	int firstAvailableSlot(const CGHeroInstance *h) const;
	int firstBackpackSlot(const CGHeroInstance *h) const;
	int getGivenSpellID() const; //to be used with scrolls (and similar arts), -1 if none

	virtual bool canBePutAt(const ArtifactLocation &al, bool assumeDestRemoved = false) const;
	virtual bool canBeDisassembled() const;
	virtual void putAt(CGHeroInstance *h, ui16 slot);
	virtual void removeFrom(CGHeroInstance *h, ui16 slot);
	virtual void putAt(CStackInstance *s, ui16 slot);
	virtual void removeFrom(CStackInstance *s, ui16 slot);
	virtual bool isPart(const CArtifactInstance *supposedPart) const; //checks if this a part of this artifact: artifact instance is a part of itself, additionally truth is returned for consituents of combined arts

	std::vector<const CArtifact *> assemblyPossibilities(const CGHeroInstance *h) const;
	void move(ArtifactLocation &src, ArtifactLocation &dst);

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

	bool canBePutAt(const ArtifactLocation &al, bool assumeDestRemoved = false) const OVERRIDE;
	bool canBeDisassembled() const OVERRIDE;
	void putAt(CGHeroInstance *h, ui16 slot) OVERRIDE;
	void removeFrom(CGHeroInstance *h, ui16 slot) OVERRIDE;
	bool isPart(const CArtifactInstance *supposedPart) const OVERRIDE;

	void createConstituents();
	void addAsConstituent(CArtifactInstance *art, int slot);
	CArtifactInstance *figureMainConstituent(ui16 slot); //main constituent is replcaed with us (combined art), not lock

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

class DLL_LINKAGE CCreatureArtifactInstance : public CArtifactInstance
{
	CCreatureArtifactInstance(CArtifact *Art);
public:

	bool canBePutAt(const ArtifactLocation &al, bool assumeDestRemoved = false) const OVERRIDE;
	void putAt(CStackInstance *s, ui16 slot) OVERRIDE;
	void removeFrom(CStackInstance *s, ui16 slot) OVERRIDE;
	bool isPart(const CArtifactInstance *supposedPart) const OVERRIDE;

	std::string nodeName() const OVERRIDE;

	CCreatureArtifactInstance();

	//void deserializationFix(); ..inherit from CArtifactInstance

	friend class CArtifactInstance;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArtifactInstance&>(*this);
		BONUS_TREE_DESERIALIZATION_FIX
	}
};

// class DLL_LINKAGE IModableArt : public CArtifact //artifact which can have different properties, such as scroll or banner
// { //used only for dynamic cast :P
// public:
// 	si32 ID; //used for smart serialization
// 
// 	template <typename Handler> void serialize(Handler &h, const int version)
// 	{
// 		h & static_cast<CArtifact&>(*this);
// 		h & ID;
// 	}
// };
// 
// class DLL_LINKAGE CScroll : public IModableArt // Spell Scroll
// {
// public:
// 	CScroll(){spellid=0;};
// 	CScroll(spelltype sid){spellid = sid;};
// 	spelltype spellid;
// 	void Init();
// 	void SetProperty (int mod){spellid = mod;};
// 	template <typename Handler> void serialize(Handler &h, const int version)
// 	{
// 		h & static_cast<IModableArt&>(*this);
// 		h & spellid;
// 	}
// };
// 
// class DLL_LINKAGE CCustomizableArt : public IModableArt // Warlord's Banner with multiple options
// {
// public:
// 	ui8 mode;
// 	CCustomizableArt(){mode=0;};
// 	void Init(){};
// 	void SetProperty (int mod){};
// 	template <typename Handler> void serialize(Handler &h, const int version)
// 	{
// 		h & static_cast<IModableArt&>(*this);
// 		h & mode;
// 	}
// };
// 
// class DLL_LINKAGE CCommanderArt : public IModableArt // Growing with time
// {
// public:
// 	ui32 level;
// 	CCommanderArt(){level = 0;};
// 	void Init(){};
// 	void SetProperty (int mod){level = mod;};
// 	void Upgrade(){level++;};
// 	template <typename Handler> void serialize(Handler &h, const int version)
// 	{
// 		h & static_cast<IModableArt&>(*this);
// 		h & level;
// 	}
// };

class DLL_LINKAGE CArtHandler //handles artifacts
{
	void giveArtBonus(int aid, Bonus::BonusType type, int val, int subtype = -1, int valType = Bonus::BASE_NUMBER, ILimiter * limiter = NULL);
	void giveArtBonus(int aid, Bonus::BonusType type, int val, int subtype, IPropagator* propagator);
public:
	std::vector<CArtifact*> treasures, minors, majors, relics;
	std::vector< ConstTransitivePtr<CArtifact> > artifacts;
	std::vector<CArtifact *> allowedArtifacts;
	std::set<ui32> bigArtifacts; // Artifacts that cannot be moved to backpack, e.g. war machines.
	std::set<ui32> creatureArtifacts; // can be held by Stacks

	void loadArtifacts(bool onlyTxt);
	void sortArts();
	void addBonuses();
	void clear();
	void clearHlpLists();
	ui16 getRandomArt (int flags);
	ui16 getArtSync (ui32 rand, int flags);
	void getAllowedArts(std::vector<ConstTransitivePtr<CArtifact> > &out, std::vector<CArtifact*> *arts, int flag);
	void getAllowed(std::vector<ConstTransitivePtr<CArtifact> > &out, int flags);
	void erasePickedArt (si32 id);
	bool isBigArtifact (ui32 artID) const {return bigArtifacts.find(artID) != bigArtifacts.end();}
// 	void equipArtifact (std::map<ui16, const CArtifact*> &artifWorn, ui16 slotID, const CArtifact* art) const;
// 	void unequipArtifact (std::map<ui16, const CArtifact*> &artifWorn, ui16 slotID) const;
	void initAllowedArtifactsList(const std::vector<ui8> &allowed); //allowed[art_id] -> 0 if not allowed, 1 if allowed
	static int convertMachineID(int id, bool creToArt);
	CArtHandler();
	~CArtHandler();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & artifacts & allowedArtifacts & treasures & minors & majors & relics;
		h & creatureArtifacts;
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

class DLL_LINKAGE IArtifactSetBase 
{ ///artifacts container
public:
	virtual void setNewArtSlot(ui16 slot, CArtifactInstance *art, bool locked);
	virtual const CArtifactInstance* getArt(ui16 pos, bool excludeLocked = true) const;
	virtual CArtifactInstance* getArt(ui16 pos, bool excludeLocked = true); //NULL - no artifact
	virtual bool hasArt(ui32 aid, bool onlyWorn = false) const;
	virtual bool isPositionFree(ui16 pos, bool onlyLockCheck = false) const;

	virtual ArtSlotInfo &retreiveNewArtSlot(ui16 slot)=0;
	virtual void eraseArtSlot(ui16 slot)=0;

	virtual const ArtSlotInfo *getSlot(ui16 pos) const=0;
	virtual si32 getArtPos(int aid, bool onlyWorn = true) const=0; //looks for equipped artifact with given ID and returns its slot ID or -1 if none(if more than one such artifact lower ID is returned)
	virtual si32 getArtPos(const CArtifactInstance *art) const=0;
	virtual const CArtifactInstance *getArtByInstanceId(int artInstId) const=0;
	virtual si32 getArtTypeId(ui16 pos) const=0;
};

class DLL_LINKAGE CArtifactSet : public IArtifactSetBase
{ ///hero artifacts
public:
	std::vector<ArtSlotInfo> artifactsInBackpack; //hero's artifacts from bag
	bmap<ui16, ArtSlotInfo> artifactsWorn; //map<position,artifact_id>; positions: 0 - head; 1 - shoulders; 2 - neck; 3 - right hand; 4 - left hand; 5 - torso; 6 - right ring; 7 - left ring; 8 - feet; 9 - misc1; 10 - misc2; 11 - misc3; 12 - misc4; 13 - mach1; 14 - mach2; 15 - mach3; 16 - mach4; 17 - spellbook; 18 - misc5

	ArtSlotInfo &retreiveNewArtSlot(ui16 slot);
	void eraseArtSlot(ui16 slot);

	const ArtSlotInfo *getSlot(ui16 pos) const;
	si32 getArtPos(int aid, bool onlyWorn = true) const;
	si32 getArtPos(const CArtifactInstance *art) const;
	const CArtifactInstance *getArtByInstanceId(int artInstId) const;
	si32 getArtTypeId(ui16 pos) const;

	virtual ~CArtifactSet();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & artifactsInBackpack & artifactsWorn;
	}
};

class DLL_LINKAGE CCreatureArtifactSet : public IArtifactSetBase
{ ///creature artifacts
public:
	std::vector<ArtSlotInfo> artifactsInBackpack; //artifacts carried by creature - 4 max (according to WoG)
	ArtSlotInfo activeArtifact; //position 0 - Arts::CREATURE_ART

	ArtSlotInfo &retreiveNewArtSlot(ui16 slot);
	void eraseArtSlot(ui16 slot);

	const ArtSlotInfo *getSlot(ui16 pos)const;
	si32 getArtPos(int aid, bool onlyWorn = true) const;
	si32 getArtPos(const CArtifactInstance *art) const;
	const CArtifactInstance *getArtByInstanceId(int artInstId) const;
	si32 getArtTypeId(ui16 pos) const;

	virtual ~CCreatureArtifactSet(){};

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & artifactsInBackpack & activeArtifact;
	}
};
