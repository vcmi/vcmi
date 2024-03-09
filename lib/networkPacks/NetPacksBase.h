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

VCMI_LIB_NAMESPACE_BEGIN

class CGameState;
class CConnection;

class ICPackVisitor;

struct DLL_LINKAGE CPack
{
	/// Pointer to connection that pack received from
	/// Only set & used on server
	std::shared_ptr<CConnection> c;

	CPack() = default;
	virtual ~CPack() = default;

	template <typename Handler> void serialize(Handler &h)
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
	void visitBasic(ICPackVisitor & cpackVisitor) override;
};

struct DLL_LINKAGE Query : public CPackForClient
{
	QueryID queryID; // equals to -1 if it is not an actual query (and should not be answered)
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
