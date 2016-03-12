#pragma once

/*
 * NetPacksBase.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CGameState;
class CStackBasicDescriptor;
class CGHeroInstance;
class CStackInstance;
class CArmedInstance;
class CArtifactSet;
class CBonusSystemNode;
struct ArtSlotInfo;



#include "ConstTransitivePtr.h"
#include "GameConstants.h"


struct DLL_LINKAGE CPack
{
	ui16 type;

	CPack(){};
	virtual ~CPack(){};
	ui16 getType() const{return type;}
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		logNetwork->errorStream() << "CPack serialized... this should not happen!";
	}
	void applyGs(CGameState *gs) { }
	virtual std::string toString() const { return boost::str(boost::format("{CPack: type '%d'}") % type); }
};

std::ostream & operator<<(std::ostream & out, const CPack * pack);


struct DLL_LINKAGE MetaString : public CPack //2001 helper for object scrips
{
private:
	enum EMessage {TEXACT_STRING, TLOCAL_STRING, TNUMBER, TREPLACE_ESTRING, TREPLACE_LSTRING, TREPLACE_NUMBER, TREPLACE_PLUSNUMBER};
public:
	enum {GENERAL_TXT=1, XTRAINFO_TXT, OBJ_NAMES, RES_NAMES, ART_NAMES, ARRAY_TXT, CRE_PL_NAMES, CREGENS, MINE_NAMES,
		MINE_EVNTS, ADVOB_TXT, ART_EVNTS, SPELL_NAME, SEC_SKILL_NAME, CRE_SING_NAMES, CREGENS4, COLOR, ART_DESCR};

	std::vector<ui8> message; //vector of EMessage

	std::vector<std::pair<ui8,ui32> > localStrings; //pairs<text handler type, text number>; types: 1 - generaltexthandler->all; 2 - objh->xtrainfo; 3 - objh->names; 4 - objh->restypes; 5 - arth->artifacts[id].name; 6 - generaltexth->arraytxt; 7 - creh->creatures[os->subID].namePl; 8 - objh->creGens; 9 - objh->mines[ID]->first; 10 - objh->mines[ID]->second; 11 - objh->advobtxt
	std::vector<std::string> exactStrings;
	std::vector<si32> numbers;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & exactStrings & localStrings & message & numbers;
	}
	void addTxt(ui8 type, ui32 serial)
	{
		message.push_back(TLOCAL_STRING);
		localStrings.push_back(std::pair<ui8,ui32>(type, serial));
	}
	MetaString& operator<<(const std::pair<ui8,ui32> &txt)
	{
		message.push_back(TLOCAL_STRING);
		localStrings.push_back(txt);
		return *this;
	}
	MetaString& operator<<(const std::string &txt)
	{
		message.push_back(TEXACT_STRING);
		exactStrings.push_back(txt);
		return *this;
	}
	MetaString& operator<<(int txt)
	{
		message.push_back(TNUMBER);
		numbers.push_back(txt);
		return *this;
	}
	void addReplacement(ui8 type, ui32 serial)
	{
		message.push_back(TREPLACE_LSTRING);
		localStrings.push_back(std::pair<ui8,ui32>(type, serial));
	}
	void addReplacement(const std::string &txt)
	{
		message.push_back(TREPLACE_ESTRING);
		exactStrings.push_back(txt);
	}
	void addReplacement(int txt)
	{
		message.push_back(TREPLACE_NUMBER);
		numbers.push_back(txt);
	}
	void addReplacement2(int txt)
	{
		message.push_back(TREPLACE_PLUSNUMBER);
		numbers.push_back(txt);
	}
	void addCreReplacement(CreatureID id, TQuantity count); //adds sing or plural name;
	void addReplacement(const CStackBasicDescriptor &stack); //adds sing or plural name;
	std::string buildList () const;
	void clear()
	{
		exactStrings.clear();
		localStrings.clear();
		message.clear();
		numbers.clear();
	}
	void toString(std::string &dst) const;
	std::string toString() const override;
	void getLocalString(const std::pair<ui8,ui32> &txt, std::string &dst) const;

	MetaString()
	{
		type = 2001;
	}
};

struct Component : public CPack //2002 helper for object scrips informations
{
	enum EComponentType {PRIM_SKILL, SEC_SKILL, RESOURCE, CREATURE, ARTIFACT, EXPERIENCE, SPELL, MORALE, LUCK, BUILDING, HERO_PORTRAIT, FLAG};
	ui16 id, subtype; //id uses ^^^ enums, when id==EXPPERIENCE subtype==0 means exp points and subtype==1 levels)
	si32 val; // + give; - take
	si16 when; // 0 - now; +x - within x days; -x - per x days

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id & subtype & val & when;
	}
	Component()
	{
		type = 2002;
	}
	DLL_LINKAGE explicit Component(const CStackBasicDescriptor &stack);
	Component(Component::EComponentType Type, ui16 Subtype, si32 Val, si16 When)
		:id(Type),subtype(Subtype),val(Val),when(When)
	{
		type = 2002;
	}
};

typedef boost::variant<ConstTransitivePtr<CGHeroInstance>, ConstTransitivePtr<CStackInstance> > TArtHolder;

struct ArtifactLocation
{
	TArtHolder artHolder;
	ArtifactPosition slot;

	ArtifactLocation()
	{
		artHolder = ConstTransitivePtr<CGHeroInstance>();
		slot = ArtifactPosition::PRE_FIRST;
	}
	template <typename T>
	ArtifactLocation(const T *ArtHolder, ArtifactPosition Slot)
	{
		artHolder = const_cast<T*>(ArtHolder); //we are allowed here to const cast -> change will go through one of our packages... do not abuse!
		slot = Slot;
	}
	ArtifactLocation(TArtHolder ArtHolder, ArtifactPosition Slot)
	{
		artHolder = ArtHolder;
		slot = Slot;
	}

	template <typename T>
	bool isHolder(const T *t) const
	{
		if(auto ptrToT = boost::get<ConstTransitivePtr<T> >(&artHolder))
		{
			return ptrToT->get() == t;
		}
		return false;
	}

	DLL_LINKAGE void removeArtifact(); // BE CAREFUL, this operation modifies holder (gs)

	DLL_LINKAGE const CArmedInstance *relatedObj() const; //hero or the stack owner
	DLL_LINKAGE PlayerColor owningPlayer() const;
	DLL_LINKAGE CArtifactSet *getHolderArtSet();
	DLL_LINKAGE CBonusSystemNode *getHolderNode();
	DLL_LINKAGE const CArtifactSet *getHolderArtSet() const;
	DLL_LINKAGE const CBonusSystemNode *getHolderNode() const;

	DLL_LINKAGE const CArtifactInstance *getArt() const;
	DLL_LINKAGE CArtifactInstance *getArt();
	DLL_LINKAGE const ArtSlotInfo *getSlot() const;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & artHolder & slot;
	}
};
