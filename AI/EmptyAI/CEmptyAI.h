/*
 * CEmptyAI.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/callback/CGlobalAI.h"

struct HeroMoveDetails;

class CEmptyAI : public CGlobalAI
{
	std::shared_ptr<CCallback> cb;

public:
	void initGameInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CCallback> CB) override;
	void yourTurn(QuestionID questionID) override;
	void yourTacticPhase(const BattleID & battleID, int distance) override;
	void activeStack(const BattleID & battleID, const CStack * stack) override;
	void heroGotLevel(const CGHeroInstance *hero, PrimarySkill pskill, std::vector<SecondarySkill> &skills, QuestionID questionID) override;
	void commanderGotLevel (const CCommanderInstance * commander, std::vector<ui32> skills, QuestionID questionID) override;
	void showBlockingDialog(const std::string &text, const std::vector<Component> &components, QuestionID questionID, const int soundID, bool selection, bool cancel, bool safeToAutoaccept) override;
	void showTeleportDialog(const CGHeroInstance * hero, TeleportChannelID channel, TTeleportExitsList exits, bool impassable, QuestionID questionID) override;
	void showGarrisonDialog(const CArmedInstance * up, const CGHeroInstance * down, bool removableUnits, QuestionID questionID, const MetaString & customTitle) override;
	void showMapObjectSelectDialog(QuestionID questionID, const Component & icon, const MetaString & title, const MetaString & description, const std::vector<ObjectInstanceID> & objects) override;
	std::optional<BattleAction> makeSurrenderRetreatDecision(const BattleID & battleID, const BattleStateInfoForRetreat & battleState) override;
};

#define NAME "EmptyAI 0.1"
