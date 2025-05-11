/*
 * IGameCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CPrivilegedInfoCallback.h"
#include "IGameEventCallback.h"

#if SCRIPTING_ENABLED
namespace scripting
{
	class Pool;
}
#endif

/// Interface class for handling general game logic and actions
class DLL_LINKAGE IGameCallback : public CPrivilegedInfoCallback, public IGameEventCallback
{
public:
	virtual ~IGameCallback(){};

#if SCRIPTING_ENABLED
	virtual scripting::Pool * getGlobalContextPool() const = 0;
#endif

	//get info
	virtual bool isVisitCoveredByAnotherQuery(const CGObjectInstance *obj, const CGHeroInstance *hero) { return false; }

	friend struct CPack;
	friend struct CPackForClient;
	friend struct CPackForServer;
};


VCMI_LIB_NAMESPACE_END
