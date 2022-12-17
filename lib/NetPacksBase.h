/*
 * NetPacksBase.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <vcmi/Metatype.h>

#include "ConstTransitivePtr.h"
#include "GameConstants.h"
#include "JsonNode.h"

class CClient;
class CGameHandler;

VCMI_LIB_NAMESPACE_BEGIN

class CGameState;
class CConnection;
class CStackBasicDescriptor;
class CGHeroInstance;
class CStackInstance;
class CArmedInstance;
class CArtifactSet;
class CBonusSystemNode;
struct ArtSlotInfo;

struct DLL_LINKAGE CPack
{
	std::shared_ptr<CConnection> c; // Pointer to connection that pack received from

	CPack() : c(nullptr) {};
	virtual ~CPack() {};

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		logNetwork->error("CPack serialized... this should not happen!");
		assert(false && "CPack serialized");
	}

	void applyGs(CGameState * gs)
	{}
};

struct CPackForClient : public CPack
{
	CPackForClient(){};

	CGameState* GS(CClient *cl);
	void applyFirstCl(CClient *cl)//called before applying to gs
	{}
	void applyCl(CClient *cl)//called after applying to gs
	{}
};

struct CPackForServer : public CPack
{
	mutable PlayerColor player;
	mutable si32 requestID;
	CGameState* GS(CGameHandler *gh);
	CPackForServer():
		player(PlayerColor::NEUTRAL)
	{
	}

	bool applyGh(CGameHandler *gh) //called after applying to gs
	{
		logGlobal->error("Should not happen... applying plain CPackForServer");
		return false;
	}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player;
		h & requestID;
	}

protected:
	void throwNotAllowedAction();
	void throwOnWrongOwner(CGameHandler * gh, ObjectInstanceID id);
	void throwOnWrongPlayer(CGameHandler * gh, PlayerColor player);
	void throwAndComplain(CGameHandler * gh, std::string txt);
	bool isPlayerOwns(CGameHandler * gh, ObjectInstanceID id);

private:
	void wrongPlayerMessage(CGameHandler * gh, PlayerColor expectedplayer);
};

struct DLL_LINKAGE MetaString
{
private:
	enum EMessage {TEXACT_STRING, TLOCAL_STRING, TNUMBER, TREPLACE_ESTRING, TREPLACE_LSTRING, TREPLACE_NUMBER, TREPLACE_PLUSNUMBER};
public:
	enum {GENERAL_TXT=1, XTRAINFO_TXT, OBJ_NAMES, RES_NAMES, ART_NAMES, ARRAY_TXT, CRE_PL_NAMES, CREGENS, MINE_NAMES,
		MINE_EVNTS, ADVOB_TXT, ART_EVNTS, SPELL_NAME, SEC_SKILL_NAME, CRE_SING_NAMES, CREGENS4, COLOR, ART_DESCR, JK_TXT};

	std::vector<ui8> message; //vector of EMessage

	std::vector<std::pair<ui8,ui32> > localStrings;
	std::vector<std::string> exactStrings;
	std::vector<int64_t> numbers;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & exactStrings;
		h & localStrings;
		h & message;
		h & numbers;
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
	MetaString& operator<<(int64_t txt)
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
	void addReplacement(int64_t txt)
	{
		message.push_back(TREPLACE_NUMBER);
		numbers.push_back(txt);
	}
	void addReplacement2(int64_t txt)
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
	std::string toString() const;
	void getLocalString(const std::pair<ui8,ui32> &txt, std::string &dst) const;

	MetaString(){}
};

struct Component
{
	enum EComponentType {PRIM_SKILL, SEC_SKILL, RESOURCE, CREATURE, ARTIFACT, EXPERIENCE, SPELL, MORALE, LUCK, BUILDING, HERO_PORTRAIT, FLAG};
	ui16 id, subtype; //id uses ^^^ enums, when id==EXPPERIENCE subtype==0 means exp points and subtype==1 levels)
	si32 val; // + give; - take
	si16 when; // 0 - now; +x - within x days; -x - per x days

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id;
		h & subtype;
		h & val;
		h & when;
	}
	Component()
		:id(0), subtype(0), val(0), when(0)
	{
	}
	DLL_LINKAGE explicit Component(const CStackBasicDescriptor &stack);
	Component(Component::EComponentType Type, ui16 Subtype, si32 Val, si16 When)
		:id(Type),subtype(Subtype),val(Val),when(When)
	{
	}
};

typedef boost::variant<ConstTransitivePtr<CGHeroInstance>, ConstTransitivePtr<CStackInstance> > TArtHolder;

struct ArtifactLocation
{
	TArtHolder artHolder;//TODO: identify holder by id
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
		h & artHolder;
		h & slot;
	}
};

class EntityChanges
{
public:
	Metatype metatype;
	int32_t entityIndex;
	JsonNode data;
	EntityChanges()
		: metatype(Metatype::UNKNOWN),
		entityIndex(0),
		data()
	{
	}
	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & metatype;
		h & entityIndex;
		h & data;
	}
};

class BattleChanges
{
public:
	enum class EOperation : si8
	{
		ADD,
		RESET_STATE,
		UPDATE,
		REMOVE,
		ACTIVATE_AND_UPDATE,
		ACTIVATE_AND_REMOVE
	};

	JsonNode data;
	EOperation operation;

	BattleChanges()
		: operation(EOperation::RESET_STATE),
		data()
	{
	}

	BattleChanges(EOperation operation_)
		: operation(operation_),
		data()
	{
	}
};

class UnitChanges : public BattleChanges
{
public:
	uint32_t id;
	int64_t healthDelta;

	UnitChanges()
		: BattleChanges(EOperation::RESET_STATE),
		id(0),
		healthDelta(0)
	{
	}

	UnitChanges(uint32_t id_, EOperation operation_)
		: BattleChanges(operation_),
		id(id_),
		healthDelta(0)
	{
	}

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & id;
		h & healthDelta;
		h & data;
		h & operation;
	}
};

class ObstacleChanges : public BattleChanges
{
public:
	uint32_t id;

	ObstacleChanges()
		: BattleChanges(EOperation::RESET_STATE),
		id(0)
	{
	}

	ObstacleChanges(uint32_t id_, EOperation operation_)
		: BattleChanges(operation_),
		id(id_)
	{
	}

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & id;
		h & data;
		h & operation;
	}
};

VCMI_LIB_NAMESPACE_END
