#include <cstring>
#include "../../AI_Base.h"
#include "../../lib/CBattleCallback.h"


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
// 		int *g = 0;
// 		*g = 4;
//		while(1);

		srand(time(NULL));
		BattleAction ba;
		ba.actionType = rand() % 14;
		ba.additionalInfo = rand() % BFIELD_SIZE + 5;
		ba.side = rand() % 7;
		ba.destinationTile = rand() % BFIELD_SIZE + 5;
		ba.stackNumber = rand() % cb->battleGetAllStacks().size() + 1;
		return ba;
	}
};

extern "C" DLL_F_EXPORT void GetAiName(char* name)
{
	strcpy(name, g_cszAiName);
}

extern "C" DLL_F_EXPORT CBattleGameInterface* GetNewBattleAI()
{
	return new CMadAI();
}

extern "C" DLL_F_EXPORT void ReleaseBattleAI(CBattleGameInterface* i)
{
	delete (CMadAI*)i;
}
