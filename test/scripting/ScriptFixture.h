/*
 * ScriptFixture.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vstd/RNG.h>

#include <vcmi/events/EventBus.h>

#include "../../lib/JsonNode.h"
#include "../../lib/HeroBonus.h"
#include "../../lib/ScriptHandler.h"
#include "../../lib/NetPacksBase.h"
#include "../../lib/battle/CBattleInfoCallback.h"

#include "../mock/mock_ServerCallback.h"
#include "../mock/mock_IBattleInfoCallback.h"
#include "../mock/mock_IGameInfoCallback.h"
#include "../mock/mock_battle_IBattleState.h"
#if SCRIPTING_ENABLED
#include "../mock/mock_scripting_Pool.h"
#endif
#include "../mock/mock_Environment.h"
#include "../mock/mock_Services.h"
#include "../mock/mock_vstd_CLoggerBase.h"
#include "../mock/BattleFake.h"

#include "../JsonComparer.h"

namespace test
{

using namespace ::testing;
using namespace ::scripting;
using ::events::EventBus;

class ScriptFixture
{
public:
	EventBus eventBus;

	std::shared_ptr<PoolMock> pool;

	std::shared_ptr<ScriptImpl> subject;
	std::shared_ptr<Context> context;

	battle::UnitsFake unitsFake;

	StrictMock<EnvironmentMock> environmentMock;

	StrictMock<IBattleInfoCallbackMock> binfoMock;
	StrictMock<IGameInfoCallbackMock> infoMock;
	StrictMock<ServerCallbackMock> serverMock;
	StrictMock<ServicesMock> servicesMock;
	LoggerMock loggerMock;

	ScriptFixture();
	virtual ~ScriptFixture();

	void loadScriptFromFile(const std::string & path);
	void loadScript(const JsonNode & scriptConfig);
	void loadScript(ModulePtr module, const std::string & scriptSource);
	void loadScript(ModulePtr module, const std::vector<std::string> & scriptSource);

	JsonNode runClientServer(const JsonNode & scriptState = JsonNode());
	JsonNode runServer(const JsonNode & scriptState = JsonNode());

	JsonNode runScript(ModulePtr module, const std::string & scriptSource, const JsonNode & scriptState = JsonNode());

protected:
	void setUp();

private:
};


}
