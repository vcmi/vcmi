/*
 * HighScore.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "HighScore.h"
#include "../CPlayerState.h"
#include "../constants/StringConstants.h"
#include "CGameState.h"
#include "StartInfo.h"
#include "../mapping/CMapHeader.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/CGTownInstance.h"

VCMI_LIB_NAMESPACE_BEGIN

HighScoreParameter HighScore::prepareHighScores(const CGameState * gs, PlayerColor player, bool victory)
{
	const auto * playerState = gs->getPlayerState(player);

	HighScoreParameter param;
	param.difficulty = gs->getStartInfo()->difficulty;
	param.day = gs->getDate();
	param.townAmount = gs->howManyTowns(player);
	param.usedCheat = gs->getPlayerState(player)->cheated;
	param.hasGrail = false;
	for(const CGHeroInstance * h : playerState->getHeroes())
		if(h->hasArt(ArtifactID::GRAIL))
			param.hasGrail = true;
	for(const CGTownInstance * t : playerState->getTowns())
		if(t->hasBuilt(BuildingID::GRAIL))
			param.hasGrail = true;
	param.allEnemiesDefeated = true;
	for (PlayerColor otherPlayer(0); otherPlayer < PlayerColor::PLAYER_LIMIT; ++otherPlayer)
	{
		auto ps = gs->getPlayerState(otherPlayer, false);
		if(ps && otherPlayer != player && !ps->checkVanquished())
			param.allEnemiesDefeated = false;
	}
	param.scenarioName = gs->getMapHeader()->name.toString();
	param.playerName = gs->getStartInfo()->playerInfos.find(player)->second.name;

	return param;
}

HighScoreCalculation::Result HighScoreCalculation::calculate()
{
	Result firstResult;
	Result summary;
	const std::array<double, 5> difficultyMultipliers{0.8, 1.0, 1.3, 1.6, 2.0};
	for(const auto & param : parameters)
	{
		double tmp = 200 - (param.day + 10) / (param.townAmount + 5) + (param.allEnemiesDefeated ? 25 : 0) + (param.hasGrail ? 25 : 0);
		firstResult = Result{static_cast<int>(tmp), static_cast<int>(tmp * difficultyMultipliers.at(param.difficulty)), param.day, param.usedCheat};
		summary.basic += firstResult.basic * 5.0 / parameters.size();
		summary.total += firstResult.total * 5.0 / parameters.size();
		summary.sumDays += firstResult.sumDays;
		summary.cheater |= firstResult.cheater;
	}

	if(parameters.size() == 1)
		return firstResult;

	return summary;
}

struct HighScoreCreature
{
	CreatureID creature;
	int min;
	int max;
};

static std::vector<HighScoreCreature> getHighscoreCreaturesList()
{
	JsonNode configCreatures(JsonPath::builtin("CONFIG/highscoreCreatures.json"));

	std::vector<HighScoreCreature> ret;

	for(auto & json : configCreatures["creatures"].Vector())
	{
		HighScoreCreature entry;
		entry.creature = CreatureID::decode(json["creature"].String());
		entry.max = json["max"].isNull() ? std::numeric_limits<int>::max() : json["max"].Integer();
		entry.min = json["min"].isNull() ? std::numeric_limits<int>::min() : json["min"].Integer();

		ret.push_back(entry);
	}

	return ret;
}

CreatureID HighScoreCalculation::getCreatureForPoints(int points, bool campaign)
{
	static const std::vector<HighScoreCreature> creatures = getHighscoreCreaturesList();

	int divide = campaign ? 5 : 1;

	for(auto & creature : creatures)
		if(points / divide <= creature.max && points / divide >= creature.min)
			return creature.creature;

	throw std::runtime_error("Unable to find creature for score " + std::to_string(points));
}

VCMI_LIB_NAMESPACE_END
