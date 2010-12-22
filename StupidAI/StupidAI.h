#pragma once

class CStupidAI : public CBattleGameInterface
{
public:
	CStupidAI(void);
	~CStupidAI(void);

	void init(IBattleCallback * CB) OVERRIDE;
	void actionFinished(const BattleAction *action) OVERRIDE;//occurs AFTER every action taken by any stack or by the hero
	void actionStarted(const BattleAction *action) OVERRIDE;//occurs BEFORE every action taken by any stack or by the hero
	BattleAction activeStack(int stackID) OVERRIDE; //called when it's turn of that stack
};

