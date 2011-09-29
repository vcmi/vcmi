#include "Client.h"
#include "../lib/Connection.h"
#include "../CThreadHelper.h"
#include "../lib/CGameState.h"
#include "../lib/BattleAction.h"
#include "../lib/CGameInterface.h"
#include "CheckTime.h"

#define NOT_LIB
#include "../lib/RegisterTypes.cpp"

template <typename T> class CApplyOnCL;

class CBaseForCLApply
{
public:
	virtual void applyOnClAfter(CClient *cl, void *pack) const =0; 
	virtual void applyOnClBefore(CClient *cl, void *pack) const =0; 
	virtual ~CBaseForCLApply(){}

	template<typename U> static CBaseForCLApply *getApplier(const U * t=NULL)
	{
		return new CApplyOnCL<U>;
	}
};

template <typename T> class CApplyOnCL : public CBaseForCLApply
{
public:
	void applyOnClAfter(CClient *cl, void *pack) const
	{
		T *ptr = static_cast<T*>(pack);
		ptr->applyCl(cl);
	}
	void applyOnClBefore(CClient *cl, void *pack) const
	{
		T *ptr = static_cast<T*>(pack);
		ptr->applyFirstCl(cl);
	}
};

static CApplier<CBaseForCLApply> *applier = NULL;

void CClient::run()
{
	setThreadName(-1, "CClient::run");
	try
	{
		CPack *pack = NULL;
		while(!terminate)
		{
			pack = serv->retreivePack(); //get the package from the server

			if (terminate) 
			{
				delete pack;
				pack = NULL;
				break;
			}

			handlePack(pack);
			pack = NULL;
		}
	} 
	catch (const std::exception& e)
	{	
		tlog3 << "Lost connection to server, ending listening thread!\n";
		tlog1 << e.what() << std::endl;
		if(!terminate) //rethrow (-> boom!) only if closing connection was unexpected
		{
			tlog1 << "Something wrong, lost connection while game is still ongoing...\n";
			throw;
		}
	}
}

void CClient::handlePack( CPack * pack )
{			
	CBaseForCLApply *apply = applier->apps[typeList.getTypeID(pack)]; //find the applier
	if(apply)
	{
		apply->applyOnClBefore(this,pack);
		tlog5 << "\tMade first apply on cl\n";
		gs->apply(pack);
		tlog5 << "\tApplied on gs\n";
		apply->applyOnClAfter(this,pack);
		tlog5 << "\tMade second apply on cl\n";
	}
	else
	{
		tlog1 << "Message cannot be applied, cannot find applier! TypeID " << typeList.getTypeID(pack) << std::endl;
	}
	delete pack;
}

void CClient::requestMoveFromAI(const CStack *s)
{
	boost::thread(&CClient::requestMoveFromAIWorker, this, s);
}

void CClient::requestMoveFromAIWorker(const CStack *s)
{
	BattleAction ba;

	try
	{
		CheckTime timer("AI was thinking for ");
		ba = ai->activeStack(s);
		MakeAction temp_action(ba);
		*serv << &temp_action;
	}
	catch(...)
	{
		tlog0 << "AI thrown an exception!\n";
	}
}

CClient::CClient()
{
	gs = NULL;
	serv = NULL;
	ai = NULL;
	curbaction = NULL;
	terminate = false;

	applier = new CApplier<CBaseForCLApply>;
	registerTypes2(*applier);
}