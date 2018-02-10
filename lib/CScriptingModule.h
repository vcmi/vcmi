/*
 * CScriptingModule.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "IGameEventsReceiver.h"

class IGameEventRealizer;
class CPrivilegedInfoCallback;

class CScriptingModule : public IGameEventsReceiver, public IBattleEventsReceiver
{
public:
	virtual void executeUserCommand(const std::string &cmd){};
	virtual void init(){}; //called upon the start of game (after map randomization, before first turn)
	virtual void giveActionCB(IGameEventRealizer *cb){}; 
	virtual void giveInfoCB(CPrivilegedInfoCallback *cb){};

	CScriptingModule(){}
	virtual ~CScriptingModule(){}
};

