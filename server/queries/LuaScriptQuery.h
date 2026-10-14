/*
 * LuaScriptQuery.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CQuery.h"

/// Keeps a paused map-script coroutine alive between blocking actions. Sits on the query stack between
/// the object-visit query and whatever child (dialog / battle) the script spawned; when that child is
/// removed this query is exposed and resumes the coroutine, popping itself once the script finishes.
class LuaScriptQuery : public CQuery
{
public:
	static constexpr QueryType TYPE = QueryType::LuaScript;

	LuaScriptQuery(CGameHandler * owner, PlayerColor player);

	void setCoroutine(int handle);
	void setPendingAnswer(std::optional<int32_t> answer);
	void setVisitingHero(ObjectInstanceID hero);

	void onExposure(QueryPtr topQuery) override;

private:
	int coroutineHandle = 0;
	std::optional<int32_t> pendingAnswer;
	ObjectInstanceID visitingHero; //if set and the hero is gone (lost a scripted combat), the coroutine is abandoned
};
