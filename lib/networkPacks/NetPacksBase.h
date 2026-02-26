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

#include "../constants/EntityIdentifiers.h"
#include "../serializer/Serializeable.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGameState;
class ICPackVisitor;

struct DLL_LINKAGE CPack : public Serializeable
{
	CPack() = default;
	virtual ~CPack() = default;

	template <typename Handler> void serialize(Handler &h)
	{
		logNetwork->error("CPack serialized... this should not happen!");
		throw std::runtime_error("CPack serialized... this should not happen!");
	}

	void visit(ICPackVisitor & cpackVisitor);

protected:
	/// For basic types of netpacks hierarchy like CPackForClient. Called first.
	virtual void visitBasic(ICPackVisitor & cpackVisitor);

	/// For leaf types of netpacks hierarchy. Called after visitBasic.
	virtual void visitTyped(ICPackVisitor & cpackVisitor);
};

struct DLL_LINKAGE CPackForClient : public CPack
{
protected:
	void visitBasic(ICPackVisitor & cpackVisitor) override;
};

struct DLL_LINKAGE Query : public CPackForClient
{
	QueryID queryID; // equals to -1 if it is not an actual query (and should not be answered)
};

struct PackForClientBattle : public CPackForClient
{
	BattleID battleID;

	void visitTyped(ICPackVisitor & visitor) override;
};

struct DLL_LINKAGE CPackForServer : public CPack
{
	mutable PlayerColor player = PlayerColor::NEUTRAL;
	mutable uint32_t requestID = 0;

	template <typename Handler> void serialize(Handler &h)
	{
		h & player;
		h & requestID;
	}

protected:
	void visitBasic(ICPackVisitor & cpackVisitor) override;
};

struct DLL_LINKAGE CPackForLobby : public CPack
{
	virtual bool isForServer() const;

protected:
	void visitBasic(ICPackVisitor & cpackVisitor) override;
};

VCMI_LIB_NAMESPACE_END
