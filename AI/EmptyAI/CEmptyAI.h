#pragma once

#include "../../lib/AI_Base.h"
#include "../../CCallback.h"

struct HeroMoveDetails;

class CEmptyAI : public CGlobalAI
{
	std::shared_ptr<CCallback> cb;

public:
	void init(std::shared_ptr<CCallback> CB) override;
	void yourTurn() override;
	void heroGotLevel(const CGHeroInstance *hero, PrimarySkill::PrimarySkill pskill, std::vector<SecondarySkill> &skills, QueryID queryID) override;
	void commanderGotLevel (const CCommanderInstance * commander, std::vector<ui32> skills, QueryID queryID) override;
	void showBlockingDialog(const std::string &text, const std::vector<Component> &components, QueryID askID, const int soundID, bool selection, bool cancel) override;
	void showTeleportDialog(TeleportChannelID channel, TTeleportExitsList exits, bool impassable, QueryID askID) override;
	void showGarrisonDialog(const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits, QueryID queryID) override;
};

#define NAME "EmptyAI 0.1"
