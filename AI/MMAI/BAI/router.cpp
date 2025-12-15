/*
 * router.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "callback/CBattleCallback.h"
#include "callback/CDynLibHandler.h"
#include "callback/IGameInfoCallback.h"
#include "filesystem/Filesystem.h"
#include "json/JsonUtils.h"

#include "BAI/base.h"
#include "BAI/model/NNModel.h"
#include "BAI/model/ScriptedModel.h"
#include "BAI/router.h"

#include "common.h"

#include <utility>

namespace MMAI::BAI
{
using ConfigStorage = std::map<std::string, std::string>;
using ModelStorage = std::map<std::string, std::unique_ptr<NNModel>>;

namespace
{
	struct ModelRepository
	{
		ModelStorage models;
		float temperature = 1.0;
		uint64_t seed = 0;
		std::unique_ptr<ScriptedModel> fallbackModel;
		std::string fallbackName;
	};

	std::unique_ptr<ModelRepository> InitModelRepository()
	{
		auto repo = std::make_unique<ModelRepository>();
		auto json = JsonUtils::assembleFromFiles("MMAI/CONFIG/mmai-settings.json");
		if(!json.isStruct())
		{
			logAi->error("Could not load MMAI config. Is MMAI mod enabled?");
			return repo;
		}

		JsonUtils::validate(json, "vcmi:mmaiSettings", "mmai");
		repo->temperature = static_cast<float>(json["temperature"].Float());
		repo->seed = json["seed"].Integer();
		for(const std::string key : {"attacker", "defender"})
		{
			std::string value = "MMAI/models/" + json["models"][key].String();
			logAi->debug("MMAI: Loading NN %s model from: %s", key, value);
			try
			{
				repo->models.try_emplace(key, std::make_unique<NNModel>(value, repo->temperature, repo->seed));
			}
			catch(std::exception & e)
			{
				logAi->error("MMAI: error loading " + key + ": " + std::string(e.what()));
			}
		}

		auto fallback = json["fallback"].String();
		logAi->debug("MMAI: preparing fallback model: %s", fallback);
		repo->fallbackModel = std::make_unique<ScriptedModel>(fallback);
		repo->fallbackName = fallback;

		return repo;
	}

	Schema::IModel * GetModel(const std::string & key)
	{
		static const auto MODEL_REPO = InitModelRepository();
		auto it = MODEL_REPO->models.find(key);
		if(it == MODEL_REPO->models.end())
		{
			logAi->error("MMAI: no %s model loaded, trying fallback: %s", key, MODEL_REPO->fallbackName);
			ASSERT(MODEL_REPO->fallbackModel, "fallback failed: model is null");
			return MODEL_REPO->fallbackModel.get();
		}

		return it->second.get();
	}
}

Router::Router()
{
	std::ostringstream oss;
	// Store the memory address and include it in logging
	const auto * ptr = static_cast<const void *>(this);
	oss << ptr;
	addrstr = oss.str();
	info("+++ constructor +++"); // log after addrstr is set
}

Router::~Router()
{
	info("--- destructor ---");
	cb->waitTillRealize = wasWaitingForRealize;
}

void Router::initBattleInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB)
{
	info("*** initBattleInterface ***");
	env = ENV;
	cb = CB;
	colorname = cb->getPlayerID()->toString();
	wasWaitingForRealize = cb->waitTillRealize;

	cb->waitTillRealize = false;
	bai.reset();
}

void Router::initBattleInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB, AutocombatPreferences prefs)
{
	autocombatPreferences = prefs;
	initBattleInterface(ENV, CB);
}

/*
 * Delegated methods
 */

void Router::actionFinished(const BattleID & bid, const BattleAction & action)
{
	bai->actionFinished(bid, action);
}

void Router::actionStarted(const BattleID & bid, const BattleAction & action)
{
	bai->actionStarted(bid, action);
}

void Router::activeStack(const BattleID & bid, const CStack * astack)
{
	bai->activeStack(bid, astack);
}

void Router::battleAttack(const BattleID & bid, const BattleAttack * ba)
{
	bai->battleAttack(bid, ba);
}

void Router::battleCatapultAttacked(const BattleID & bid, const CatapultAttack & ca)
{
	bai->battleCatapultAttacked(bid, ca);
}

void Router::battleEnd(const BattleID & bid, const BattleResult * br, QueryID queryID)
{
	bai->battleEnd(bid, br, queryID);
}

void Router::battleGateStateChanged(const BattleID & bid, const EGateState state)
{
	bai->battleGateStateChanged(bid, state);
};

void Router::battleLogMessage(const BattleID & bid, const std::vector<MetaString> & lines)
{
	bai->battleLogMessage(bid, lines);
};

void Router::battleNewRound(const BattleID & bid)
{
	bai->battleNewRound(bid);
}

void Router::battleNewRoundFirst(const BattleID & bid)
{
	bai->battleNewRoundFirst(bid);
}

void Router::battleObstaclesChanged(const BattleID & bid, const std::vector<ObstacleChanges> & obstacles)
{
	bai->battleObstaclesChanged(bid, obstacles);
};

void Router::battleSpellCast(const BattleID & bid, const BattleSpellCast * sc)
{
	bai->battleSpellCast(bid, sc);
}

void Router::battleStackMoved(const BattleID & bid, const CStack * stack, const BattleHexArray & dest, int distance, bool teleport)
{
	bai->battleStackMoved(bid, stack, dest, distance, teleport);
}

void Router::battleStacksAttacked(const BattleID & bid, const std::vector<BattleStackAttacked> & bsa, bool ranged)
{
	bai->battleStacksAttacked(bid, bsa, ranged);
}

void Router::battleStacksEffectsSet(const BattleID & bid, const SetStackEffect & sse)
{
	bai->battleStacksEffectsSet(bid, sse);
}

void Router::battleStart(
	const BattleID & bid,
	const CCreatureSet * army1,
	const CCreatureSet * army2,
	int3 tile,
	const CGHeroInstance * hero1,
	const CGHeroInstance * hero2,
	BattleSide side,
	bool replayAllowed
)
{
	Schema::IModel * model;
	const std::string modelkey = side == BattleSide::ATTACKER ? "attacker" : "defender";
	model = GetModel(modelkey);

	auto modelside = model->getSide();
	auto realside = static_cast<Schema::Side>(EI(side));

	if(modelside != realside && modelside != Schema::Side::BOTH)
		logAi->warn("The loaded '%s' model was not trained to play as %s", modelkey, modelkey);

	switch(model->getType())
	{
		case Schema::ModelType::SCRIPTED:
			if(model->getName() == "StupidAI")
			{
				bai = CDynLibHandler::getNewBattleAI("StupidAI");
				bai->initBattleInterface(env, cb, autocombatPreferences);
			}
			else if(model->getName() == "BattleAI")
			{
				bai = CDynLibHandler::getNewBattleAI("BattleAI");
				bai->initBattleInterface(env, cb, autocombatPreferences);
			}
			else
			{
				THROW_FORMAT("Unexpected scripted model name: %s", model->getName());
			}
			break;
		case Schema::ModelType::NN:
			// XXX: must not call initBattleInterface here
			bai = Base::Create(model, env, cb, autocombatPreferences.enableSpellsUsage);
			break;

		default:
			THROW_FORMAT("Unexpected model type: %d", EI(model->getType()));
	}

	bai->battleStart(bid, army1, army2, tile, hero1, hero2, side, replayAllowed);
}

void Router::battleTriggerEffect(const BattleID & bid, const BattleTriggerEffect & bte)
{
	bai->battleTriggerEffect(bid, bte);
}

void Router::battleUnitsChanged(const BattleID & bid, const std::vector<UnitChanges> & changes)
{
	bai->battleUnitsChanged(bid, changes);
}

void Router::yourTacticPhase(const BattleID & bid, int distance)
{
	bai->yourTacticPhase(bid, distance);
}

/*
 * private
 */

void Router::error(const std::string & text) const
{
	log(ELogLevel::ERROR, text);
}
void Router::warn(const std::string & text) const
{
	log(ELogLevel::WARN, text);
}
void Router::info(const std::string & text) const
{
	log(ELogLevel::INFO, text);
}
void Router::debug(const std::string & text) const
{
	log(ELogLevel::DEBUG, text);
}
void Router::trace(const std::string & text) const
{
	log(ELogLevel::TRACE, text);
}
void Router::log(ELogLevel::ELogLevel level, const std::string & text) const
{
	if(logAi->getEffectiveLevel() <= level)
		logAi->debug("Router-%s [%s] %s", addrstr, colorname, text);
}
}
