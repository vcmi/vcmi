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

#include "../ConstTransitivePtr.h"
#include "../GameConstants.h"

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

struct DLL_LINKAGE Query : public CPackForClient
{
	QueryID queryID; // equals to -1 if it is not an actual query (and should not be answered)
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

using TArtHolder = std::variant<ConstTransitivePtr<CGHeroInstance>, ConstTransitivePtr<CStackInstance>>;

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
		if(auto ptrToT = std::get<ConstTransitivePtr<T>>(artHolder))
		{
			return ptrToT == t;
		}
		return false;
	}

	DLL_LINKAGE void removeArtifact(); // BE CAREFUL, this operation modifies holder (gs)

	DLL_LINKAGE const CArmedInstance *relatedObj() const; //hero or the stack owner
	DLL_LINKAGE PlayerColor owningPlayer() const;
	DLL_LINKAGE CArtifactSet *getHolderArtSet();
	DLL_LINKAGE CBonusSystemNode *getHolderNode();
	DLL_LINKAGE CArtifactSet *getHolderArtSet() const;
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

VCMI_LIB_NAMESPACE_END
