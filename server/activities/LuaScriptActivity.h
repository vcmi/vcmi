/*
 * LuaScriptActivity.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "Activity.h"

/// Keeps a paused map script coroutine alive between blocking actions. Sits on the stack
/// between the object visit activity and the dialog or battle that the script started. On
/// removal of that child it is exposed and resumes the coroutine, popping itself at the end.
class LuaScriptActivity : public Activity
{
public:
	static constexpr ActivityType TYPE = ActivityType::LuaScript;

	LuaScriptActivity(CGameHandler * owner, PlayerColor player);

	void setCoroutine(int handle);
	void setPendingAnswer(std::optional<int32_t> answer);
	void setVisitingHero(ObjectInstanceID hero);

	void onExposure(ActivityPtr topActivity) override;

private:
	int coroutineHandle = 0;
	std::optional<int32_t> pendingAnswer;
	ObjectInstanceID visitingHero; //if set and the hero is gone (lost a scripted combat), the coroutine is abandoned
};
