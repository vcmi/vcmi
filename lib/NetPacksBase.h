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
class CLobbyScreen;
class CServerHandler;
class CVCMIServer;

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

class ICPackVisitor;

enum class EInfoWindowMode : uint8_t
{
	AUTO,
	MODAL,
	INFO
};

enum class EOpenWindowMode : uint8_t
{
	EXCHANGE_WINDOW,
	RECRUITMENT_FIRST,
	RECRUITMENT_ALL,
	SHIPYARD_WINDOW,
	THIEVES_GUILD,
	UNIVERSITY_WINDOW,
	HILL_FORT_WINDOW,
	MARKET_WINDOW,
	PUZZLE_MAP,
	TAVERN_WINDOW
};

struct DLL_LINKAGE CPack
{
	std::shared_ptr<CConnection> c; // Pointer to connection that pack received from

	CPack() = default;
	virtual ~CPack() = default;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		logNetwork->error("CPack serialized... this should not happen!");
		assert(false && "CPack serialized");
	}

	void applyGs(CGameState * gs)
	{}

	void visit(ICPackVisitor & cpackVisitor);

protected:
	/// <summary>
	/// For basic types of netpacks hierarchy like CPackForClient. Called first.
	/// </summary>
	virtual void visitBasic(ICPackVisitor & cpackVisitor);

	/// <summary>
	/// For leaf types of netpacks hierarchy. Called after visitBasic.
	/// </summary>
	virtual void visitTyped(ICPackVisitor & cpackVisitor);
};

struct DLL_LINKAGE CPackForClient : public CPack
{
protected:
	virtual void visitBasic(ICPackVisitor & cpackVisitor) override;
};

struct DLL_LINKAGE CPackForServer : public CPack
{
	mutable PlayerColor player = PlayerColor::NEUTRAL;
	mutable si32 requestID;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player;
		h & requestID;
	}

protected:
	virtual void visitBasic(ICPackVisitor & cpackVisitor) override;
};

struct DLL_LINKAGE CPackForLobby : public CPack
{
	virtual bool isForServer() const;

protected:
	virtual void visitBasic(ICPackVisitor & cpackVisitor) override;
};

struct DLL_LINKAGE MetaString
{
private:
	enum EMessage {TEXACT_STRING, TLOCAL_STRING, TNUMBER, TREPLACE_ESTRING, TREPLACE_LSTRING, TREPLACE_NUMBER, TREPLACE_PLUSNUMBER};
public:
	enum {GENERAL_TXT=1, OBJ_NAMES, RES_NAMES, ART_NAMES, ARRAY_TXT, CRE_PL_NAMES, CREGENS, MINE_NAMES,
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
		localStrings.emplace_back(type, serial);
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
		localStrings.emplace_back(type, serial);
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
	void addCreReplacement(const CreatureID & id, TQuantity count); //adds sing or plural name;
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
	void getLocalString(const std::pair<ui8, ui32> & txt, std::string & dst) const;

};

struct Component
{
	enum class EComponentType : uint8_t 
	{
		PRIM_SKILL,
		SEC_SKILL,
		RESOURCE,
		CREATURE,
		ARTIFACT,
		EXPERIENCE,
		SPELL,
		MORALE,
		LUCK,
		BUILDING,
		HERO_PORTRAIT,
		FLAG,
		INVALID //should be last
	};
	EComponentType id = EComponentType::INVALID;
	ui16 subtype = 0; //id==EXPPERIENCE subtype==0 means exp points and subtype==1 levels
	si32 val = 0; // + give; - take
	si16 when = 0; // 0 - now; +x - within x days; -x - per x days

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id;
		h & subtype;
		h & val;
		h & when;
	}
	Component() = default;
	DLL_LINKAGE explicit Component(const CStackBasicDescriptor &stack);
	Component(Component::EComponentType Type, ui16 Subtype, si32 Val, si16 When)
		:id(Type),subtype(Subtype),val(Val),when(When)
	{
	}
};

using TArtHolder = boost::variant<ConstTransitivePtr<CGHeroInstance>, ConstTransitivePtr<CStackInstance>>;

struct ArtifactLocation
{
	TArtHolder artHolder;//TODO: identify holder by id
	ArtifactPosition slot = ArtifactPosition::PRE_FIRST;

	ArtifactLocation()
		: artHolder(ConstTransitivePtr<CGHeroInstance>())
	{
	}
	template<typename T>
	ArtifactLocation(const T * ArtHolder, ArtifactPosition Slot)
		: artHolder(const_cast<T *>(ArtHolder)) //we are allowed here to const cast -> change will go through one of our packages... do not abuse!
		, slot(Slot)
	{
	}
	ArtifactLocation(TArtHolder ArtHolder, const ArtifactPosition & Slot)
		: artHolder(std::move(std::move(ArtHolder)))
		, slot(Slot)
	{
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
	Metatype metatype = Metatype::UNKNOWN;
	int32_t entityIndex = 0;
	JsonNode data;
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
	};

	JsonNode data;
	EOperation operation = EOperation::RESET_STATE;

	BattleChanges() = default;
	BattleChanges(EOperation operation_)
		: operation(operation_)
	{
	}
};

class UnitChanges : public BattleChanges
{
public:
	uint32_t id = 0;
	int64_t healthDelta = 0;

	UnitChanges() = default;
	UnitChanges(uint32_t id_, EOperation operation_)
		: BattleChanges(operation_)
		, id(id_)
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
	uint32_t id = 0;

	ObstacleChanges() = default;

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
