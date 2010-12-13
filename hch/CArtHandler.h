#ifndef __CARTHANDLER_H__
#define __CARTHANDLER_H__
#include "../global.h"
#include "../lib/HeroBonus.h"
#include <set>
#include <list>
#include <string>
#include <vector>

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

class DLL_EXPORT CArtifact : public CBonusSystemNode //container for artifacts
{
protected:
	std::string name, description; //set if custom
public:
	enum EartClass {ART_SPECIAL=1, ART_TREASURE=2, ART_MINOR=4, ART_MAJOR=8, ART_RELIC=16}; //artifact classes
	const std::string &Name() const; //getter
	const std::string &Description() const; //getter
	bool isBig () const;
	bool isModable () const;
	bool fitsAt (const std::map<ui16, const CArtifact*> &artifWorn, ui16 slot) const;
	bool canBeAssembledTo (const std::map<ui16, const CArtifact*> &artifWorn, ui32 artifactID) const;
	void addBonusesTo (BonusList *otherBonuses) const;
	void removeBonusesFrom (BonusList *otherBonuses) const;
	virtual void SetProperty (int mod){};
	virtual void Init(){};
	int getArtClassSerial() const; //0 - treasure, 1 - minor, 2 - major, 3 - relic, 4 - spell scroll, 5 - other

	ui32 price;
	std::vector<ui16> possibleSlots; //ids of slots where artifact can be placed
	std::vector<ui32> * constituents; // Artifacts IDs a combined artifact consists of, or NULL.
	std::vector<ui32> * constituentOf; // Reverse map of constituents.
	EartClass aClass;
	si32 id;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CBonusSystemNode&>(*this);;
		h & name & description & price & possibleSlots & constituents & constituentOf & aClass & id;
	}

	CArtifact();
	~CArtifact();

	//override
	void getParents(TCNodes &out, const CBonusSystemNode *root = NULL) const;
};

class DLL_EXPORT IModableArt : public CArtifact //artifact which can have different properties, such as scroll or banner
{ //used only for dynamic cast :P
public:
	si32 ID; //used for smart serialization

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArtifact&>(*this);
		h & ID;
	}
};

class DLL_EXPORT CScroll : public IModableArt // Spell Scroll
{
public:
	CScroll(){spellid=0;};
	CScroll(spelltype sid){spellid = sid;};
	spelltype spellid;
	void Init();
	void SetProperty (int mod){spellid = mod;};
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<IModableArt&>(*this);
		h & spellid;
	}
};

class DLL_EXPORT CCustomizableArt : public IModableArt // Warlord's Banner with multiple options
{
public:
	ui8 mode;
	CCustomizableArt(){mode=0;};
	void Init(){};
	void SetProperty (int mod){};
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<IModableArt&>(*this);
		h & mode;
	}
};

class DLL_EXPORT CCommanderArt : public IModableArt // Growing with time
{
public:
	ui32 level;
	CCommanderArt(){level = 0;};
	void Init(){};
	void SetProperty (int mod){level = mod;};
	void Upgrade(){level++;};
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<IModableArt&>(*this);
		h & level;
	}
};

class DLL_EXPORT CArtHandler //handles artifacts
{
	void giveArtBonus(int aid, Bonus::BonusType type, int val, int subtype = -1, int valType = Bonus::BASE_NUMBER, ILimiter * limiter = NULL);
public:
	std::vector<CArtifact*> treasures, minors, majors, relics;
	std::vector<CArtifact *> artifacts;
	std::vector<CArtifact *> allowedArtifacts;
	std::set<ui32> bigArtifacts; // Artifacts that cannot be moved to backpack, e.g. war machines.
	std::map<ui32, ui8> modableArtifacts; //1-scroll, 2-banner, 3-commander art with progressive bonus

	void loadArtifacts(bool onlyTxt);
	void sortArts();
	void addBonuses();
	void clear();
	void clearHlpLists();
	ui16 getRandomArt (int flags);
	ui16 getArtSync (ui32 rand, int flags);
	void getAllowedArts(std::vector<CArtifact*> &out, std::vector<CArtifact*> *arts, int flag);
	void getAllowed(std::vector<CArtifact*> &out, int flags);
	void erasePickedArt (si32 id);
	bool isBigArtifact (ui32 artID) {return bigArtifacts.find(artID) != bigArtifacts.end();}
	void equipArtifact (std::map<ui16, const CArtifact*> &artifWorn, ui16 slotID, const CArtifact* art);
	void unequipArtifact (std::map<ui16, const CArtifact*> &artifWorn, ui16 slotID);
	void initAllowedArtifactsList(const std::vector<ui8> &allowed); //allowed[art_id] -> 0 if not allowed, 1 if allowed
	static int convertMachineID(int id, bool creToArt);
	CArtHandler();
	~CArtHandler();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & artifacts & allowedArtifacts & treasures & minors & majors & relics;
		//if(!h.saving) sortArts();
	}
};


#endif // __CARTHANDLER_H__
