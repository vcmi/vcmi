#include <cstring>
#include "../../AI_Base.h"


const char *g_cszAiName = "Mad AI";



class CMadAI : public CBattleGameInterface
{
	CBattleCallback *cb;
	virtual void init(CBattleCallback * CB) 
	{
		cb = CB;
	}

	virtual BattleAction activeStack(const CStack * stack) 
	{
		srand(time(NULL));
		BattleAction ba;
		ba.actionType = rand() % 14;
		ba.additionalInfo = rand() % BFIELD_SIZE + 5;
		ba.side = rand() % 7;
		ba.destinationTile = rand() % BFIELD_SIZE + 5;
		ba.stackNumber = rand() % 500;
		return ba;
	}
};

extern "C" DLL_F_EXPORT void GetAiName(char* name)
{
	strcpy_s(name, strlen(g_cszAiName) + 1, g_cszAiName);
}

extern "C" DLL_F_EXPORT CBattleGameInterface* GetNewBattleAI()
{
	return new CMadAI();
}

extern "C" DLL_F_EXPORT void ReleaseBattleAI(CBattleGameInterface* i)
{
	delete (CMadAI*)i;
}
